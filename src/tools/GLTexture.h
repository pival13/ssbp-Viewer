#pragma once

#include <filesystem>

struct GLTexture {
    GLTexture(const std::filesystem::path &path);
    ~GLTexture();
    GLTexture(GLTexture &&rhs);

    operator int() const { return id; }

    std::string file_name;
    uint32_t id;
    uint64_t width;
    uint64_t height;
    uint64_t nbChannels;
    bool loaded = false;
};
