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
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <vector>

namespace tar {
    namespace // anonymous namespace
    {
#pragma pack(0)
        struct posix_header {      /* byte offset */
            uint8_t name[100];     /*   0 */
            uint8_t mode[8];       /* 100 */
            uint8_t uid[8];        /* 108 */
            uint8_t gid[8];        /* 116 */
            uint8_t size[12];      /* 124 */
            uint8_t mtime[12];     /* 136 */
            uint8_t chksum[8];     /* 148 */
            uint8_t typeflag;      /* 156 */
            uint8_t linkname[100]; /* 157 */
            uint8_t magic[6];      /* 257 */
            uint8_t version[2];    /* 263 */
            uint8_t uname[32];     /* 265 */
            uint8_t gname[32];     /* 297 */
            uint8_t devmajor[8];   /* 329 */
            uint8_t devminor[8];   /* 337 */
            uint8_t prefix[155];   /* 345 */
            uint8_t padding[12];   /* 500 */
                                   /* 512 */
        };

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
    } // namespace

    class Reader {
    private:
        std::string replacePrefix(std::string str, const std::string& from, const std::string& to) {
            size_t start_pos = 0;
            if ((start_pos = str.rfind(from, 0)) != std::string::npos) { str.replace(start_pos, from.length(), to); }
            return str;
        }

        std::pair<std::filesystem::path, std::filesystem::path> splitPathPrefix(std::filesystem::path p) {
            size_t start_pos = 0;
            std::string psL = p.string();
            std::string psR = psL;
            if ((start_pos = psL.find("/")) != std::string::npos) {
                psL = psL.substr(0, start_pos);
                psR = psR.substr(start_pos + 1);
            } else {
                return {psL, ""};
            }
            return {psL, psR};
        }

        std::pair<bool, std::filesystem::path> changeRoot(
            std::filesystem::path path, std::filesystem::path from, std::filesystem::path to
        ) {
            path = ("." / path).lexically_normal();
            from = ("." / from).lexically_normal();
            to = ("." / to).lexically_normal();

            if (from == "" || from == "." || from == "./") { return {true, (to / path).lexically_normal()}; }

            bool fromIsDir = !from.has_filename();
            bool toIsDir = !to.has_filename();
            bool pathIsDir = !path.has_filename();

            to = (to / "").lexically_normal();
            from = (from / "").lexically_normal();
            path = (path / "").lexically_normal();

            if (path.string().rfind(from, 0) != 0) { return {false, path}; }

            std::filesystem::path result;
            if (toIsDir && !fromIsDir) { //from is a file && to is a dir
                to = to.replace_filename(from.parent_path().filename());
                to = (to / "").lexically_normal();
            }

            // case: from is a dir && to is a file
            //     dir cannot me moved to a file,
            //     must also be a dir (same as two dirs)
            // case: two dirs

            // do nothing in this case

            if (from == "" || from == "." || from == "./") result = to / path;
            else
                result = replacePrefix(path, from, to);

            result = result.lexically_normal();

            if (!pathIsDir) result = result.parent_path().lexically_normal();

            return {true, result};
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

        std::filesystem::path followSoftlink(std::filesystem::path pathToGet) {
            std::filesystem::path& right = pathToGet;
            std::filesystem::path left = "";
            while (right != "" && right != ".") {
                std::filesystem::path tmp = "";
                std::tie(tmp, right) = splitPathPrefix(right);
                left /= tmp;
                if (softLinks.count(left)) left = (left.parent_path() / softLinks[left]).lexically_normal();
            }
            return left.lexically_normal();
        }

        template <bool extract = false>
        void indexFiles(std::filesystem::path source, std::filesystem::path destination) {
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
                        throw std::runtime_error("Could not allocate a string of size " + std::to_string(header.size));
                    }
                    if (!inputStream.read(longname.data(), header.size)) {
                        throw std::runtime_error("Failed to read longname of size " + std::to_string(header.size));
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

                std::filesystem::path inTarPath = header.name;
                inTarPath = inTarPath.lexically_normal();

                std::filesystem::path extractPath;
                bool isValidForExtract;
                if (extract) std::tie(isValidForExtract, extractPath) = changeRoot(header.name, source, destination);

                if (paths.count(inTarPath.string())) {
                    throw std::runtime_error("Duplicate filename-entry while reading tar-file: " + inTarPath.string());
                }

                paths.insert(inTarPath.string());

                if (header.typeflag == '0' || header.typeflag == '\0') { // is file
                    auto filestream =
                        estd::isubstream(inputStream.rdbuf(), inputStream.tellg(), std::streamsize(header.size));
                    files.insert({
                        inTarPath,
                        filestream,
                    });

                    if (extract && isValidForExtract) {
                        if (!extractPath.has_filename()) extractPath.replace_filename(source.filename());
                        std::filesystem::create_directories(extractPath.parent_path());
                        std::ofstream f = std::ofstream(extractPath, std::ios::out | std::ios::binary);
                        f << filestream.rdbuf();
                        f.close();
                    }
                } else if (header.typeflag == '5') { // is dir
                    if (extract && isValidForExtract) std::filesystem::create_directories(extractPath);
                } else if (header.typeflag == '1') { // hard
                    std::filesystem::path linkPath = header.linkname;
                    hardLinks.insert({inTarPath, linkPath.lexically_normal()});
                } else if (header.typeflag == '2') { // soft
                    // soft link can link to a folder
                    std::filesystem::path linkPath = header.linkname;
                    softLinks.insert({inTarPath, linkPath.lexically_normal()});
                } else {
                    std::streampos pos = header.size + dataBlockOffset;
                    if (throwOnUnsupported) {
                        throw std::runtime_error(
                            "Tar has an unsuppoted entry type (TODO - not "
                            "implemented): " +
                            header.name
                        );
                    }
                }
                inputStream.seekg(seekAfterEntry);

                if (!inputStream) { throw std::runtime_error("Tar filename-entry with illegal size: " + header.name); }
            }

            if (!extract) return;

            for (auto& hardLink : hardLinks) {
                std::filesystem::path extractPath;
                bool isValidForExtract;
                std::tie(isValidForExtract, extractPath) = changeRoot(hardLink.first, source, destination);
                if (!isValidForExtract) continue;

                if (!extractHardLinksAsCopies) {
                    auto [isValid, localPath] = changeRoot(hardLink.second, source, destination);
                    if (isValid) {
                        try {
                            std::filesystem::create_hard_link(localPath, extractPath);
                        } catch (...) {}
                        continue; // skip the copy code then.
                    }
                }

                estd::isubstream sourceFile = open(hardLink.second);
                std::ofstream destinationFile = std::ofstream(extractPath, std::ios::out | std::ios::binary);
                destinationFile << sourceFile.rdbuf();
                destinationFile.close();
            }

            for (auto& softLink : softLinks) {
                std::filesystem::path extractPath;
                std::filesystem::path sourcePath = softLink.first;
                std::filesystem::path linkedPath = softLink.second;
                bool isValidForExtract;
                std::tie(isValidForExtract, extractPath) = changeRoot(sourcePath, source, destination);
                if (!isValidForExtract) continue;


                std::filesystem::path rootLinkedPath = (sourcePath.parent_path() / linkedPath).lexically_normal();
                if (!extractSoftLinksAsCopies) {
                    auto [isValid, _] = changeRoot(rootLinkedPath, source, destination);
                    if (isValid || !throwOnBrokenSoftlinks) {
                        try {
                            std::filesystem::create_symlink(linkedPath, extractPath);
                        } catch (...) {
                            throw std::runtime_error("Link " + sourcePath.string() + " symlink could not be made");
                        }
                        continue;
                    }
                }

                linkedPath = rootLinkedPath;

                if (!linkedPath.has_filename() || linkedPath.filename() == ".") { // is dir
                    std::set<std::string> visited = {sourcePath.lexically_normal()};
                    extractDir(linkedPath, extractPath, visited);
                    continue;
                }

                bool storeState = followSoftlinks;
                followSoftlinks = true;
                estd::isubstream sourceFile;
                try {
                    sourceFile = open(linkedPath);
                } catch (...) {
                    if (throwOnBrokenSoftlinks) {
                        throw std::runtime_error("Link " + sourcePath.string() + " is broken in the archive");
                    } else {
                        try {
                            if(skipOnBrokenSoftlinks) continue;
                            std::filesystem::create_symlink(softLink.second, extractPath);
                            continue;
                        } catch (...) {
                            throw std::runtime_error("Link " + sourcePath.string() + " symlink could not be made");
                        }
                    }
                }
                followSoftlinks = storeState;

                std::ofstream destinationFile = std::ofstream(extractPath, std::ios::out | std::ios::binary);
                destinationFile << sourceFile.rdbuf();
                destinationFile.close();
            }
        }

        // This is only used for softlinks
        void extractDir(
            std::filesystem::path source, std::filesystem::path destination, std::set<std::string>& visited
        ) {
            for (std::filesystem::path inTarPath : paths) {
                auto [isValidForExtract, extractPath] = changeRoot(inTarPath, source, destination);
                if (!isValidForExtract) continue;

                if (!inTarPath.has_filename()) std::filesystem::create_directories(extractPath);

                estd::isubstream sourceFile;

                if (softLinks.count(inTarPath)) {
                    //if softlink is a dir; recurse
                    std::filesystem::path linksTo = softLinks[inTarPath];
                    linksTo = (inTarPath.parent_path() / linksTo).lexically_normal();
                    if (!linksTo.has_filename() || linksTo.filename() == ".") { // it is a dir
                        if (throwOnBrokenSoftlinks && !paths.count(linksTo))
                            throw std::runtime_error("Detected broken softlink " + inTarPath.string());

                        std::filesystem::create_directories(extractPath);
                        if (visited.count(inTarPath.lexically_normal())) {
                            if (throwOnInfiniteRecursion)
                                throw std::runtime_error(
                                    "Infinite recursion detected in symlink " + inTarPath.string()
                                );
                        } else {
                            visited.insert(inTarPath.lexically_normal());
                            extractDir(linksTo, extractPath, visited);
                        }
                        continue;
                    }
                }
                if (inTarPath.has_filename()) {
                    bool storeState = followSoftlinks;
                    followSoftlinks = true;
                    try {
                        sourceFile = open(inTarPath);
                    } catch (...) {
                        if (throwOnBrokenSoftlinks)
                            throw std::runtime_error("Link " + inTarPath.string() + " is broken in the archive");
                    }
                    followSoftlinks = storeState;

                    std::ofstream destinationFile = std::ofstream(extractPath, std::ios::out | std::ios::binary);
                    destinationFile << sourceFile.rdbuf();
                    destinationFile.close();
                }
            }
        }

        std::unique_ptr<std::ifstream> inputStreamPtr;
        std::istream& inputStream;
        std::map<std::string, estd::isubstream> files;
        std::map<std::string, std::string> hardLinks;
        std::map<std::string, std::string> softLinks;

        std::set<std::string> paths;

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
        

        Reader(std::string const& filename) :
            inputStreamPtr(std::make_unique<std::ifstream>(filename, std::ios_base::in | std::ios_base::binary)),
            inputStream(*inputStreamPtr.get()) {}

        Reader(std::istream& is) : inputStream(is) {}

        void indexFiles() { indexFiles<false>("", ""); }

        estd::isubstream open(std::filesystem::path source) {
            indexFiles();
            source = source.lexically_normal();

            if (followSoftlinks) { source = followSoftlink(source); }

            if (!files.count(source)) { // search hardlinks then
                if (!hardLinks.count(source)) {
                    throw std::runtime_error("File " + source.string() + " was not found in the archive");
                }

                std::string linksTo = hardLinks[source];

                if (!files.count(linksTo))
                    throw std::runtime_error("Hardlink " + source.string() + " is broken and does not point to a file");

                return files[linksTo];
            }
            return files[source];
        }

        void extractAll(std::filesystem::path destination) { extractPath("./", destination); }

        void extractPath(std::filesystem::path source, std::filesystem::path destination) {
            indexFiles<true>(source, destination);
        }
    };
}; // namespace tar
