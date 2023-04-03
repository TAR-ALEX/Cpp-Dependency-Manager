#pragma once

#include <estd/filesystem.hpp>
#include <estd/ostream_proxy.hpp>
#include <estd/ptr.hpp>
#include <filesystem>
#include <map>
#include <string>

using estd::files::Path;
using estd::files::TmpDir;
using namespace estd::shortnames;
using namespace std;

class RepoCache {
private:
    size_t nextRepoCacheId = 0;
    std::map<std::string, Path> cahceDirs;
    std::map<std::string, Path> cahceFiles;
    std::map<std::string, std::set<Path>> cacheSourcePaths;
    jptr<TmpDir> temp;
    // estd::ostream_proxy dbg{&std::cout};
    estd::ostream_proxy dbg{};

public:
    RepoCache(jptr<TmpDir> t) : temp(t) {}
    Path access(std::string repo) {
        if (cahceDirs.count(repo)) return cahceDirs[repo];

        Path p = temp->path() / std::to_string(nextRepoCacheId++);
        p = p.normalize();
        std::filesystem::create_directories(p);
        cahceDirs[repo] = p;
        return p;
    }

    bool exists(std::string repo) { return cahceDirs.count(repo); }

    inline Path createDir(std::string repo, Path sourcePath, std::function<void(Path)> creationFunc) {
        if (exists(repo)) {
            for (Path cachedPath : cacheSourcePaths[repo]) {
                if (cachedPath.contains(sourcePath)) {
                    dbg << "cache hit\n";
                    return access(repo);
                }
            }
            dbg << "cache miss\n";
            Path p = access(repo);
            creationFunc(p);
            cacheSourcePaths[repo].insert(sourcePath);
            return p;
        }
        dbg << "cache init\n";
        Path p = access(repo);
        creationFunc(p);
        cacheSourcePaths[repo].insert(sourcePath);
        return p;
    }

    inline Path getFilePath(std::string repo) {
        if (cahceFiles.count(repo)) return cahceFiles[repo];
        Path p = temp->path() / std::to_string(nextRepoCacheId++);
        p = p.normalize();
        cahceFiles[repo] = p;
        return p;
    }
};