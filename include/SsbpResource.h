#pragma once

#include "ssbpReader.h"
#include <array>
#include <map>
#include <filesystem>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

struct GLFWwindow;
struct Texture {
    Texture(const std::filesystem::path &path);
    ~Texture();
    Texture(Texture &&rhs);

    operator int() const { return id; }

    std::string file_name;
    uint32_t id;
    uint64_t width;
    uint64_t height;
    uint64_t nbChannels;
    bool loaded = false;
};

struct Quad {
    static std::array<glm::vec4,4> originalColor;
    static std::array<glm::vec2,4> fullTexture;

    Quad();
    ~Quad();

    void draw();
    void draw(const std::array<glm::vec3,4> &vertex, const std::array<glm::vec2,4> &uvs=fullTexture, const std::array<glm::vec4,4> &colors=originalColor);
    void set(const std::string &var, int value);
    void set(const std::string &var, float value);
    void set(const std::string &var, const glm::vec2 &value);
    void set(const std::string &var, const glm::vec3 &value);
    void set(const std::string &var, const glm::vec4 &value);
    void set(const std::string &var, const glm::mat4 &value);
    void set(const std::string &var, const Texture &value);

private:
    int id;
    unsigned int buffer;
    unsigned int vertexArray;
    int textureId = 0;
};

struct SsbpResource {
    static void addTexture(const std::filesystem::path &ssbpPath, const std::string &imageBaseDir, const std::string &texturePath) {
        std::filesystem::path path = (ssbpPath.parent_path() / imageBaseDir / texturePath).lexically_normal();
        if (_textures.find(path.string()) != _textures.end())
            return;
        _textures.emplace(path.string(), Texture(path));
    }
    static const Texture &getTexture(const std::filesystem::path &ssbpPath, const std::string &imageBaseDir, const std::string &texturePath) {
        std::filesystem::path path = (ssbpPath.parent_path() / imageBaseDir / texturePath).lexically_normal();
        auto it = _textures.find(path.string());
        if (it == _textures.end())
            throw std::invalid_argument("Unknow texture " + texturePath);
        return it->second;
    }

    static GLFWwindow *window;
    static std::map<std::string, Ssbp> _ssbps;
    static std::map<std::string, const Texture> _textures;
    static Quad quad;
};