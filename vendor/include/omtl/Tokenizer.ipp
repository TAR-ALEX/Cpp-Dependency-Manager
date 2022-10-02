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

namespace {
    bool oddBackslashes(std::string s) {
        bool result = false;
        for (auto i = s.size() - 1; i >= 0; i--) {
            if (s[i] == '\\') {
                result = !result;
            } else {
                return result;
            }
        }
        return result;
    }
}; // namespace

std::string Token::getDiagnosticString() {
    using namespace estd::string_util;
    std::map<uint8_t, std::string> strfy = {
        {Token::string, "string"},
        {Token::comment, "comment"},
        {Token::name, "name"},
        {Token::number, "number"},
    };

    return strfy[dataType] + "\t" + location + "\t" + rawValue + ""; //escape_string
}

std::vector<Token> Tokenizer::tokenize(std::string filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::in);
    if (file.fail()) throw std::runtime_error("File " + filename + " does not exist");
    return tokenize(file, filename);
}

std::vector<Token> Tokenizer::tokenize(std::istream& infile, std::string filename) {
    char c;
    bool inString = false;
    bool inComment = false;
    std::vector<Token> tokens;
    Token token;
    int lineNumber = 1;
    int charachterNumber = 0;

    const auto updateLocation = [&]() {
        std::stringstream ss;
        if (filename != "") {
            ss << "(file: " << filename << " ";
        } else {
            ss << "(";
        }
        ss << "line: " << lineNumber << " column: " << charachterNumber << ")";
        token.location = ss.str();
    };

    const auto insertToken = [&](Token t) {
        if (t.dataType == Token::name) {
            try {
                estd::BigDec tst = t.rawValue;          // if we can parse as a number
                t.dataType = Token::number;             // it is a number
            } catch (...) { t.dataType = Token::name; } // failed to parse, not a number
        }
        tokens.push_back(t);
        token.location = "";
        token.rawValue = "";
        token.paddingBefore = "";
        token.paddingAfter = "";
        token.dataType = Token::name;
    };

    while (infile >> std::noskipws >> c) {
        if (c == '\n') {
            charachterNumber = 0;
            lineNumber++;
        } else {
            charachterNumber++;
        }

        if (inString) {
            token.dataType = Token::string;
            if ((c == '\"') && token.rawValue[token.rawValue.size() - 1] != '\\' && !oddBackslashes(token.rawValue)) {
                token.rawValue += c;
                if (token.rawValue != "") insertToken(token);
                inString = false;
            } else {
                token.rawValue += c;
            }
        } else if (inComment) {
            token.dataType = Token::comment;
            if (c == ')') {
                token.rawValue += c;
                if (token.rawValue != "") insertToken(token);
                inComment = false;
            } else {
                token.rawValue += c;
            }
        } else {
            token.dataType = Token::name;
            if (estd::string_util::isWhitespace(c)) {
                if (token.rawValue != "") insertToken(token);
                token.paddingBefore += c;
            } else if (c == '[' || c == ']' || c == ':' || c == ',') {
                if (token.rawValue != "") insertToken(token);
                updateLocation();
                token.rawValue = c;
                insertToken(token);
            } else if (c == '\"') {
                if (token.rawValue != "") insertToken(token);
                updateLocation();
                token.rawValue = "";
                token.rawValue += c;
                inString = true;
            } else if (c == '(') {
                if (token.rawValue != "") insertToken(token);
                updateLocation();
                token.rawValue = "";
                token.rawValue += c;
                inComment = true;
            } else {
                if (token.location == "") updateLocation();
                token.rawValue += c;
            }
        }
    }
    if (token.rawValue != "") insertToken(token);


    if (storeCommentsAsPadding) {
        std::vector<Token> tokensNoComments;
        std::string lastPadding = "";

        for (size_t i = 0; i < tokens.size(); i++) {
            Token t = tokens[i];
            t.paddingBefore = lastPadding + t.paddingBefore;
            lastPadding = "";
            if (t.dataType != Token::comment) tokensNoComments.push_back(t);
            else { lastPadding = t.paddingBefore + t.rawValue + t.paddingAfter; }
        }

        if (!tokensNoComments.empty()) tokensNoComments.back().paddingAfter = lastPadding;

        return tokensNoComments;
    }

    return tokens;
}

std::string Tokenizer::reconstruct(std::vector<Token>& tokens) {
    std::ostringstream ss;
    for (Token tmp : tokens) { ss << tmp.paddingBefore << tmp.rawValue << tmp.paddingAfter; }
    return ss.str();
}

std::string Token::getString() {
    using namespace estd::string_util;
    if (dataType != Token::string || rawValue.size() < 2)
        throw std::runtime_error("Token is not a string at " + location);
    return unescape_string(rawValue.substr(1, rawValue.size() - 2));
    // TODO: replace unescape with language specific unescape, for now use C++
    // strings limited to octal escapes and standard escapes such as \n
}

std::string Token::getEscapedString() {
    if (dataType != Token::string || rawValue.size() < 2)
        throw std::runtime_error("Token is not a string at " + location);
    return rawValue.substr(1, rawValue.size() - 2);
}

std::string Token::getComment() {
    if (dataType != Token::comment || rawValue.size() < 2)
        throw std::runtime_error("Token is not a comment at " + location);
    return rawValue.substr(1, rawValue.size() - 2);
}

std::string Token::getName() {
    if (dataType != Token::name) throw std::runtime_error("Token is not a name at " + location);
    return rawValue;
}

estd::BigDec Token::getNumber() {
    if (dataType != Token::number) throw std::runtime_error("Token is not a number at " + location);
    return estd::BigDec(rawValue);
}

std::string Token::getRaw() { return rawValue; }

std::string Token::getValue() {
    if (isName()) {
        return getName();
    } else if (isString()) {
        return getString();
    } else if (isNumber()) {
        return estd::BigDec(rawValue).toString(); // normalize the format this way so 0.00 will be 0
    } else {
        throw std::runtime_error("Token is not a value at " + location);
    }
}