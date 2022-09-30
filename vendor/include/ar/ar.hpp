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

#include <estd/isubstream.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <vector>

namespace ar {
	namespace// anonymous namespace
	{
#pragma pack(push, 1)
		struct raw_header {
			uint8_t name[16];
			uint8_t mtime[12];
			uint8_t uid[6];
			uint8_t gid[6];

			uint8_t mode[8];

			uint8_t size[10];
			uint8_t end[2];
		};
#pragma pack(pop)

		struct parsed_header {
			std::string name;
			uint8_t mode;
			uint64_t uid;
			uint64_t gid;
			uint64_t size;
			uint64_t mtime;
		};

		bool StartsWith(const std::string& str, const std::string& prefix) {
			return prefix == "" || prefix == "." || str.rfind(prefix, 0) == 0;
		}

		std::string ReplacePrefix(std::string str, const std::string& from, const std::string& to) {
			size_t sArt_pos = 0;
			if ((sArt_pos = str.rfind(from, 0)) != std::string::npos) { str.replace(sArt_pos, from.length(), to); }
			return str;
		}

		const std::string WHITESPACE = " \n\r\t\f\v";

		std::string ltrim(const std::string& s, std::string whitespace = WHITESPACE) {
			size_t sArt = s.find_first_not_of(whitespace);
			return (sArt == std::string::npos) ? "" : s.substr(sArt);
		}

		std::string rtrim(const std::string& s, std::string whitespace = WHITESPACE) {
			size_t end = s.find_last_not_of(whitespace);
			return (end == std::string::npos) ? "" : s.substr(0, end + 1);
		}

		std::string trim(const std::string& s, std::string whitespace = WHITESPACE) {
			return rtrim(ltrim(s, whitespace), whitespace);
		}
	}// namespace

	class Reader {
	private:
		raw_header unpackPosixHeader(const std::array<char, sizeof(raw_header)>& arr) {
			return unpackPosixHeader(arr.data());
		}

		raw_header unpackPosixHeader(const char* raw) {
			raw_header header;
			header = *((raw_header*)raw);
			return header;
		}

		std::array<char, sizeof(raw_header)> packPosixHeader(const raw_header& header) {
			std::array<char, sizeof(raw_header)> raw;
			char* headerAsCharArray = ((char*)(&header));
			std::copy(headerAsCharArray, headerAsCharArray + sizeof(header), raw.data());
			return raw;
		}

		inline parsed_header parsePosixHeader(std::array<char, sizeof(raw_header)> const& buffer) {
			auto header = unpackPosixHeader(buffer.data());
			parsed_header result;
			result.size =
				static_cast<std::size_t>(std::stol(std::string((char*)header.size, sizeof(header.size)), 0, 10));
			result.name = std::string((char*)header.name, sizeof(header.name));
			result.name = std::string(result.name.c_str());
			result.name = trim(result.name);
			return result;
		}
		std::unique_ptr<std::ifstream> inputStreamPtr;
		std::istream& inputStream;
		std::map<std::string, std::size_t> files;

	public:
		bool throwOnUnsupported = true;
		bool allowSeekg = true;
		Reader(std::string const& filename) :
			inputStreamPtr(std::make_unique<std::ifstream>(filename, std::ios_base::in | std::ios_base::binary)),
			inputStream(*inputStreamPtr.get()) {}

		Reader(std::istream& is) : inputStream(is) {}

		estd::isubstream open(std::string filename) {
			inputStream.clear();
			inputStream.seekg(0, std::ios::beg);

			std::string magicBytes;
			magicBytes.resize(8);

			inputStream.read(magicBytes.data(), magicBytes.size());

			if (magicBytes != "!<arch>\n")
				throw std::runtime_error("Not an ar-file, wrong magic bytes " + std::string(magicBytes));

			std::array<char, sizeof(raw_header)> buffer{};
			std::array<char, 4096> dataBuffer;

			while (inputStream) {
				inputStream.read(buffer.data(), sizeof(raw_header));

				if (inputStream.eof()) break;
				if (inputStream.fail()) throw std::runtime_error("Failed to read ar-file.");

				parsed_header header;
				header = parsePosixHeader(buffer);


				if (header.name == "//") {
					std::string longname;
					longname.resize(header.size);
					inputStream.read(longname.data(), header.size);
					if (header.size % 2) inputStream.read(dataBuffer.data(), 1);// even align
					if (inputStream.eof()) throw std::runtime_error("Failed to read ar-file.");

					inputStream.read(buffer.data(), sizeof(raw_header));
					if (inputStream.eof()) throw std::runtime_error("Failed to read ar-file.");

					header = parsePosixHeader(buffer);
					header.name = longname;
				}

				header.name = trim(header.name, WHITESPACE + "/");

				if (header.name == filename) {
					return estd::isubstream(inputStream.rdbuf(), inputStream.tellg(), header.size);
				} else {
					if (allowSeekg) {
						std::streampos pos = header.size + (header.size % 2);
						inputStream.seekg(inputStream.tellg() + pos);
					} else {
						for (std::size_t i = header.size + (header.size % 2); i > 0;) {
							int howMuchToRead = (dataBuffer.size() < i) ? (dataBuffer.size()) : (i);
							inputStream.read(dataBuffer.data(), howMuchToRead);
							i -= howMuchToRead;
						}
					}
				}

				if (!inputStream) { throw std::runtime_error("Ar filename-entry with illegal size: " + header.name); }
			}
			throw std::runtime_error("Ar filename-entry not found: " + filename);
		}

		void extractAll(std::filesystem::path destination) { extractPath("./", destination); }

		void extractPath(std::filesystem::path source, std::filesystem::path destination) {
			inputStream.clear();
			inputStream.seekg(0, std::ios::beg);

			std::string magicBytes;
			magicBytes.resize(8);

			inputStream.read(magicBytes.data(), magicBytes.size());

			if (magicBytes != "!<arch>\n") {
				throw std::runtime_error("Not an ar-file, wrong magic bytes " + std::string(magicBytes));
			}

			files.clear();

			std::array<char, sizeof(raw_header)> buffer{};
			std::array<char, 4096> dataBuffer;

			while (inputStream) {
				inputStream.read(buffer.data(), sizeof(raw_header));

				if (inputStream.eof()) break;
				if (inputStream.fail()) throw std::runtime_error("Failed to read ar-file.");

				parsed_header header;
				header = parsePosixHeader(buffer);


				if (header.name == "//") {
					std::string longname;
					longname.resize(header.size);
					inputStream.read(longname.data(), header.size);
					if (header.size % 2) inputStream.read(dataBuffer.data(), 1);// even align
					if (inputStream.eof()) throw std::runtime_error("Failed to read ar-file.");

					inputStream.read(buffer.data(), sizeof(raw_header));
					if (inputStream.eof()) throw std::runtime_error("Failed to read ar-file.");

					header = parsePosixHeader(buffer);
					header.name = longname;
				}

				header.name = trim(header.name, WHITESPACE + "/");


				if (files.count(header.name)) {
					throw std::runtime_error("Duplicate filename-entry while reading Ar-file: " + header.name);
				}

				files[header.name] = 0;

				std::filesystem::path inArPath = header.name;
				destination = destination.lexically_normal();
				source = source.lexically_normal();
				inArPath = inArPath.lexically_normal();
				auto path = destination / inArPath;

				bool canSkip = false;
				if (!StartsWith(inArPath.string(), source.string())) {
					canSkip = true;
				} else {
					inArPath = ReplacePrefix(inArPath.string(), source.string(), "");
					inArPath = inArPath.lexically_normal();
				}

				if (inArPath.string() == "") path = destination;

				if (canSkip) {
					for (std::size_t i = header.size + (header.size % 2); i > 0;) {
						int howMuchToRead = (dataBuffer.size() < i) ? (dataBuffer.size()) : (i);
						inputStream.read(dataBuffer.data(), howMuchToRead);
						i -= howMuchToRead;
					}
				} else {
					if (!path.has_filename()) path.replace_filename(source.filename());


					std::filesystem::create_directories(path.parent_path());
					std::ofstream f = std::ofstream(path, std::ios::out | std::ios::binary);

					for (std::size_t i = header.size; i > 0;) {
						int howMuchToRead = (dataBuffer.size() < i) ? (dataBuffer.size()) : (i);
						inputStream.read(dataBuffer.data(), howMuchToRead);
						f.write(dataBuffer.data(), howMuchToRead);
						i -= howMuchToRead;
					}

					f.close();

					if (header.size % 2) inputStream.read(dataBuffer.data(), 1);
				}

				if (!inputStream) { throw std::runtime_error("Ar filename-entry with illegal size: " + header.name); }
			}
		}
	};
};// namespace ar
