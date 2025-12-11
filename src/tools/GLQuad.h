#pragma once

#include "GLTexture.h"
#include <array>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

struct GLQuad {
    static std::array<glm::vec4,4> originalColor;
    static std::array<glm::vec2,4> fullTexture;

    GLQuad();
    ~GLQuad();

    void draw();
    void draw(const std::array<glm::vec3,4> &vertex, const std::array<glm::vec2,4> &uvs=fullTexture, const std::array<glm::vec4,4> &colors=originalColor);
    void set(const std::string &var, int value);
    void set(const std::string &var, float value);
    void set(const std::string &var, const glm::vec2 &value);
    void set(const std::string &var, const glm::vec3 &value);
    void set(const std::string &var, const glm::vec4 &value);
    void set(const std::string &var, const glm::mat4 &value);
    void set(const std::string &var, const GLTexture &value);

private:
    int id = 0;
    unsigned int buffer = 0;
    unsigned int vertexArray = 0;
    int textureId = 0;
};
