#include "GLQuad.h"

#include <iostream>
#include <vector>
#include <glm/gtc/type_ptr.hpp>
#include <glad/glad.h>

std::array<glm::vec4,4> GLQuad::originalColor{glm::vec4{1,1,1,1},{1,1,1,1},{1,1,1,1},{1,1,1,1}};
std::array<glm::vec2,4> GLQuad::fullTexture{glm::vec2{0,0},{0,1},{1,0},{1,1}};

#define OpenGLCheckShaderError(shader) {int success=1; glGetShaderiv(shader, GL_COMPILE_STATUS, &success); if (!success) {char log[512] = {0}; glGetShaderInfoLog(shader, 512, nullptr, log); throw std::logic_error(std::string(__FILE__) + ":" + std::to_string(__LINE__) + ": " + log);}}
#define OpenGLCheckProgramError() {int success=1; glGetProgramiv(id, GL_LINK_STATUS, &success); if (!success) {char log[512] = {0}; glGetProgramInfoLog(id, 512, nullptr, log); throw std::logic_error(std::string(__FILE__) + ":" + std::to_string(__LINE__) + ": " + log);}}
GLQuad::GLQuad()
{
    const char *vertexCode = 
        #include "shaders/ssbpOpenGL.vert"
;   const char *fragmentCode = 
        #include "shaders/ssbpOpenGL.frag"
;   unsigned int vertex, fragment;

    try {
        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vertexCode, nullptr);
        glCompileShader(vertex);
        OpenGLCheckShaderError(vertex);

        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fragmentCode, nullptr);
        glCompileShader(fragment);
        OpenGLCheckShaderError(fragment);

        id = glCreateProgram();
        glAttachShader(id, vertex);
        glAttachShader(id, fragment);
        glLinkProgram(id);
        OpenGLCheckProgramError();

        glDeleteShader(vertex);
        glDeleteShader(fragment);

        glGenVertexArrays(1, &vertexArray);
        glGenBuffers(1, &buffer);
        glBindVertexArray(vertexArray);
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 9 * 4, nullptr, GL_DYNAMIC_DRAW);
        // Vertex
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glEnableVertexAttribArray(0);
        // UV
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)48/*sizeof(float) * 3 * 4*/);
        glEnableVertexAttribArray(1);
        // Color
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 0, (void*)80/*(sizeof(float) * (3+2) * 4*/);
        glEnableVertexAttribArray(2);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        abort();
    }
}

GLQuad::~GLQuad()
{
    if (buffer != 0)
        glDeleteBuffers(1, &buffer);
    if (vertexArray != 0)
        glDeleteVertexArrays(1, &vertexArray);
    if (id != 0)
        glDeleteProgram(id);
    buffer = vertexArray = id = 0;
}

void GLQuad::draw()
{
    glUseProgram(id);
    glBindVertexArray(vertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferSubData(GL_ARRAY_BUFFER, 0, 48, std::vector<float>({
        -5, 5, 0,
        -5, -5, 0,
        5, 5, 0,
        5, -5, 0,
    }).data());
    glBufferSubData(GL_ARRAY_BUFFER, 48, 32, std::vector<float>({
        0, 1,
        0, 0,
        1, 1,
        1, 0,
    }).data());
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void GLQuad::draw(const std::array<glm::vec3,4> &vertex, const std::array<glm::vec2,4> &uvs, const std::array<glm::vec4,4> &colors)
{
    glUseProgram(id);
    glBindVertexArray(vertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferSubData(GL_ARRAY_BUFFER, 0,  48/*sizeof(glm::vec3) * vertex.size()*/, vertex.data());
    glBufferSubData(GL_ARRAY_BUFFER, 48, 32/*sizeof(glm::vec2) * uvs.size()*/,    uvs.data());
    glBufferSubData(GL_ARRAY_BUFFER, 80, 64/*sizeof(glm::vec4) * colors.size()*/, colors.data());
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    textureId = 0;
}

void GLQuad::set(const std::string &var, int value) { glUseProgram(id); glUniform1i(glGetUniformLocation(id, var.c_str()), value); }
void GLQuad::set(const std::string &var, float value) { glUseProgram(id); glUniform1f(glGetUniformLocation(id, var.c_str()), value); }
void GLQuad::set(const std::string &var, const glm::vec2 &value) { glUseProgram(id); glUniform2f(glGetUniformLocation(id, var.c_str()), value.x, value.y); }
void GLQuad::set(const std::string &var, const glm::vec3 &value) { glUseProgram(id); glUniform3f(glGetUniformLocation(id, var.c_str()), value.x, value.y, value.z); }
void GLQuad::set(const std::string &var, const glm::vec4 &value) { glUseProgram(id); glUniform4f(glGetUniformLocation(id, var.c_str()), value.x, value.y, value.z, value.w); }
void GLQuad::set(const std::string &var, const glm::mat4 &value) { glUseProgram(id); glUniformMatrix4fv(glGetUniformLocation(id, var.c_str()), 1, false, glm::value_ptr(value)); }
void GLQuad::set(const std::string &var, const GLTexture &value)
{
    glUseProgram(id);
    glUniform1i(glGetUniformLocation(id, var.c_str()), textureId);
    glActiveTexture(GL_TEXTURE0 + textureId++);
    glBindTexture(GL_TEXTURE_2D, value);
}
