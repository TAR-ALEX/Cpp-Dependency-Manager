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

#include <deque>
#include <estd/ostream_proxy.hpp>
#include <estd/ptr.hpp>
#include <estd/string_util.h>
#include <iostream>
#include <map>
#include <memory>
#include <omtl/ParseTree.hpp>
#include <omtl/Tokenizer.hpp>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace omtl {
    class Element {
    private:
        friend class ParseTreeBuilder;
        estd::clone_ptr<std::deque<std::pair<std::string, Element>>> tuple;
        estd::clone_ptr<std::deque<Element>> statement;
        estd::clone_ptr<Token> value;

        inline Element& getSingleElement();

    public:
        Element();
        Element(Token v);
        Element(std::deque<std::pair<std::string, Element>> t);
        Element(std::deque<Element> s);
        Element(const Element& e);
        ~Element() {}

        std::string location = "";

        std::string getDiagnosticString();

        size_t size();
        bool onlyContains(std::set<std::string> names);
        bool contains(std::string name);
        bool contains(size_t id);
        estd::clone_ptr<Element> operator[](std::string name);
        estd::clone_ptr<Element> operator[](size_t id);

        Element slice(size_t left, size_t right = SIZE_MAX);
        Element popFront();
        Element popBack();
        void popFront(size_t n);
        void popBack(size_t n);
        void pushFront(Element e);
        void pushBack(Element e);
        void pushFront(std::string n, Element e);
        void pushBack(std::string n, Element e);

        bool isTuple();
        bool isStatement();
        bool isToken();

        bool isString();
        bool isComment();
        bool isName();
        bool isNumber();
        bool isValue();
        bool isRaw();

        Token getToken();
        std::string getString();
        std::string getEscapedString();
        std::string getComment();
        std::string getName();
        estd::BigDec getNumber();
        std::string getValue();
        std::string getRaw();
    };

    class ParseTreeBuilder {
    private:
        size_t findMatchingBracket(std::vector<Token>& tokens, size_t i);
        bool isTuple(std::vector<Token>& tokens, size_t i);
        Element parseStatement(std::vector<Token>& tokens, size_t& i);
        Element parseTuple(std::vector<Token>& tokens, size_t& i);

    public:
        estd::ostream_proxy log{&std::cout};
        Element buildParseTree(std::vector<Token> vector);
    };

    inline static Element Tuple(std::deque<std::pair<std::string, Element>> in = {}) { return Element(in); }
    inline static Element Statement(std::deque<Element> in = {}) { return Element(in); }

#include <omtl/ParseTree.ipp>
} // namespace omtl