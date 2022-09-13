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

namespace fs = std::filesystem;

namespace {
    std::map<Path, string> fileMap;

    std::vector<std::pair<Path, Path>> getListOfTransfered(Path from, Path to) {
        std::vector<std::pair<Path, Path>> result;
        if (fs::exists(from)) {
            if (fs::is_directory(from)) {
                for (const auto& entry : std::filesystem::recursive_directory_iterator(from)) {
                    Path f = entry.path().lexically_normal();
                    Path t = *f.replacePrefix(from, to);
                    result.push_back({f, t});
                }
            } else {
                Path f = from.lexically_normal();
                Path t = *f.replacePrefix(from, to);
                result.push_back({f, t});
            }
        }
        return result;
    }
} // namespace

void copyRepo(std::string repo, Path source, Path destination) {
    if (fs::is_directory(source)) {
        fs::create_directories(destination);
    } else {
        fs::create_directories(destination.parent_path());
    }

    for (auto e : getListOfTransfered(source, destination)) {
        Path from = e.first;
        Path to = e.second;

        if (fileMap.count(to)) {
            if (fileMap[to] != repo && !std::filesystem::is_directory(from))
                cout << "[WARNING] conflicting file " << to << " in (" << repo << ") using file from (" << fileMap[to]
                     << ") since it was installed first\n";
            continue;
        }
        fs::create_directories(to.parent_path());
        fs::copy(from, to, fs::copy_options::overwrite_existing | fs::copy_options::copy_symlinks);
        fileMap[to] = repo;
    }
}