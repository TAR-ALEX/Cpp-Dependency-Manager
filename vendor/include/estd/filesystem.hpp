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

#include <estd/ptr.hpp>
#include <estd/string_util.h>
#include <filesystem>
#include <vector>

namespace estd {
    namespace files {
        class Path : public std::filesystem::path {
        public:
            using std::filesystem::path::path;
            using std::filesystem::path::operator=;

            Path(const std::filesystem::path& p) { *this = std::filesystem::path(p); }
            Path(std::filesystem::path&& p) { *this = std::filesystem::path(p); }
            Path(const Path& p) { *this = std::filesystem::path(p); }
            Path(Path&& p) { *this = std::filesystem::path(p); }

            Path& operator=(const Path& p) { return (*this = std::filesystem::path(p)); }
            Path& operator=(Path&& p) { return (*this = std::filesystem::path(p)); }
            Path& operator=(const std::filesystem::path& p) { return *this = p.string(), *this; }
            Path& operator=(std::filesystem::path&& p) { return *this = p.string(), *this; }

            Path normalize() {
                Path tmp = this->lexically_normal();
                if (tmp == "." || tmp == "./") { tmp = ""; }
                return tmp;
            }

            // tells if this path is a parent of the passed path if(other is a tree in *this) (only for directories)
            bool contains(Path& other) {
                Path left = Path((*this) / "").normalize();
                Path right = Path(other / "").normalize();

                return estd::string_util::hasPrefix(right, left);
            }

            std::pair<Path, Path> splitPrefix() {
                size_t mid_pos = 0;
                std::string source = string();
                std::string psL = "";
                std::string psR = "";
                if ((mid_pos = source.find("/")) != std::string::npos) {
                    psL = source.substr(0, mid_pos);
                    psR = source.substr(mid_pos + 1);
                } else {
                    return {source, ""};
                }
                return {psL, psR};
            }

            std::pair<Path, Path> splitSuffix() {
                size_t mid_pos = 0;
                std::string source = string();
                std::string psL = "";
                std::string psR = "";
                if ((mid_pos = source.rfind("/")) != std::string::npos) {
                    psL = source.substr(0, mid_pos);
                    psR = source.substr(mid_pos + 1);
                } else {
                    return {"", source};
                }
                return {psL, psR};
            }

            estd::stack_ptr<Path> replacePrefix(Path from, Path to) {
                Path path = *this;

                path = ("." / path).lexically_normal();
                from = ("." / from).lexically_normal();
                to = ("." / to).lexically_normal();

                bool pathIsDir = !path.has_filename();
                bool fromIsDir = !from.has_filename();
                bool toIsDir = !to.has_filename();

                to = (to / "").lexically_normal();
                from = (from / "").lexically_normal();
                path = (path / "").lexically_normal();

                if (toIsDir && !fromIsDir) { // from is a file && to is a dir
                    to = to.replace_filename(from.parent_path().filename());
                    to = (to / "").lexically_normal();
                }

                if (from == "" || from == "." || from == "./") {
                    Path result = (to / path).lexically_normal();
                    if (!pathIsDir) return result.splitSuffix().first; // remove slash at end
                    return result;
                }

                if (!estd::string_util::hasPrefix(path, from)) return nullptr;
                Path result = estd::string_util::replacePrefix(path, from, to);

                result = result.lexically_normal();
                if (!pathIsDir) result = result.splitSuffix().first.lexically_normal();

                return result;
            }
        };
        // sample error:
        // filesystem error: cannot copy: No such file or directory [...] [...]


        class TmpDir {
        private:
            std::filesystem::path iPath = "";
            inline static std::string generateUniqueTempDir(std::filesystem::path root) {
                while (true) {
                    std::filesystem::path name = "." + estd::string_util::gen_random(10);
                    name = root / name;
                    if (!std::filesystem::exists(name)) {
                        std::filesystem::create_directories(name);
                        return name.lexically_normal();
                    }
                }
            }

        public:
            TmpDir(std::filesystem::path root) { iPath = generateUniqueTempDir(root); }
            TmpDir() { iPath = generateUniqueTempDir(std::filesystem::current_path()); }
            ~TmpDir() { std::filesystem::remove_all(iPath); }

            std::filesystem::path path() { return iPath; }

            void discard() {
                for (const auto& entry : std::filesystem::directory_iterator(iPath)) std::filesystem::remove_all(entry);
            }
        };

        template <bool recursive = true, bool overwrite = true>
        void copy(Path from, Path to) {
            if (!std::filesystem::is_directory(from)) {
                std::filesystem::copy_file(from, to, std::filesystem::copy_options::overwrite_existing);
            } else {
                if (!std::filesystem::exists(to) || overwrite) {
                    std::filesystem::copy_file(from, to, std::filesystem::copy_options::overwrite_existing);
                }
            }
        }

        // returns the files that could not be written to
        template <bool recursive = true>
        void copyNoOverwrite(Path from, Path to, std::vector<Path>& visited) {
            if (!std::filesystem::is_directory(from)) {
                if (!std::filesystem::exists(to)) {
                    std::filesystem::copy_file(from, to);
                } else {
                    visited.push_back(to);
                }
            } else {
                std::filesystem::create_directories(to);
                if constexpr (recursive) {
                    for (const auto& entry : std::filesystem::directory_iterator(from)) {
                        Path dirname = Path(entry.path()).splitSuffix().second;

                        Path toF = to / dirname;
                        Path fromF = entry.path();

                        copyNoOverwrite<recursive>(fromF, toF, visited);
                    }
                }
            }
        }

        // returns the files that could not be written to
        template <bool recursive = true>
        std::vector<Path> copyNoOverwrite(Path from, Path to) {
            std::vector<Path> visited = {};
            copyNoOverwrite<recursive>(from, to, visited);
            return visited;
        }

        // Path findLargestCommonPrefix(Path p1, Path p2) {
        //     p1 = p1.lexically_normal();
        //     p2 = p2.lexically_normal();
        //     if (p1 == "./" || p1 == "." || p1 == "" || p2 == "./" || p2 == "." || p2 == "") return ".";

        //     return "";
        // }
    } // namespace files
} // namespace estd
