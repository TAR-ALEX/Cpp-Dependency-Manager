#pragma once

#include <filesystem>

class TmpDir {
private:
    std::filesystem::path iPath = "";
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
    TmpDir() { iPath = generateUniqueTempDir(); }
    ~TmpDir() { std::filesystem::remove_all(iPath); }

    std::filesystem::path path() { return iPath; }

    void discard() {
        for (const auto& entry : std::filesystem::directory_iterator(iPath)) std::filesystem::remove_all(entry);
    }
};