#pragma once

#include <filesystem>

class TmpDir {
private:
    inline static std::string generateUniqueTempDir() {
        while (true) {
            std::filesystem::path name = "." + estd::string_util::gen_random(10);
            if (!std::filesystem::exists(name)) {
                std::filesystem::create_directories(name);
                return (std::filesystem::current_path() / name.string()).lexically_normal();
            }
        }
    }

public:
    std::filesystem::path path = "";
    TmpDir() { path = generateUniqueTempDir(); }
    ~TmpDir() { std::filesystem::remove_all(path); }

    void discard(){
        for (const auto & entry : std::filesystem::directory_iterator(path))
            std::filesystem::remove_all(entry);
    }
};