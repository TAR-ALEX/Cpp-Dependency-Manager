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


#include <iostream>
#include <fstream>
#include <filesystem>
#include "Tokenizer.hpp"

using namespace std;
namespace fs = std::filesystem;

int main(){
    const auto root = fs::current_path();
    
    Tokenizer tkn;
    ifstream configFile("vendor.txt");
    auto tokens = tkn.tokenize(configFile);
    fs::remove_all(root / "tmp");
    for(int i = 0; i < (int)tokens.size() - 5; i += 6){
        string sourceType = tokens[i+0].getString();
        string sourceUrl = tokens[i+1].getString();
        string sourceHash = tokens[i+2].getString();
        string source = tokens[i+3].getString();
        string destination = tokens[i+4].getString();
        if(tokens[i+5].getString() != ","){
            cerr << "[WARNING] missing separator after " + tokens[i+4].toString() << endl;
        }

        string gitCall = "git clone '";
        gitCall += sourceUrl;
        gitCall+= "' ";
        gitCall+= "-b ";
        gitCall+= sourceHash;
        gitCall+= " '";;
        gitCall+=(root / "tmp").string();
        gitCall+= "'";

        cout << gitCall << endl;
        if(system(gitCall.c_str()) != 0){
            cout << "git clone for " << sourceUrl << " returned a non zero exit code\n";
        }

        const auto src = root/"tmp"/source;
        const auto target = root/destination;

        cout << src.c_str() << endl << target.c_str() << endl;
        
        if(fs::is_directory(source)){
            fs::create_directories(target);
        }else{
            fs::create_directories(target.parent_path());
        }

        fs::copy(src, target, fs::copy_options::overwrite_existing | fs::copy_options::recursive);
        cout << endl;
        fs::remove_all(root / "tmp");
    }
}