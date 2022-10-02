// BSD 3-Clause License

// Copyright (c) 2022, Alex Tarasov
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:

// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.

// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.

// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include <array>
#include <estd/filesystem.hpp>
#include <estd/isubstream.hpp>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <vector>

namespace tar {
	namespace// anonymous namespace
	{
		using estd::files::Path;
#pragma pack(push, 1)
		struct posix_header {	   /* byte offset */
			uint8_t name[100];	   /*   0 */
			uint8_t mode[8];	   /* 100 */
			uint8_t uid[8];		   /* 108 */
			uint8_t gid[8];		   /* 116 */
			uint8_t size[12];	   /* 124 */
			uint8_t mtime[12];	   /* 136 */
			uint8_t chksum[8];	   /* 148 */
			uint8_t typeflag;	   /* 156 */
			uint8_t linkname[100]; /* 157 */
			uint8_t magic[6];	   /* 257 */
			uint8_t version[2];	   /* 263 */
			uint8_t uname[32];	   /* 265 */
			uint8_t gname[32];	   /* 297 */
			uint8_t devmajor[8];   /* 329 */
			uint8_t devminor[8];   /* 337 */
			uint8_t prefix[155];   /* 345 */
			uint8_t padding[12];   /* 500 */
								   /* 512 */
		};
#pragma pack(pop)

		struct parsed_posix_header {
			std::string name;
			uint8_t mode;
			uint64_t uid;
			uint64_t gid;
			uint64_t size;
			uint64_t mtime;
			std::string chksum;
			uint8_t typeflag;
			std::string linkname;
			std::string magic;
			std::string version;
			std::string uname;
			std::string gname;
			std::string devmajor;
			std::string devminor;
			std::string prefix;
		};
	}// namespace

	class Reader {
	private:
		template <class T>
		inline void wrapFilesystemCall(T fsCall) {
			if (throwOnFilesystemFailures) {
				fsCall();
			} else {
				try {
					fsCall();
				} catch (...) {}
			}
		}

		std::pair<bool, Path> changeRoot(Path path, Path from, Path to) {
			path = ("." / path).normalize();// paths in tar file can only be relative
			from = ("." / from).normalize();// prefix paths can only be relative
			to = (to).normalize();			// if you wish, you can extract to an absolute path
			auto res = path.replacePrefix(from, to);
			return {res.has_value(), res.value_or("")};
		}


		posix_header unpackPosixHeader(const std::array<char, 512> raw) {
			posix_header header;
			header = *((posix_header*)raw.data());
			return header;
		}

		std::array<char, 512> packPosixHeader(const posix_header& header) {
			std::array<char, 512> raw;
			char* headerAsCharArray = ((char*)(&header));
			std::copy(headerAsCharArray, headerAsCharArray + sizeof(posix_header), raw.data());
			return raw;
		}

		inline std::string calc_checksum(posix_header header) {
			uint8_t* headerAsCharArray = ((uint8_t*)(&header));
			for (uint64_t i = 0; i < sizeof(header.chksum); i++) header.chksum[i] = ' ';

			unsigned long long sum = 0;
			for (uint64_t i = 0; i < sizeof(posix_header); i++) sum += headerAsCharArray[i];

			std::ostringstream os;
			os << std::oct << std::setfill('0') << std::setw(6) << sum << '\0' << ' ';

			return os.str();
		}

		inline parsed_posix_header parsePosixHeader(std::array<char, 512> const& buffer) {
			auto header = unpackPosixHeader(buffer);
			parsed_posix_header result;
			result.magic = std::string((char*)header.magic, sizeof(header.magic));
			if (result.magic != "ustar ") {
				throw std::runtime_error(
					"Tar: loaded file without magic 'ustar', magic is: '" + std::string(result.magic) + "'"
				);
			}
			result.chksum = std::string((char*)header.chksum, sizeof(header.chksum));
			result.size =
				static_cast<std::size_t>(std::stol(std::string((char*)header.size, sizeof(header.size)), 0, 8));
			result.name = std::string((char*)header.name, sizeof(header.name));
			result.name = std::string(result.name.c_str());
			result.linkname = std::string((char*)header.linkname, sizeof(header.linkname));
			result.linkname = std::string(result.linkname.c_str());
			result.typeflag = header.typeflag;

			if (result.chksum != calc_checksum(header)) {
				throw std::runtime_error(
					std::string("Tar: loaded file with wrong checksum '") + std::string(result.chksum) +
					std::string("' != '") + std::string(calc_checksum(header) + "'")
				);
			}

			return result;
		}

		bool isBufferAllZeros(char* buff, size_t len) {
			bool isAllZero = true;
			for (size_t i = 0; i < len; i++)
				if (buff[i] != 0) isAllZero = false;
			return isAllZero;
		}

		Path followSoftlink(Path pathToGet) {
			Path& right = pathToGet;
			Path left = ".";
			while (right != "" && right != ".") {
				Path tmp = "";
				std::tie(tmp, right) = right.splitPrefix();
				left = (left / tmp).normalize();
				if (softLinks.count(left)) left = (left.getAntiSuffix() / softLinks[left]).normalize();
			}
			return left.normalize();
		}

		uint16_t toUnixPermissions(uint16_t tarP) {
			// octal [owner][group][other] 0=none, 1=e/r/w, 2=r/w, 3=r
			uint16_t unixP = 0;
			auto chunkToUnix = [](uint16_t chunk) {
				chunk &= 7;// 7 in octal
				switch (chunk) {
					case 0: return 0;// none
					case 1: return 7;// all
					case 2: return 6;// read and write
					case 3: return 4;// read only
				}
				return 6;
			};
			unixP = (chunkToUnix(tarP)) | (chunkToUnix(tarP >> 3) << 3) | (chunkToUnix(tarP >> 6) << 6);
			return unixP;
		}

		template <bool extract = false>
		void indexFiles(Path source, Path destination) {
			if (paths.size() != 0 && !extract) return;

			files.clear();
			hardLinks.clear();
			softLinks.clear();
			paths.clear();

			inputStream.clear();
			inputStream.seekg(0, std::ios::beg);

			std::array<char, 512> buffer;
			while (inputStream) {
				do {
					if (!inputStream.read(buffer.data(), 512)) break;
				} while (isBufferAllZeros(buffer.data(), 512));
				if (!inputStream) break;

				parsed_posix_header header = parsePosixHeader(buffer);

				if (header.name == "././@LongLink") {
					std::string longname = "";
					longname.resize(header.size);
					if (longname.length() != header.size) {
						throw std::runtime_error(
							"Tar: could not allocate a string of size " + std::to_string(header.size)
						);
					}
					if (!inputStream.read(longname.data(), header.size)) {
						throw std::runtime_error("Tar: failed to read longname of size " + std::to_string(header.size));
					}

					int nameBlockOffset = (512 - (header.size % 512)) % 512;
					inputStream.read(buffer.data(), nameBlockOffset);
					inputStream.read(buffer.data(), 512);
					if (!inputStream) break;
					header = parsePosixHeader(buffer);
					header.name = longname;
				}

				std::streampos dataBlockOffset = (512 - (header.size % 512)) % 512;
				std::streampos seekAfterEntry = inputStream.tellg() + std::streamoff(header.size + dataBlockOffset);

				Path inTarPath = header.name;
				inTarPath = inTarPath.normalize();

				if (header.typeflag == '5') {
					inTarPath = inTarPath.addEmptySuffix();
				} else {
					inTarPath = inTarPath.removeEmptySuffix();
				}

				Path extractPath;
				bool isValidForExtract;
				if (extract) std::tie(isValidForExtract, extractPath) = changeRoot(header.name, source, destination);

				if (paths.count(inTarPath.string())) {
					throw std::runtime_error(
						"Tar: duplicate filename-entry while reading tar-file: " + inTarPath.string()
					);
				}

				paths.insert(inTarPath.string());
				permissions[inTarPath.string()] = toUnixPermissions(header.mode) | minPermissions;

				if (header.typeflag == '0' || header.typeflag == '\0') {// is file
					auto filestream =
						estd::isubstream(inputStream.rdbuf(), inputStream.tellg(), std::streamsize(header.size));
					files.insert({
						inTarPath,
						filestream,
					});

					if (extract && isValidForExtract) {
						if (!extractPath.hasSuffix()) extractPath.replaceSuffix(source.getSuffix());
						wrapFilesystemCall([&] {
							estd::files::createDirectories(extractPath.getAntiSuffix());
							std::ofstream f = std::ofstream(extractPath, std::ios::out | std::ios::binary);
							if (f.fail()) throw std::runtime_error("Tar: failed to create file: " + extractPath);
							f << filestream.rdbuf();
							f.close();
							estd::files::setPermissions(extractPath, permissions[inTarPath.string()]);
						});
					}
				} else if (header.typeflag == '5') {// is dir
					if (extract && isValidForExtract) {
						wrapFilesystemCall([&] {
							estd::files::createDirectories(extractPath);
							estd::files::setPermissions(extractPath, permissions[inTarPath.string()]);
						});
					}
				} else if (header.typeflag == '1') {// hard
					Path linkPath = header.linkname;
					hardLinks.insert({inTarPath, linkPath.normalize()});
				} else if (header.typeflag == '2') {// soft
					// soft link can link to a folder
					Path linkPath = header.linkname;
					softLinks.insert({inTarPath, linkPath.normalize()});
				} else {
					if (throwOnUnsupported) {
						throw std::runtime_error(
							"Tar: tar has an unsuppoted entry type (TODO - not "
							"implemented): " +
							header.name
						);
					}
				}
				inputStream.seekg(seekAfterEntry);

				if (!inputStream) {
					throw std::runtime_error("Tar: tar filename-entry with illegal size: " + header.name);
				}
			}

			if (!extract) return;

			for (auto& hardLink : hardLinks) {
				Path extractPath;
				bool isValidForExtract;
				std::tie(isValidForExtract, extractPath) = changeRoot(hardLink.first, source, destination);
				if (!isValidForExtract) continue;

				wrapFilesystemCall([&] { estd::files::createDirectories(extractPath.getAntiSuffix()); });

				if (!extractHardLinksAsCopies) {
					bool isValid;
					Path localPath;
					std::tie(isValid, localPath) = changeRoot(hardLink.second, source, destination);
					if (isValid) {
						wrapFilesystemCall([&] {
							estd::files::createHardLink(localPath, extractPath);
							estd::files::setPermissions(extractPath, permissions[hardLink.first]);
						});
						continue;// skip the copy code then.
					}
				}
				wrapFilesystemCall([&] {
					estd::isubstream sourceFile = open(hardLink.second);
					std::ofstream destinationFile = std::ofstream(extractPath, std::ios::out | std::ios::binary);
					if (destinationFile.fail()) throw std::runtime_error("Tar: failed to create file: " + extractPath);
					destinationFile << sourceFile.rdbuf();
					destinationFile.close();
					estd::files::setPermissions(extractPath, permissions[hardLink.first]);
				});
			}

			for (auto& softLink : softLinks) {
				Path extractPath;
				Path sourcePath = softLink.first;
				bool isValidForExtract;
				std::tie(isValidForExtract, extractPath) = changeRoot(sourcePath, source, destination);
				if (!isValidForExtract) continue;
				std::set<std::string> visited = {};
				extractSoftlinks(sourcePath, extractPath, source, visited);
			}
		}

		bool isExistingDirectory(Path p) {
			p = p.addEmptySuffix().normalize();
			if (paths.count(p)) return true;
			return false;
		}

		bool isExistingFile(Path p) {
			p = p.removeEmptySuffix().normalize();
			if (files.count(p) || hardLinks.count(p)) return true;
			return false;
		}


		void extractSoftlinks(Path path, Path destination, Path origExtPath, std::set<std::string>& visited) {
			if (isExistingDirectory(path)) {
				path = path.addEmptySuffix();
				wrapFilesystemCall([&] {
					estd::files::createDirectories(destination);
					estd::files::setPermissions(destination, permissions[path]);
				});
				for (Path subpath : paths) {
					if (path.contains(subpath) && subpath != path) {
						Path subdestination = subpath.replacePrefix(path, destination).value();
						if (!isExistingDirectory(subpath)) {
							extractSoftlinks(subpath, subdestination, origExtPath, visited);
						} else {
							wrapFilesystemCall([&] {
								estd::files::createDirectories(subdestination);
								estd::files::setPermissions(subdestination, permissions[subpath]);
							});
						}
					}
				}
				return;
			} else if (isExistingFile(path)) {
				path = path.removeEmptySuffix();
				estd::isubstream file;
				try {
					file = open(path);
				} catch (...) { throw std::runtime_error("Tar: file stream could not be opened " + path.string()); }
				wrapFilesystemCall([&] {
					estd::files::createDirectories(destination.getAntiSuffix());
					std::ofstream destinationFile = std::ofstream(destination, std::ios::out | std::ios::binary);
					if (destinationFile.fail()) throw std::runtime_error("Tar: failed to create file: " + destination);
					destinationFile << file.rdbuf();
					destinationFile.close();
					estd::files::setPermissions(destination, permissions[path]);
				});
				return;
			}

			if (!softLinks.count(path.removeEmptySuffix())) {
				if (!paths.count(path)) {
					if (throwOnUnsupported) throw std::runtime_error("Tar: broken softlink points to " + path.string());
				} else if (throwOnUnsupported) {
					throw std::runtime_error("Tar: unknown entry type " + path.string());
				}
				return;// ensure it is a softlink anyways
			}

			// is a softlink from here

			if (visited.count(path.removeEmptySuffix())) {
				if (throwOnInfiniteRecursion)
					throw std::runtime_error("Tar: infinite recursion detected in symlink " + path.string());
				return;
			} else {
				visited.insert(path.removeEmptySuffix());
			}

			Path linkedPath = softLinks[path];
			Path rootLinkedPath = (path.getAntiSuffix() / linkedPath).normalize().removeEmptySuffix();
			bool isExtracted = origExtPath.contains(rootLinkedPath);

			// if extracting as a copy, or what it points to is not extracted
			if (extractSoftLinksAsCopies || !isExtracted) {
				// if link is real
				if (isExistingDirectory(rootLinkedPath) || isExistingFile(rootLinkedPath)) {
					// extract as copies
					extractSoftlinks(rootLinkedPath, destination, origExtPath, visited);
					return;
				}
			}
			if (!isExtracted) {// if what it points to is not extracted
				if (skipOnBrokenSoftlinks) {
					return;
				} else if (throwOnBrokenSoftlinks) {
					throw std::runtime_error(
						"Tar: softlink " + rootLinkedPath + " is broken and points to " + linkedPath
					);
				}
			}

			wrapFilesystemCall([&] { estd::files::createSoftLinkRelative(linkedPath, destination); });
		}

		std::unique_ptr<std::ifstream> inputStreamPtr;
		std::istream& inputStream;
		std::map<std::string, estd::isubstream> files;
		std::map<std::string, std::string> hardLinks;
		std::map<std::string, std::string> softLinks;
		std::set<std::string> paths;
		std::map<std::string, uint16_t> permissions;

	public:
		// will throw if block files detected
		bool throwOnUnsupported = false;

		// if false will just not extract the tree ater the loop is detected, will throw if true
		bool throwOnInfiniteRecursion = false;

		// will extract all hardlinks as copies
		// (no exceptions, hardlinks cannot be broken)
		bool extractHardLinksAsCopies = false;

		// will extract all softlinks as copies when the links are not broken
		// (otherwise, will extract as softlink that points to a broken location)
		bool extractSoftLinksAsCopies = false;

		// will throw when a broken softlink is detected
		bool throwOnBrokenSoftlinks = false;

		// skip broken softlinks when they are detected, will still throw if that is enabled
		bool skipOnBrokenSoftlinks = false;

		// this option is relevant only for the open function.
		bool followSoftlinks = true;

		// throw on filesystem failures
		bool throwOnFilesystemFailures = false;

		// permissions will be OR'd with this mask (octal permission example permissionMask = 0777)
		uint16_t minPermissions = 644;


		Reader(std::string const& filename) :
			inputStreamPtr(std::make_unique<std::ifstream>(filename, std::ios_base::in | std::ios_base::binary)),
			inputStream(*inputStreamPtr.get()) {
			if (inputStream.fail()) throw std::runtime_error("Tar: could not open file: " + filename);
		}

		Reader(std::istream& is) : inputStream(is) {
			if (inputStream.fail()) throw std::runtime_error("Tar: failed stream provided");
		}

		void indexFiles() { indexFiles<false>("", ""); }

		estd::isubstream open(Path source) {
			indexFiles();
			source = source.normalize();

			if (followSoftlinks) { source = followSoftlink(source); }

			if (!files.count(source)) {// search hardlinks then
				if (!hardLinks.count(source)) {
					throw std::runtime_error("Tar: File " + source.string() + " was not found in the archive");
				}

				std::string linksTo = hardLinks[source];

				if (!files.count(linksTo))
					throw std::runtime_error(
						"Tar: Hardlink " + source.string() + " is broken and does not point to a file"
					);

				return files[linksTo];
			}
			return files[source];
		}

		void extractAll(Path destination) { extractPath("./", destination); }

		void extractPath(Path source, Path destination) { indexFiles<true>(source, destination); }
	};
};// namespace tar
