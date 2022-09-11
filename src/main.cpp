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
#include "conflict-detector.hpp"
#include "omtl/ParseTree.hpp"
#include "omtl/Tokenizer.hpp"
#include "repo-cache.hpp"
#include <bxzstr.hpp>
#include <deb-downloader.hpp>
#include <estd/filesystem.hpp>
#include <estd/ptr.hpp>
#include <filesystem>
#include <fstream>
#include <httplib.h>
#include <iostream>
#include <map>
#include <tar/tar.hpp>
#include <thread>
#include <vector>

using namespace std;
using namespace httplib;
using namespace omtl;
using namespace estd::shortnames;
using estd::files::Path;

namespace fs = std::filesystem;

cptr<deb::Installer> debInstaller;
jptr<estd::files::TmpDir> temp;
cptr<RepoCache> repoCache;

tuple<string, string, string> splitUrl(string url) {
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

    return make_tuple(scheme, host, path);
}

Path downloadFile(string url, Path location) {
    std::string scheme = "";
    std::string host = "";
    std::string path = "";

    tie(scheme, host, path) = splitUrl(url);

    fs::create_directories(location.parent_path());

    ofstream file(location);

    httplib::Client cli((scheme + host).c_str());
    cli.set_follow_location(true);

    auto res = cli.Get(
        path.c_str(),
        Headers(),
        [&](const Response& response) {
            return true; // return 'false' if you want to cancel the request.
        },
        [&](const char* data, size_t data_length) {
            file.write(data, data_length);
            return true; // return 'false' if you want to cancel the request.
        }
    );

    file.close();
    return location;
}

void parseMoveCache(Path cache, string repoId, Element tokens) {
    if (tokens.size() != 2) {
        cout << "[WARNING] not enough arguments for copy portion of statement at " + tokens.location << endl;
    }
    string source = tokens[0]->getValue();
    string destination = tokens[1]->getValue();

    const auto src = cache / source;
    const auto target = fs::current_path() / destination;

    cout << src.c_str() << endl << target.c_str() << endl;

    copyRepo(repoId, src, target);
    cout << endl;
}

Path parseAheadCommonRoot(Element tokens) {
    if (tokens.size() != 2) {
        cout << "[WARNING] not enough arguments for copy portion of statement at " + tokens.location << endl;
    }
    string source = tokens[0]->getValue();
    string destination = tokens[1]->getValue();

    return Path(source).normalize();
}

void parseGit(Element tokens) {
    if (tokens.size() < 3) { cout << "[WARNING] not enough arguments for git statement at " + tokens.location << endl; }

    string sourceUrl = tokens[1]->getValue();
    string sourceHash = tokens[2]->getValue();
    string repoId = "git " + sourceUrl + " " + sourceHash;

    Path cache = repoCache->createDir(repoId, "", [&](Path cache) {
        string gitCall = "git clone '";
        gitCall += sourceUrl;
        gitCall += "' ";
        gitCall += "-b ";
        gitCall += sourceHash;
        gitCall += " '";

        gitCall += cache.string();
        gitCall += "'";

        cout << gitCall << endl;
        if (system(gitCall.c_str()) != 0) {
            cout << "git clone for " << sourceUrl << " returned a non zero exit code\n";
        }
    });

    parseMoveCache(cache, repoId, tokens.slice(3));
}

void parseTar(Element tokens) {
    if (tokens.size() < 2) { cout << "[WARNING] not enough arguments for tar statement at " + tokens.location << endl; }

    string sourceUrl = tokens[1]->getValue();
    string repoId = "tar " + sourceUrl;

    Path common = parseAheadCommonRoot(tokens.slice(2));

    Path cache = repoCache->createDir(repoId, common, [&](Path cache) {
        string filename = repoCache->getFilePath(repoId);

        if (!std::filesystem::exists(filename)) {
            cout << "Downloading .tar package " << sourceUrl << endl;
            downloadFile(sourceUrl, repoCache->getFilePath(repoId));
        }

        bxz::ifstream zFile = bxz::ifstream(filename);
        tar::Reader r(zFile);

        cout << "Extracting .tar package " << sourceUrl << endl;


        r.extractPath(common, cache / common);

        zFile.close();
    });

    parseMoveCache(cache, repoId, tokens.slice(2));
}

void parseDebInit(Element tokens) {
    std::vector<std::string> sources;
    if (tokens.size() < 2) {
        cout << "[WARNING] not enough arguments for deb-repo statement at " + tokens.location << endl;
    }
    if (tokens[1]->isTuple()) {
        for (size_t i = 0; i < tokens[1]->size(); i++) {
            auto line = tokens[1][i];
            if (!line->isString()) {
                cout << "[WARNING] debian repository must be a string " + line->location << endl;
                continue;
            }
            sources.push_back(line->getString());
        }
        debInstaller = new deb::Installer(sources, new TmpDir(temp->path()));
    } else {
        cout << "[WARNING] bad arguments for deb-repo statement at " + tokens.location << endl;
    }
}

void parseDebRecurseDepth(Element tokens) {
    if (tokens.size() < 2) {
        cout << "[WARNING] not enough arguments for deb-recurse-limit statement at " + tokens.location << endl;
    }
    int recurseDepth = int(tokens[1]->getNumber().toInt());
    debInstaller->recursionLimit = recurseDepth;
}

void parseDebMarkInstall(Element tokens) {
    if (tokens.size() < 2) {
        cout << "[WARNING] not enough arguments for deb-ignore statement at " + tokens.location << endl;
    }
    for (size_t i = 1; i < tokens.size(); i++) { debInstaller->markPreInstalled({tokens[i]->getValue()}); }
}

void parseDebInstall(Element tokens) {
    if (tokens.size() < 2) { cout << "[WARNING] not enough arguments for deb statement at " + tokens.location << endl; }
    string repoId = "deb " + tokens[1]->getValue();

    Path common = parseAheadCommonRoot(tokens.slice(2));

    Path cache = repoCache->createDir(repoId, common, [&](Path cache) {
        cache = repoCache->access(repoId);
        cout << "Installing .deb package " << tokens[1]->getValue() << endl;
        debInstaller->install(tokens[1]->getValue(), {{common, cache / common}});
        debInstaller->clearInstalled();
    });

    parseMoveCache(cache, repoId, tokens.slice(2));
}

void parseBlock(Element pt);
void parseInclude(Element cmd) {
    if (cmd.size() != 2) throw runtime_error("invalid include command at " + cmd.location);
    Tokenizer tkn;
    ParseTreeBuilder ptb;

    auto pt = ptb.buildParseTree(tkn.tokenize(cmd[1]->getValue()));

    parseBlock(pt);
}

void parseBlock(Element pt) {
    for (size_t i = 0; i < pt.size(); i++) {
        if (pt[i]->size() <= 0) continue;

        if (!pt[i][0]->isName()) {
            cout << "[WARNING] unsupported statement at " << pt[i][0]->location << endl;
        } else if (pt[i][0]->getName() == "git") {
            parseGit(pt[i].value());
        } else if (pt[i][0]->getName() == "tar") {
            parseTar(pt[i].value());
        } else if (pt[i][0]->getName() == "deb-init") {
            parseDebInit(pt[i].value());
        } else if (pt[i][0]->getName() == "deb-ignore") {
            parseDebMarkInstall(pt[i].value());
        } else if (pt[i][0]->getName() == "deb-recurse-limit") {
            parseDebRecurseDepth(pt[i].value());
        } else if (pt[i][0]->getName() == "deb") {
            parseDebInstall(pt[i].value());
        } else if (pt[i][0]->getName() == "include") {
            parseInclude(pt[i].value());
        } else {
            cout << "[WARNING] unsupported statement at " << pt[i][0]->location << endl;
        }
    }
}



int main() {
    try {
        srand(time(nullptr));
        temp = new estd::files::TmpDir();
        repoCache = new RepoCache(temp);
        parseInclude(Element({Token("include"), Token("vendor.txt")}));

    } catch (std::exception& e) {
        cout << "[ERROR] ";
        cout << e.what() << endl;
    }

    return 0;
}