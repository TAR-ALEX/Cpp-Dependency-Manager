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

#include <filesystem>
#include <estd/string_util.h>

namespace estd {
    namespace files {


        class TmpDir {
        private:
            std::filesystem::path iPath = "";
            inline static std::string generateUniqueTempDir() {
                while (true) {
                    std::filesystem::path name = "." + estd::string_util::gen_random(10);
                    if (!std::filesystem::exists(name)) {
                        std::filesystem::create_directories(name);
                        return (std::filesystem::current_path() / name.string()).lexically_normal();
                    }
                }
            }

        public:
            TmpDir() { iPath = generateUniqueTempDir(); }
            ~TmpDir() { std::filesystem::remove_all(iPath); }

            std::filesystem::path path() { return iPath; }

            void discard() {
                for (const auto& entry : std::filesystem::directory_iterator(iPath)) std::filesystem::remove_all(entry);
            }
        };
    } // namespace files
} // namespace estd