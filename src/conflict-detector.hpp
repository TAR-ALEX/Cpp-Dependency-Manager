#pragma once

#include <estd/filesystem.hpp>
#include <estd/ptr.hpp>
#include <filesystem>
#include <iostream>
#include <map>
#include <memory>
#include <tar/tar.hpp>
#include <thread>
#include <vector>

using namespace std;
using namespace estd::shortnames;
using estd::files::Path;
using estd::files::RecursiveDirectoryIterator;

namespace fs = estd::files;

namespace {
    std::map<Path, string> fileMap;

    std::vector<std::pair<Path, Path>> getListOfTransfered(Path from, Path to) {
        std::vector<std::pair<Path, Path>> result;
        if (fs::exists(from)) {
            Path f = from.normalize();
            Path t = *f.replacePrefix(from, to);
            result.push_back({f, t});

            if (fs::isDirectory(from)) {
                for (const auto& entry : RecursiveDirectoryIterator(from)) {
                    Path f = entry.path().normalize();
                    Path t = *f.replacePrefix(from, to);
                    result.push_back({f, t});
                }
            }
        }
        return result;
    }
} // namespace

void copyRepo(std::string repo, Path source, Path destination) {
    if (fs::isDirectory(source)) {
        fs::createDirectories(destination);
    } else {
        fs::createDirectories(destination.splitSuffix().first);
    }

    for (auto e : getListOfTransfered(source, destination)) {
        Path from = e.first;
        Path to = e.second;

        if (fileMap.count(to)) {
            if (fileMap[to] != repo && !fs::isDirectory(from))
                cout << "[WARNING] conflicting file " << to << " in (" << repo << ") using file from (" << fileMap[to]
                     << ") since it was installed first\n";
            continue;
        }
        // cout << "fs::copy(" << from << ", " << to << ")\n";
        fs::createDirectories(to.removeEmptySuffix().splitSuffix().first);
        fs::copy(from, to, fs::CopyOptions::overwriteExisting);
        fileMap[to] = repo;
    }
}