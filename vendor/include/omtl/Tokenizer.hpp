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

// OMTL - Object Message Tuple List

#pragma once

#include <estd/BigNumbers.h>
#include <estd/string_util.h>
#include <fstream>
#include <map>
#include <omtl/Tokenizer.hpp>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace omtl {
    class Token {
    private:
        friend class Tokenizer;
        inline const static uint8_t name = 0;
        inline const static uint8_t string = 1;
        inline const static uint8_t number = 2;
        inline const static uint8_t comment = 3;

    public:
        std::string paddingBefore = "";
        std::string location = "";
        std::string rawValue = "";
        uint8_t dataType = Token::name;
        std::string paddingAfter = "";

        Token() {}
        Token(std::string s) {
            rawValue = s;
            dataType = Token::name;
            if (rawValue.size() >= 2 && rawValue[0] == '\"' && rawValue[1] == '\"') dataType = Token::string;
            if (rawValue.size() >= 2 && rawValue[0] == '(' && rawValue[1] == ')') dataType = Token::comment;
            if (dataType == Token::name) {
                try {
                    estd::BigDec tst = rawValue;          // if we can parse as a number
                    dataType = Token::number;             // it is a number
                } catch (...) { dataType = Token::name; } // failed to parse, not a number
            }
        }
        Token(std::string s, std::string c) : Token(s) { location = c; }

        std::string getDiagnosticString();
        std::string getRaw();
        std::string getValue();

        bool isString() { return dataType == Token::string; }
        bool isComment() { return dataType == Token::comment; }
        bool isName() { return dataType == Token::name; }
        bool isNumber() { return dataType == Token::number; }
        bool isValue() { return isString() || isNumber() || isName(); }

        std::string getString();
        std::string getEscapedString();
        std::string getComment();
        std::string getName();
        estd::BigDec getNumber();
    };

    class Tokenizer {
    public:
        bool storeCommentsAsPadding = true;
        std::vector<Token> tokenize(std::istream& infile, std::string filename = "");
        std::vector<Token> tokenize(std::string filename);
        std::string reconstruct(std::vector<Token>& tokens);
    };

#include <omtl/Tokenizer.ipp>

} // namespace omtl