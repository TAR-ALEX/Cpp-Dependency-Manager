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

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <iostream>
#include <fstream>
#include <filesystem>
#include <thread>
#include <httplib.h>
#include <bxzstr.hpp>
#include <tar/tar.hpp>
#include "Tokenizer.hpp"

using namespace std;
using namespace httplib;
namespace fs = std::filesystem;

tuple<string,string,string> splitUrl(string url){
    std::string delimiter = " ";
    std::string scheme = "";
    std::string host = "";
    std::string path = "";

    size_t pos = 0;
    std::string token;

    delimiter = "://";
    if ((pos = url.find(delimiter)) != std::string::npos) {
        token = url.substr(0, pos + delimiter.length());
        scheme = token;
        url.erase(0, pos + delimiter.length());
    }

    delimiter = "/";
    if ((pos = url.find(delimiter)) != std::string::npos) {
        token = url.substr(0, pos);
        host = token;
        url.erase(0, pos);
    }

    path = url;

    return make_tuple(scheme,host,path);
}

std::filesystem::path downloadFile(string url, std::filesystem::path location){
    std::string scheme = "";
    std::string host = "";
    std::string path = "";

    tie(scheme, host, path) = splitUrl(url);

    std::filesystem::path extractFilename = path;
    std::filesystem::path filename = extractFilename.filename();

    fs::create_directories(location);
    
    ofstream file(location/filename);

    httplib::Client cli((scheme+host).c_str());
    cli.set_follow_location(true);

    auto res = cli.Get(path.c_str(), Headers(),
    [&](const Response &response) {
        return true; // return 'false' if you want to cancel the request.
    },
    [&](const char *data, size_t data_length) {
        file.write(data, data_length);
        return true; // return 'false' if you want to cancel the request.
    });

    file.close();
    return location/filename;
}

std::vector<std::vector<Token>> splitTokens(std::vector<Token> tokens, string delimiter){
    std::vector<std::vector<Token>> result;
    std::vector<Token> current;
    for(std::size_t i = 0; i < tokens.size(); i++){
        if(tokens[i].value == delimiter){
            result.push_back(current);
            current.clear();
        }else{
            current.push_back(tokens[i]);
        }
    }
    result.push_back(current);
    current.clear();
    return result;
}

void parseGit(std::vector<Token> tokens, std::filesystem::path root){
    if(tokens.size() != 5){
        cerr << "[WARNING] not enough arguments for git statement at " + tokens[0].location << endl;
    }

    string sourceType = tokens[0].getString();
    string sourceUrl = tokens[1].getString();
    string sourceHash = tokens[2].getString();
    string source = tokens[3].getString();
    string destination = tokens[4].getString();


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

void parseTar(std::vector<Token> tokens, std::filesystem::path root){
    if(tokens.size() != 4){
        cerr << "[WARNING] not enough arguments for http statement at " + tokens[0].location << endl;
    }

    string sourceType = tokens[0].getString();
    string sourceUrl = tokens[1].getString();
    string source = tokens[2].getString();
    string destination = tokens[3].getString();

    cout << sourceUrl << endl;

    string filename = downloadFile(sourceUrl, root / "tmp");

    bxz::ifstream zFile = bxz::ifstream(filename);
    tar::Reader r(zFile);

    
    const auto target = root/destination;

    cout << source.c_str() << endl << target.c_str() << endl;
    
    r.extractPath(source, target);

    cout << endl;
    zFile.close();
    fs::remove_all(root / "tmp");
}

int main(){
    const auto root = fs::current_path();
    
    Tokenizer tkn;
    ifstream configFile("vendor.txt");
    auto statements = splitTokens(tkn.tokenize(configFile), ",");


    fs::remove_all(root / "tmp");
    for(auto tokens: statements){
        if(tokens.size() <= 0) continue;
        if(tokens[0].getString() == "git")
            parseGit(tokens, root);
        if(tokens[0].getString() == "tar")
            parseTar(tokens, root);
    }
    return 0;
}