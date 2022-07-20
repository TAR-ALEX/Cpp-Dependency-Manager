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

#include "Tokenizer.hpp"
#include <string>
#include <sstream>

bool oddBackslashes(string s) {
    bool result = false;
    for(auto i = s.size()-1; i>=0; i--){
        if(s[i] == '\\'){
            result = !result;
        } else {
            return result;
        }
    }
    return result;
}

vector<Token> Tokenizer::tokenize(istream& infile) {
    char c;
    bool inString = false;
    bool inComment = false;
    vector<Token> tokens;
    Token token;
    int lineNumber = 1;
    int charachterNumber = 0;
    while (infile >> noskipws >> c) {
        if (c == '\n'){
          charachterNumber = 0;
          lineNumber++;
        }else{
          charachterNumber++;
        }
        if (c != ' ' && c != '\n' && token.location == ""){
          stringstream ss;
          ss << lineNumber << "{" << charachterNumber << "}";
          token.location = ss.str();
        }
        
        if(inString){
            if((c == '\"') && token.value[token.value.size()-1] != '\\' && !oddBackslashes(token.value)) {
                token.value += c;
                if (token.value != "") tokens.push_back(token);
                token.location = "";
                token.value = "";
                inString = false;
            }else{
                token.value += c;
            }
        }else if(inComment){
            if (c == ')'){
                token.location = "";
                token.value = "";
                inComment = false;
            }else{
                token.value+=c;
            }
        }else{
            if (c == ' ' || c == '\n') {
                if (token.value != "") tokens.push_back(token);
                token.location = "";
                token.value = "";
            } else if (c == '[' || c == ']' || c == ':' || c == ',') {
                if (token.value != "") tokens.push_back(token);
                token.value = c;
                tokens.push_back(token);
                token.location = "";
                token.value = "";
            } else if (c == '\"'){
                if (token.value != "") tokens.push_back(token);
                stringstream ss;
                ss << lineNumber << "{" << charachterNumber << "}";
                token.location = ss.str();
                token.value = "";
                token.value += c;
                inString = true;
            } else if (c == '('){
                if (token.value != "") tokens.push_back(token);
                stringstream ss;
                ss << lineNumber << "{" << charachterNumber << "}";
                token.location = ss.str();
                token.value = "";
                token.value += c;
                inComment = true;
            } else {
                token.value+=c;
            }
        }
    }
    if (token.value != "") tokens.push_back(token);
    return tokens;
}
