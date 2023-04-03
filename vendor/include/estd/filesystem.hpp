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

#include <cstdio>
#include <estd/ptr.hpp>
#include <estd/string_util.h>
#include <exception>
#include <filesystem>
#include <functional>
#include <set>
#include <vector>

namespace estd {
    namespace files {
        class FileException : public std::runtime_error {
        public:
            using std::runtime_error::runtime_error;
        };
        class Path {
        private:
            std::string path;

            void winToUnixPath() {
                //if windows
                path = estd::string_util::replace_all(path, "\\", "/");
            }

        public:
            Path() noexcept {}
            Path(const Path& p) = default;
            Path(Path&& p) = default;
            Path(std::filesystem::path&& source) {
                path = source.string();
                winToUnixPath();
            }
            template <class Source>
            Path(const Source& source) {
                path = source;
                winToUnixPath();
            }
            Path(std::string source) {
                path = source;
                winToUnixPath();
            }
            Path(const char* source) {
                path = source;
                winToUnixPath();
            }
            Path(const std::filesystem::path& source) {
                path = source.string();
                winToUnixPath();
            }

            Path& operator=(const Path& p) = default;
            Path& operator=(Path&& p) = default;

            bool operator==(Path&& other) const { return path == other.string(); }
            bool operator==(const Path& other) const { return path == other.string(); }
#if __cplusplus < 202002L
            bool operator!=(Path&& other) const { return path != other.string(); }
            bool operator!=(const Path& other) const { return path != other.string(); }
#endif

            friend Path operator/(const Path& lhs, const Path& rhs) { return Path(lhs.string() + "/" + rhs.string()); }
            friend Path operator+(const Path& lhs, const Path& rhs) { return Path(lhs.string() + rhs.string()); }
            friend Path& operator/=(Path& lhs, const Path& rhs) { return lhs.path += "/" + rhs.string(), lhs; }
            friend Path& operator+=(Path& lhs, const Path& rhs) { return lhs.path += rhs.string(), lhs; }

            friend std::ostream& operator<<(std::ostream& stream, Path p) {
                return stream << "\"" << p.string() << "\"";
            }
            friend bool operator<(const Path left, const Path right) { return left.string() < right.string(); }
            friend bool operator>(const Path left, const Path right) { return left.string() > right.string(); }
            friend bool operator<=(const Path left, const Path right) { return left.string() <= right.string(); }
            friend bool operator>=(const Path left, const Path right) { return left.string() >= right.string(); }
            // friend bool operator==(const Path left, const Path right) { return left.string() == right.string(); }
            // friend bool operator!=(const Path left, const Path right) { return left.string() != right.string(); }

            operator std::filesystem::path() const { return std::filesystem::path(path); }
            operator std::string() const { return path; }
            // operator const char*() const { return path.c_str(); }


            std::string string() const noexcept { return path; }

            Path normalize() {
                Path tmp = std::filesystem::path(path).lexically_normal();
                if (tmp == "" || tmp == "." || tmp == "./") { tmp = "./"; }
                return tmp;
            }

            Path normalizeSafe() = delete; // TODO: Not implemented yet

            // tells if this path is a parent of the passed path if(other is a tree in *this) (only for directories)
            bool contains(Path& other) { // TODO: cover edge cases
                Path left = Path((*this) / "").normalize();
                Path right = Path(other / "").normalize();
                if (left == "./") { return true; }

                return estd::string_util::hasPrefix(right, left);
            }

            bool isFile() { return hasSuffix(); }
            bool isDirectory() { return !hasSuffix(); }
            Path addEmptySuffix() { return hasSuffix() ? *this / "" : *this; }
            Path addEmptyPrefix() { return hasPrefix() ? "" / *this : *this; }
            Path removeEmptySuffix() { return hasSuffix() ? *this : splitSuffix().first; }
            Path removeEmptyPrefix() { return hasPrefix() ? *this : splitPrefix().second; }
            bool hasPrefix() { return splitPrefix().first != ""; }
            bool hasSuffix() { return splitSuffix().second != ""; }
            Path getPrefix() { return splitPrefix().first; }
            Path getSuffix() { return splitSuffix().second; }
            bool hasAntiPrefix() { return splitPrefix().second != ""; }
            bool hasAntiSuffix() { return splitSuffix().first != ""; }
            Path getAntiPrefix() { return splitPrefix().second; }
            Path getAntiSuffix() { return splitSuffix().first; }
            Path replaceSuffix(Path s) { return splitSuffix().first / s; }
            Path replacePrefix(Path s) { return s / splitPrefix().second; }

            bool hasExtention() { return splitLongExtention().second != ""; }
            std::string getExtention() { return splitExtention().second; }
            Path replaceExtention(std::string s) { return splitExtention().first + "." + s; }

            std::string getLongExtention() { return splitLongExtention().second; }
            Path replaceLongExtention(std::string s) { return splitLongExtention().first + "." + s; }


            std::pair<Path, std::string> splitExtention() {
                size_t mid_pos = 0;
                std::string source = getSuffix();
                std::string psL = "";
                std::string psR = "";
                bool isHidden = false;
                if (isDirectory()) return {string(), ""};
                if (estd::string_util::hasPrefix(source, ".")) {
                    source = source.substr(1);
                    isHidden = true;
                }
                if ((mid_pos = source.rfind(".")) != std::string::npos) {
                    psL = source.substr(0, mid_pos);
                    psR = source.substr(mid_pos);
                } else {
                    return {string(), ""};
                }
                if (isHidden) psL = "." + psL;
                if (estd::string_util::contains(string(), "/")) psL = getAntiSuffix() / psL;
                return {psL, psR};
            }


            std::pair<Path, std::string> splitLongExtention() {
                size_t mid_pos = 0;
                std::string source = getSuffix();
                std::string psL = "";
                std::string psR = "";
                bool isHidden = false;
                if (isDirectory()) return {string(), ""};
                if (estd::string_util::hasPrefix(source, ".")) {
                    source = source.substr(1);
                    isHidden = true;
                }
                if ((mid_pos = source.find(".")) != std::string::npos) {
                    psL = source.substr(0, mid_pos);
                    psR = source.substr(mid_pos);
                } else {
                    return {string(), ""};
                }
                if (isHidden) psL = "." + psL;
                if (estd::string_util::contains(string(), "/")) psL = getAntiSuffix() / psL;
                return {psL, psR};
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

                // path = ("" / path).normalize();
                // from = ("" / from).normalize();
                // to = ("" / to).normalize();

                bool pathIsDir = !path.hasSuffix();
                bool fromIsDir = !from.hasSuffix();
                bool toIsDir = !to.hasSuffix();

                to = (to / "").normalize();
                from = (from / "").normalize();
                path = (path / "").normalize();

                if (toIsDir && !fromIsDir) { // from is a file && to is a dir
                    to = to.replaceSuffix(from.splitSuffix().first.getSuffix());
                    to = (to / "").normalize();
                }

                if (from == "" || from == "." || from == "./") {
                    Path result = (to / path).normalize();
                    if (!pathIsDir) return result.splitSuffix().first; // remove slash at end
                    return result;
                }

                if (!estd::string_util::hasPrefix(path, from)) return nullptr;
                Path result = estd::string_util::replacePrefix(path, from, to);

                result = result.normalize();
                if (!pathIsDir) result = result.splitSuffix().first.normalize();

                return result;
            }
        };

        namespace {
            void throwError(std::string description, Path* dir1 = nullptr, Path* dir2 = nullptr) {
                if (dir2 != nullptr && dir1 != nullptr) {
                    throw estd::files::FileException(
                        std::string("filesystem error: ") + description + " [" + dir1->string() + "]" + " [" +
                        dir2->string() + "]"
                    );
                } else if (dir1 != nullptr) {
                    throw estd::files::FileException(
                        std::string("filesystem error: ") + description + " [" + dir1->string() + "]"
                    );
                } else {
                    throw estd::files::FileException(std::string("filesystem error: ") + description);
                }
            };
        } // namespace

        inline bool isDirectory(Path p);
        inline Path followSoftLink(Path p);
        inline bool isSoftLink(Path p);
        inline bool isBlockFile(Path p);
        inline bool isCharacterFile(Path p);
        inline bool isEmptry(Path p);
        inline bool isFIFO(Path p);
        inline bool isOther(Path p);
        inline bool isFile(Path p);

        enum CopyOptions : uint64_t {
            none = 0,

            skipExisting = 1 << 0,
            overwriteExisting = 1 << 1,
            updateExisting = 1 << 2,

            recursive = 1 << 3,

            softLinksAsCopies = 1 << 4,
            skipSoftLinks = 1 << 5,
            directoriesOnly = 1 << 6,

            copyAsSoftLinks = 1 << 7,
            copyAsHardLinks = 1 << 8,
            overwriteReadonly = 1 << 9
        };

        class DirectoryEntry : public std::filesystem::directory_entry {
        public:
            using std::filesystem::directory_entry::directory_entry;
            Path path() const noexcept { return std::filesystem::directory_entry::path(); }
            operator Path() const noexcept { return std::filesystem::directory_entry::path(); }
            void assign(const Path& p) { std::filesystem::directory_entry::assign(p); }
        };

        class DirectoryIterator : public std::filesystem::directory_iterator {
            mutable DirectoryEntry e;

        public:
            using std::filesystem::directory_iterator::directory_iterator;
            friend inline DirectoryIterator begin(DirectoryIterator iter) noexcept { return iter; }
            friend inline DirectoryIterator end(DirectoryIterator) noexcept { return DirectoryIterator(); }
            const DirectoryEntry& operator*() const noexcept {
                e = DirectoryEntry(std::filesystem::directory_iterator::operator*());

                if (e.is_directory()) {
                    e.assign(e.path().addEmptySuffix());
                    return e;
                } else if (e.is_symlink()) {
                    if (isDirectory(e.path())) {
                        e.assign(e.path().addEmptySuffix());
                        return e;
                    } else {
                        e.assign(e.path().removeEmptySuffix());
                        return e;
                    }
                }
                e.assign(Path(e.path()).removeEmptySuffix());
                return e;
            }
            const DirectoryEntry* operator->() const noexcept { return &*(*this); }
            DirectoryIterator& operator++() {
                std::filesystem::directory_iterator::operator++();
                return *this;
            }
            DirectoryIterator& increment(std::error_code& ec) {
                std::filesystem::directory_iterator::increment(ec);
                return *this;
            }

            DirectoryIterator operator++(int) {
                DirectoryIterator old{**this};
                ++*this;
                return old;
            }
        };
        class RecursiveDirectoryIterator : public std::filesystem::recursive_directory_iterator {
            mutable DirectoryEntry e;

        public:
            using std::filesystem::recursive_directory_iterator::recursive_directory_iterator;
            friend inline RecursiveDirectoryIterator begin(RecursiveDirectoryIterator iter) noexcept { return iter; }
            friend inline RecursiveDirectoryIterator end(RecursiveDirectoryIterator) noexcept {
                return RecursiveDirectoryIterator();
            }
            const DirectoryEntry& operator*() const noexcept {
                e = DirectoryEntry(std::filesystem::recursive_directory_iterator::operator*());
                if (e.is_directory()) {
                    e.assign(e.path().addEmptySuffix());
                    return e;
                } else if (e.is_symlink()) {
                    if (isDirectory(e.path())) {
                        e.assign(e.path().addEmptySuffix());
                        return e;
                    } else {
                        e.assign(e.path().removeEmptySuffix());
                        return e;
                    }
                }
                e.assign(Path(e.path()).removeEmptySuffix());
                return e;
            }
            const DirectoryEntry* operator->() const noexcept { return &*(*this); }
            RecursiveDirectoryIterator& operator++() {
                return std::filesystem::recursive_directory_iterator::operator++(), *this;
            }
            RecursiveDirectoryIterator& increment(std::error_code& ec) {
                return std::filesystem::recursive_directory_iterator::increment(ec), *this;
            }
            RecursiveDirectoryIterator operator++(int) {
                RecursiveDirectoryIterator old{**this};
                ++*this;
                return old;
            }
        };

        typedef std::filesystem::perms Permissions;

        using FileTime = std::filesystem::file_time_type;
        inline Permissions getPermissions(Path& p) { return std::filesystem::status(p).permissions(); }
        template <class T>
        inline void setPermissions(Path& path, T perm) {
            std::filesystem::permissions(path, Permissions(perm));
        }
        inline Path currentPath() { return std::filesystem::current_path(); }
        inline void copy(Path from, Path to, const uint64_t opt = CopyOptions::recursive);

        inline bool exists(Path p) {
            return std::filesystem::exists(p) || std::filesystem::is_symlink(p);
        } // standards version returns false on broken symlink if symlink exists (strange)
        inline uintmax_t remove(Path p) { return std::filesystem::remove_all(p); }
        inline bool isDirectory(Path p) { return std::filesystem::is_directory(p); }
        inline Path followSoftLink(Path p) { return std::filesystem::read_symlink(p); }
        inline bool isSoftLink(Path p) { return std::filesystem::is_symlink(p); }
        inline bool isBlockFile(Path p) { return std::filesystem::is_block_file(p); }
        inline bool isCharacterFile(Path p) { return std::filesystem::is_character_file(p); }
        inline bool isEmptry(Path p) { return std::filesystem::is_empty(p); }
        inline bool isFIFO(Path p) { return std::filesystem::is_fifo(p); }
        inline bool isOther(Path p) { return std::filesystem::is_other(p); }
        inline bool isFile(Path p) { return std::filesystem::is_regular_file(p); }

        // returns if it is a directory or a softlink to a directory
        inline bool isSoftDirectory(Path p) {
            std::function<bool(Path, std::set<Path>&)> iSD;
            iSD = [&iSD](Path p, std::set<Path>& visited) {
                if (visited.count(p)) return false;
                visited.insert(p);

                if (isFile(p)) return true;
                if (isSoftLink(p)) {
                    Path link = followSoftLink(p);
                    if (exists(link)) return iSD(p, visited);
                    return link.hasSuffix();
                }
                return false;
            };
            std::set<Path> visited;
            return iSD(p, visited);
        }

        inline bool isSoftFile(Path p) {
            std::function<bool(Path, std::set<Path>&)> iSF;
            iSF = [&iSF](Path p, std::set<Path>& visited) {
                if (visited.count(p)) return false;
                visited.insert(p);

                if (isFile(p)) return true;
                if (isSoftLink(p)) {
                    Path link = followSoftLink(p);
                    if (exists(link)) return iSF(p, visited);
                    return link.hasSuffix();
                }
                return false;
            };
            std::set<Path> visited;
            return iSF(p, visited);
        }

        inline bool isSocket(Path p) { return std::filesystem::is_socket(p); }
        inline void createHardLink(Path from, Path to) { return std::filesystem::create_hard_link(from, to); }
        inline void createSoftLink(Path from, Path to) {
            Path linkroot = to.removeEmptySuffix().splitSuffix().first;
            to = to.removeEmptySuffix();
            from = std::filesystem::relative(from, linkroot);
            std::filesystem::create_symlink(from, to);
        }
        //from path will be relative (the way it is in the OS)
        inline void createSoftLinkRelative(Path from, Path to) {
            to = to.removeEmptySuffix();
            std::filesystem::create_symlink(from, to);
        }

        inline void createDirectories(Path p) { std::filesystem::create_directories(p); }
        inline void createDirectory(Path p) { std::filesystem::create_directory(p); }

        inline FileTime getModificationTime(Path p) { return std::filesystem::last_write_time(p); }
        inline void setModificationTime(Path p, FileTime n) { std::filesystem::last_write_time(p, n); }

        inline void copySoftLink(Path from, Path to, const uint64_t opt = CopyOptions::none) {
            if (!isSoftLink(from)) throwError("copySoftLink: not a softlink", &from);
            if (opt & CopyOptions::updateExisting) {
                if (exists(to)) {
                    auto newTime = getModificationTime(from);
                    auto oldTime = getModificationTime(to);
                    if (newTime > oldTime) {
                        remove(to);
                        std::filesystem::copy_symlink(from, to);
                    }
                } else {
                    std::filesystem::copy_symlink(from, to);
                }
            } else if (opt & CopyOptions::overwriteExisting) {
                if (exists(to)) remove(to);
                std::filesystem::copy_symlink(from, to);
            } else if (opt & CopyOptions::skipExisting) {
                if (!exists(to)) std::filesystem::copy_symlink(from, to);
            } else {
                if (!exists(to)) {
                    std::filesystem::copy_symlink(from, to);
                } else {
                    throwError("copySoftLink cannot copy, entry exists", &to);
                }
            }
        }
        inline void copyDirectory(Path from, Path to, const uint64_t opt = CopyOptions::recursive) {
            if (from.isFile()) {
                throwError("copyDirectory cannot copy, from is not a directory", &from);
            } else if (to.isFile()) {
                throwError("copyDirectory cannot copy, to is not a directory", &to);
            }

            if (!exists(from)) throwError("copyDirectory trying to copy a directory that does not exist", &from);
            if (!isDirectory(from)) throwError("copyDirectory trying to copy a file", &from);

            if (exists(to) && !isDirectory(to)) { throwError("copyDirectory trying to copy to a file", &to); }

            if (opt & CopyOptions::updateExisting) {
                auto newTime = getModificationTime(from);
                auto oldTime = getModificationTime(to);
                if (newTime > oldTime) {
                    if (exists(to) && !isDirectory(to)) remove(to);
                    createDirectories(to); // copy_dir does not work
                    setModificationTime(to, newTime);
                }
            } else if (opt & CopyOptions::overwriteExisting) {
                auto newTime = getModificationTime(from);
                if (exists(to) && !isDirectory(to)) remove(to);
                createDirectories(to); // copy_dir does not work
                setModificationTime(to, newTime);
            } else if (opt & CopyOptions::skipExisting) {
                if (!exists(to)) createDirectories(to); // copy_dir does not work
            } else {
                if (!exists(to)) {
                    createDirectories(to); // copy_dir does not work
                } else {
                    throwError("copyDirectory cannot copy, entry exists", &to);
                }
            }

            if (!isDirectory(to)) return; // do not copy sub files to a file

            // dir has been copied

            if (!(opt & CopyOptions::recursive)) return;

            estd::stack_ptr<std::runtime_error> err; // do not abort on a single error
            for (auto e : DirectoryIterator(from)) {
                try {
                    Path fromE = e.path();
                    Path toE = fromE.replacePrefix(from, to).value();

                    copy(fromE, toE, opt);
                } catch (std::exception& tmp) { err = std::runtime_error(tmp.what()); }
            }
            if (err) throw err.value();
        }
        inline void copyFile(Path from, Path to, const uint64_t opt = CopyOptions::none) {
            if (from.isFile() && to.isDirectory()) {
                copyFile(from, to / from.getSuffix(), opt);
                return;
            }
            if (from.isDirectory()) throwError("copyFile cannot copy a directory", &from);
            // At this point we can assume that from is a file and to is also a file (in terms of paths)

            using sco = std::filesystem::copy_options;
            sco sopt = sco::none;

            if (opt & CopyOptions::overwriteExisting) {
                sopt |= sco::overwrite_existing;
            } else if (opt & CopyOptions::updateExisting) {
                sopt |= sco::update_existing;
            } else if (opt & CopyOptions::skipExisting) {
                sopt |= sco::skip_existing;
            }

            if (opt & CopyOptions::directoriesOnly) { return; }
            if (!(opt & CopyOptions::softLinksAsCopies)) { sopt |= sco::copy_symlinks; }
            if (opt & CopyOptions::copyAsHardLinks) {
                sopt |= sco::create_hard_links;
            } else if (opt & CopyOptions::copyAsSoftLinks) {
                sopt |= sco::create_symlinks;
            }

            if (isDirectory(from)) throwError("copyFile cannot copy expected a file got a directory", &from);
            if (isDirectory(to)) {
                if (opt & CopyOptions::skipExisting) {
                    return;
                } else if (opt & CopyOptions::overwriteExisting) {
                    remove(to);
                    std::filesystem::copy_file(from, to, sopt);
                    return;
                } else if (opt & CopyOptions::updateExisting) {
                    if (getModificationTime(from) < getModificationTime(to)) return;
                    remove(to);
                    std::filesystem::copy_file(from, to, sopt);
                    return;
                } else {
                    throwError("copyFile cannot copy a file to replace a directory", &from);
                }
            }

            if (CopyOptions::overwriteReadonly) remove(to);
            std::filesystem::copy_file(from, to, sopt);
        }

        inline void rename(Path from, Path to) {
            if (from.isDirectory() != isDirectory(from)) {
                if (from.isDirectory()) {
                    throwError("cannot rename: source not a directory", &from);
                } else {
                    throwError("cannot rename: source not a file", &from);
                }
            }
            if (from.isDirectory() != to.isDirectory()) {
                if (from.isDirectory()) {
                    throwError("cannot rename: destination not a directory", &from);
                } else {
                    throwError("cannot rename: destination not a file", &from);
                }
            }
            if (std::rename(from.string().c_str(), to.string().c_str()) != 0) {
                throwError("Failed to rename: source file ", &from);
            }
        }

        inline void copy(Path from, Path to, const uint64_t opt) {
            if (!exists(from.removeEmptySuffix())) throwError("cannot copy: No such file or directory", &from);

            if (from.isDirectory() != isDirectory(from)) {
                if (from.isDirectory()) {
                    throwError("cannot copy: source not a directory", &from);
                } else {
                    throwError("cannot copy: source not a file", &from);
                }
            }

            try {
                if (isSoftLink(from.removeEmptySuffix())) {
                    // std::cout << "copy_symlink(" << from << ", " << to << ")\n";
                    copySoftLink(from.removeEmptySuffix(), to.removeEmptySuffix(), opt);
                } else if (from.isFile()) { // TODO: test strange files such as sockets and blocks under this if
                    // std::cout << "copy_file(" << from << ", " << to << ")\n";
                    copyFile(from, to, opt);
                } else if (from.isDirectory()) {
                    // std::cout << "copy_dir(" << from << ", " << to << ")\n";
                    copyDirectory(from, to.addEmptySuffix(), opt);
                }
            } catch (std::exception& e) { throw std::runtime_error(e.what()); }
        }

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
                        return name.lexically_normal().string();
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

        // template <bool recursive = true, bool overwrite = true>
        // void copy(Path from, Path to) {
        //     if (!std::filesystem::is_directory(from)) {
        //         std::filesystem::copy_file(from, to, std::filesystem::copy_options::overwrite_existing);
        //     } else {
        //         if (!std::filesystem::exists(to) || overwrite) {
        //             std::filesystem::copy_file(from, to, std::filesystem::copy_options::overwrite_existing);
        //         }
        //     }
        // }

        // Path findLargestCommonPrefix(Path p1, Path p2) {
        //     p1 = p1.lexically_normal();
        //     p2 = p2.lexically_normal();
        //     if (p1 == "./" || p1 == "." || p1 == "" || p2 == "./" || p2 == "." || p2 == "") return ".";

        //     return "";
        // }
    } // namespace files
} // namespace estd
