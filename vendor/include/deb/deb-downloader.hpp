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

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <ar/ar.hpp>
#include <boost/regex.hpp>
#include <bxzstr.hpp>
#include <estd/filesystem.hpp>
#include <estd/ostream_proxy.hpp>
#include <estd/ptr.hpp>
#include <estd/semaphore.h>
#include <estd/thread_pool.hpp>
#include <filesystem>
#include <fstream>
#include <httplib.h>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <tar/tar.hpp>
#include <thread>

namespace deb {
    namespace {
        using namespace std;
        namespace fs = std::filesystem;

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

        std::string downloadString(string url) {
            int numRetry = 3;
            for (int i = 1; i <= numRetry; i++) {
                try {
                    std::string scheme = "";
                    std::string host = "";
                    std::string path = "";

                    tie(scheme, host, path) = splitUrl(url);

                    httplib::Client cli((scheme + host).c_str());
                    cli.set_follow_location(true);
                    cli.set_read_timeout(5);
                    cli.set_connection_timeout(7);
                    cli.set_write_timeout(3);
                    auto res = cli.Get(path.c_str());
                    if (res.error() != httplib::Error::Success) throw runtime_error("Request error " + url);
                    return res->body;
                } catch (exception& e) {
                    if (i == numRetry) throw e;
                }
            }
            throw runtime_error("Failed to fetch url: " + url);
        }

        std::filesystem::path downloadFile(string url, std::filesystem::path location) {
            int numRetry = 3;
            for (int i = 1; i <= numRetry; i++) {
                try {
                    std::string scheme = "";
                    std::string host = "";
                    std::string path = "";

                    tie(scheme, host, path) = splitUrl(url);

                    std::filesystem::path extractFilename = path;
                    std::filesystem::path filename = extractFilename.filename();

                    fs::create_directories(location);

                    ofstream file(location / filename);

                    httplib::Client cli((scheme + host).c_str());
                    cli.set_read_timeout(20);
                    cli.set_connection_timeout(20);
                    cli.set_write_timeout(20);
                    cli.set_follow_location(true);

                    auto res = cli.Get(
                        path.c_str(),
                        httplib::Headers(),
                        [&](const httplib::Response& response) {
                            return true; // return 'false' if you want to cancel the request.
                        },
                        [&](const char* data, size_t data_length) {
                            file.write(data, data_length);
                            return true; // return 'false' if you want to cancel the request.
                        }
                    );

                    file.close();
                    if (res.error() != httplib::Error::Success) throw runtime_error("Request error " + url);
                    return location / filename;
                } catch (exception& e) {
                    if (i == numRetry) throw e;
                }
            }
            throw runtime_error("Failed to fetch url: " + url);
        }

        std::vector<std::string> split(const string& input, const string& regex) {
            // passing -1 as the submatch index parameter performs splitting
            static map<string, boost::regex> rgx;
            if (!rgx.count(regex)) rgx[regex] = boost::regex(regex, boost::regex::optimize);
            boost::sregex_token_iterator first{input.begin(), input.end(), rgx[regex], -1}, last;
            return {first, last};
        }

        string streamToString(istream& is) {
            stringstream ss;
            ss << is.rdbuf();
            return ss.str();
        }
    }; // namespace

    using namespace std;

    class Installer {
    public:
        estd::ostream_proxy cout;
        std::vector<std::string> sourcesList;
        std::map<std::string, std::string> packageToUrl;
        std::set<string> installed;
        std::set<string> preInstalled;
        std::mutex installLock;
        estd::thread_pool trm{16};

        vector<tuple<string, string>> getListUrls() {
            vector<tuple<string, string>> result;
            for (auto source : sourcesList) {
                stringstream ss(source);
                string token;
                if (!(ss >> token)) continue;
                if (token != "deb") continue;
                string baseUrl;
                if (!(ss >> baseUrl)) continue;
                string distribution;
                if (!(ss >> distribution)) continue;

                string component;

                while (ss >> component) {
                    tuple<string, string> t = {
                        baseUrl,
                        baseUrl + "/dists/" + distribution + "/" + component + "/" + architecture + "/" +
                            "Packages.gz"};
                    result.push_back(t);
                }
            }
            for (const auto& elem : result) cout << get<1>(elem) << "\n";
            return result;
        }
        void getPackageList() {
            auto urls = getListUrls();
            std::vector<std::map<std::string, std::string>> packageToUrlMaps;
            packageToUrlMaps.resize(urls.size());
            int k = 0;
            for (const auto& entry : urls) {
                auto* mapToPlace = &packageToUrlMaps[k];
                k++;
                trm.schedule([=]() {
                    string listUrl;
                    string baseUrl;
                    tie(baseUrl, listUrl) = entry;

                    stringstream compressed(downloadString(listUrl));
                    bxz::istream decompressed(compressed);
                    stringstream ss;
                    ss << decompressed.rdbuf();

                    vector<string> entries = split(ss.str(), "\n\n");
                    cout << entries.size() << "\n";

                    for (string entry : entries) {
                        string packageName = "";
                        string packagePath = "";
                        {
                            static boost::regex rgx("Package:\\s?([^\\r\\n]*)", boost::regex::optimize);
                            boost::smatch matches;
                            if (boost::regex_search(entry, matches, rgx) && matches.size() == 2) {
                                packageName = matches[1];
                            } else {
                                cout << "Match not found\n";
                                continue;
                            }
                        }
                        {
                            static boost::regex rgx("Filename:\\s?([^\\r\\n]*)", boost::regex::optimize);
                            boost::smatch matches;
                            if (boost::regex_search(entry, matches, rgx) && matches.size() == 2) {
                                packagePath = matches[1];
                            } else {
                                cout << "Match not found\n";
                                continue;
                            }
                        }
                        packagePath = baseUrl + "/" + packagePath;
                        auto provides = getFields(entry, "Provides");
                        auto source = getFields(entry, "Source");
                        provides.insert(provides.end(), source.begin(), source.end());
                        provides.push_back(packageName);
                        for (auto i : provides) {
                            if (!mapToPlace->count(i)) { (*mapToPlace)[i] = packagePath; }
                        }
                    }
                });
            }
            trm.wait();
            for (auto& m : packageToUrlMaps) { packageToUrl.insert(m.begin(), m.end()); }
            // for(const auto& elem : packageToUrl){
            //     cout << elem.first << " -> " << elem.second << "\n";
            // }
        }
        vector<string> getFields(const string& contolFile, string typeOfDep = "Depends") {
            vector<string> result;
            map<string, boost::regex> rgx;
            if (!rgx.count(typeOfDep))
                rgx[typeOfDep] = boost::regex(typeOfDep + ": ?([^\\r\\n]*)", boost::regex::optimize);
            string depends;
            {
                boost::smatch matches;
                if (boost::regex_search(contolFile, matches, rgx[typeOfDep]) && matches.size() == 2) {
                    depends = string(matches[1]);
                } else {
                    return result;
                }
            }
            auto entries = split(depends, ",|\\|");
            boost::regex r("(?:\\s+)|(?:\\(.*\\))|(?::.*)", boost::regex::optimize);
            for (auto entry : entries) {
                // (\S+)\s?(?:\((\S*)\s?(\S*)\))?
                result.push_back(boost::regex_replace(entry, r, ""));
            }
            return result;
        }
        void installPrivate(
            string package, std::set<std::pair<std::string, std::string>> locations, int recursionDepth
        ) {
            string url;
            vector<std::thread> threads;
            {
                unique_lock<mutex> lock(installLock);

                if (!packageToUrl.count(package)) {
                    if (throwOnFailedDependency)
                        throw runtime_error("package " + package + " does not exist in repository.");
                    return;
                }
                url = packageToUrl[package];

                if (installed.count(url)) {
                    cout << "already installed " + package + "\n";
                    return;
                }

                installed.insert(url);
                cout << "installed " + package + "\n";
            }

            auto packageLoc = downloadFile(url, tmpDirectory->path());
            ar::Reader deb(packageLoc.string());

            auto versionStream = deb.open("debian-binary");
            string version = streamToString(versionStream);
            if (version.find("2.0") == string::npos)
                throw runtime_error("package " + package + " has a bad version number " + version + ".");

            string controlString;

            estd::isubstream dataTarCompressedStream;
            for (auto path : {"data.tar.xz", "data.tar.gz", "data.tar"}) {
                try {
                    dataTarCompressedStream = deb.open(path);
                    break;
                } catch (...) {}
            }

            bxz::istream dataTarStream(dataTarCompressedStream);
            tar::Reader dataTar(dataTarStream);
            dataTar.throwOnUnsupported = false;
            dataTar.extractHardLinksAsCopies = extractHardLinksAsCopies;
            dataTar.extractSoftLinksAsCopies = extractSoftLinksAsCopies;
            dataTar.throwOnInfiniteRecursion = false;
            dataTar.throwOnBrokenSoftlinks = false;
            dataTar.permissionMask = permissionMask;

            for (auto [source, destination] : locations) { dataTar.extractPath(source, destination); }


            if (recursionDepth <= 1) return;

            estd::isubstream controlTarCompressedStream;
            for (auto path : {"control.tar.xz", "control.tar.gz", "control.tar"}) {
                try {
                    controlTarCompressedStream = deb.open(path);
                    break;
                } catch (...) {}
            }

            bxz::istream controlTarStream(controlTarCompressedStream);
            tar::Reader controlTar(controlTarStream);
            auto controlFile = controlTar.open("control");
            controlString = streamToString(controlFile);

            // https://stackoverflow.com/a/3177252
            const auto combineVectors = [](std::vector<std::string>& A, std::vector<std::string> B){
                std::vector<std::string> AB;
                AB.reserve( A.size() + B.size() ); // preallocate memory
                AB.insert( AB.end(), A.begin(), A.end() );
                AB.insert( AB.end(), B.begin(), B.end() );
                return AB;
            };

            // cout << "\n" << controlString << "\n";
            auto deps = getFields(controlString, "Depends");
            deps = combineVectors(deps, getFields(controlString, "Recommends"));
            deps = combineVectors(deps, getFields(controlString, "Suggests"));
            deps = combineVectors(deps, getFields(controlString, "Pre-Depends"));

            for (auto dep : deps) {
                trm.schedule([this, dep, locations, recursionDepth]() {
                    this->installPrivate(dep, locations, recursionDepth - 1);
                });
            }
        }


    public:
        std::string architecture = "binary-amd64";
        estd::joint_ptr<estd::files::TmpDir> tmpDirectory;
        int recursionLimit = 9999;
        bool throwOnFailedDependency = true;
        bool extractHardLinksAsCopies = true;
        bool extractSoftLinksAsCopies = true; // dont do this, there can be links among different packages like libX.so linking to libX.so.5.1.1
        uint16_t permissionMask = 0777;

        Installer() {}

        Installer(vector<string> l, estd::joint_ptr<estd::files::TmpDir> tmp = nullptr) : sourcesList(l) {
            if (tmp) {
                tmpDirectory = tmp;
            } else {
                tmpDirectory = new estd::files::TmpDir();
            }
        }

        void setSources(vector<string> l) {
            sourcesList = l;
            packageToUrl = {};
        }

        void markPreInstalled(std::set<std::string> pkgs) {
            if (packageToUrl.empty()) getPackageList();

            for (auto pkg : pkgs) preInstalled.insert(pkg);
            //TODO: mark dependencies as installed as well
        }

        void markInstalled(std::set<std::string> pkgs) {
            if (packageToUrl.empty()) getPackageList();

            for (auto pkg : pkgs) installed.insert(pkg);
            //TODO: mark dependencies as installed as well
        }

        void clearInstalled() { installed.clear(); }

        void install(string package, string location) { install(package, {{"./", location}}); }

        void install(std::string package, std::set<std::pair<std::string, std::string>> locations) {
            if (packageToUrl.empty()) getPackageList();

            installed.insert(preInstalled.begin(), preInstalled.end());

            auto pkgs = split(package, "\\s+");
            for (auto pkg : pkgs) {
                trm.schedule([this, pkg, locations]() { installPrivate(pkg, locations, recursionLimit); });
            }
            trm.forwardExceptions = true;
            trm.wait();
        }
    };
}; // namespace deb
