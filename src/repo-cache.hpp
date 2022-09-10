#pragma once

#include <estd/filesystem.hpp>
#include <estd/ptr.hpp>
#include <map>
#include <string>
#include <filesystem>

using estd::files::Path;
using estd::files::TmpDir;
using namespace estd::shortnames;

class RepoCache {
private:
    size_t nextRepoCacheId = 0;
    std::map<std::string, Path> repoCachePath;
    jptr<TmpDir> temp;

public:
    RepoCache(jptr<TmpDir> t) : temp(t) {}
    Path access(std::string repo) {
        if (repoCachePath.count(repo)) return repoCachePath[repo];

        Path p = temp->path() / std::to_string(nextRepoCacheId++);
        p = p.lexically_normal();
        std::filesystem::create_directories(p);
        repoCachePath[repo] = p;
        return p;
    }

    bool exists(std::string repo) { return repoCachePath.count(repo); }
};