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
#ifndef tokenizer_hpp
#define tokenizer_hpp

#include <fstream>
#include <vector>

using namespace std;

struct Token{
private:
  string ReplaceAll(string str, const string& from, const string& to) {
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
    return str;
  }

public:
  string location;
  string value;
  Token(){
    location = "";
    value = "";
  }
  Token(string s){
    location = "";
    value = s;
  }
  Token(string s, string c){
    location = c;
    value = s;
  }
  string toString(){
    return value + " //"+ location;
  }
  string getString(){
    string result = value;
    if(result.size() <= 0) return "";
    if(result[0] != '"') return result;

    result = ReplaceAll(result, "\\n", "\n");
    result = ReplaceAll(result, "\\\"", "\"");
    result = ReplaceAll(result, "\\\\", "\\");
    result = ReplaceAll(result, "\\0", "\0");
    if(result.size() < 2 || result[0] != '"' || result[result.size()-1] != '"') return "";
    return result.substr(1,result.size()-2);
  }
};

struct Tokenizer{
  vector<Token> tokenize(istream &infile);
};

#endif /* tokenizer_hpp */
