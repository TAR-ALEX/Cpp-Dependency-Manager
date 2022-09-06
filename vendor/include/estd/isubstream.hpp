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

#include <estd/substreambuf.hpp>
#include <iostream>
#include <memory>

namespace estd {
	class isubstream : public std::istream {
	public:
		isubstream() : std::istream(&buffer_) {}

		isubstream(std::streambuf* buffer, int64_t start, int64_t size) :
			std::istream(&buffer_), buffer_(buffer, std::streampos(start), std::streamsize(size)) {}

		isubstream(std::streambuf& buffer, int64_t start, int64_t size) :
			std::istream(&buffer_), buffer_(&buffer, std::streampos(start), std::streamsize(size)) {}

		isubstream(std::istream& buffer, int64_t start, int64_t size) : isubstream(buffer.rdbuf(), start, size) {}

		isubstream(std::istream* buffer, int64_t start, int64_t size) : isubstream(buffer->rdbuf(), start, size) {}

		isubstream(isubstream const& other) : std::istream(&buffer_), buffer_(other.buffer_) {}

		isubstream(isubstream&& other) : std::istream(&buffer_), buffer_(other.buffer_) {}

		isubstream& operator=(const isubstream& other) {
			buffer_ = other.buffer_;
			init(&buffer_);
			return *this;
		}

		isubstream& operator=(isubstream&& other) {
			buffer_ = other.buffer_;
			init(&buffer_);
			return *this;
		}

	private:
		substreambuf buffer_;
	};
};// namespace estd
