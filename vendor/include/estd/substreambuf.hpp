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

#include <streambuf>

namespace estd {
	class substreambuf : public std::streambuf {
	public:
		substreambuf() : buf(nullptr), start(0), length(0), pos(0) { setbuf(nullptr, 0); }

		substreambuf(std::streambuf* sbuf, uint64_t start, uint64_t len) :
			buf(sbuf), start(start), length(len), pos(0) {
			setbuf(nullptr, 0);
		}

		substreambuf(const substreambuf& other) {
			setbuf(nullptr, 0);
			buf = other.buf;
			start = other.start;
			length = other.length;
			pos = other.pos;
		}

		substreambuf(substreambuf&& other) {
			setbuf(nullptr, 0);
			buf = other.buf;
			start = other.start;
			length = other.length;
			pos = other.pos;
		}

		substreambuf& operator=(const substreambuf& other) {
			setbuf(nullptr, 0);
			buf = other.buf;
			start = other.start;
			length = other.length;
			pos = other.pos;
			return *this;
		}

		substreambuf& operator=(substreambuf&& other) {
			setbuf(nullptr, 0);
			buf = other.buf;
			start = other.start;
			length = other.length;
			pos = other.pos;
			return *this;
		}

	protected:
		int underflow() {
			if (pos >= length) return traits_type::eof();
			seekoff(std::streamoff(0), std::ios_base::cur);
			return buf->sgetc();
		}

		int uflow() {
			if (pos > length) return traits_type::eof();
			seekoff(std::streamoff(0), std::ios_base::cur);
			pos += std::streamsize(1);
			return buf->sbumpc();
		}

		std::streampos seekoff(
			std::streamoff off,
			std::ios_base::seekdir way,
			std::ios_base::openmode which = std::ios_base::in | std::ios_base::out
		) {
			std::streampos cursor;

			if (way == std::ios_base::beg) cursor = off;
			else if (way == std::ios_base::cur)
				cursor = pos + off;
			else if (way == std::ios_base::end)
				cursor = length - off;

			if (cursor < 0 || cursor >= length) return std::streampos(-1);
			pos = cursor;
			if (buf->pubseekpos(start + pos, which) == std::streampos(-1)) return std::streampos(-1);

			return pos;
		}

		std::streampos seekpos(
			std::streampos sp, std::ios_base::openmode which = std::ios_base::in | std::ios_base::out
		) {
			if (sp < 0 || sp >= length) return std::streampos(-1);
			pos = sp;
			if (buf->pubseekpos(start + pos, which) == std::streampos(-1)) return std::streampos(-1);
			return pos;
		}

		std::streamsize xsgetn(char* s, std::streamsize n) {
			seekoff(std::streamoff(0), std::ios_base::cur);
			if (pos + n >= length) { n = length - pos; }
			pos += n;
			if (n == 0) {
				return 0;
			}
			return buf->sgetn(s, n);
		}

	private:
		std::streambuf* buf;
		std::streampos start;
		std::streamsize length;
		std::streampos pos;
	};
};// namespace estd
