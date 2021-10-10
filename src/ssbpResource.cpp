#include "ssbpResource.h"
#include <iostream>
#include <glm/gtc/type_ptr.hpp>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <Magick++.h>

static GLFWwindow *initOpenGL()
{
    if (!glfwInit())
        throw std::runtime_error("Failed to init GLFW");
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow *window = glfwCreateWindow(500, 500, "SSBP Viewer", nullptr, nullptr);
    if (window == nullptr)
        throw std::runtime_error("Failed to create GLFW window");
    glfwMakeContextCurrent(window);
    glfwSetWindowSizeLimits(window, 300, 300, GLFW_DONT_CARE, GLFW_DONT_CARE);
    glfwSwapInterval(1);
    glfwSetInputMode(window, GLFW_STICKY_MOUSE_BUTTONS, GL_TRUE);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        throw std::runtime_error("Failed to initialize GLAD");
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(1,1,1,1);
    glClear(GL_COLOR_BUFFER_BIT);
    return window;
}

GLFWwindow *SsbpResource::window = initOpenGL();
std::map<std::string, Ssbp> SsbpResource::_ssbps;
std::map<std::string, const Texture> SsbpResource::_textures;
Quad SsbpResource::quad;

Texture::Texture(const std::filesystem::path &path)
{
    if (!std::filesystem::exists(path))
        return;
    try {
        file_name = path.stem().string();
        std::string name = path.string();
        Magick::Image img(name);
        width = img.columns();
        height = img.rows();
        nbChannels = img.channels();

        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        uint8_t* pixel_array = img.getPixels(0, 0, width, height);
        if (nbChannels == 3)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, (int)width, (int)height, 0, GL_RGB, GL_UNSIGNED_BYTE, pixel_array);
        else if (nbChannels == 4) {
            for (int i = 0; i < width * height; ++i) {
                pixel_array[i*4] = uint8_t(int(pixel_array[i*4] * pixel_array[i*4+3]) / 0xFF);
                pixel_array[i*4+1] = uint8_t(int(pixel_array[i*4+1] * pixel_array[i*4+3]) / 0xFF);
                pixel_array[i*4+2] = uint8_t(int(pixel_array[i*4+2] * pixel_array[i*4+3]) / 0xFF);
            }
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (int)width, (int)height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixel_array);
        } else
            throw std::runtime_error("Unsupported format image");
        glGenerateMipmap(GL_TEXTURE_2D);

        loaded = true;
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
};

Texture::Texture(Texture &&rhs)
{
    file_name = std::move(rhs.file_name);
    id = rhs.id; rhs.id = 0;
    loaded = rhs.loaded; rhs.loaded = false;
    width = rhs.width; height = rhs.height; nbChannels = rhs.nbChannels;
}

Texture::~Texture()
{
    if (loaded)
        glDeleteTextures(1, &id);
};

#define OpenGLCheckShaderError(shader) {int success=1; glGetShaderiv(shader, GL_COMPILE_STATUS, &success); if (!success) {char log[512] = {0}; glGetShaderInfoLog(shader, 512, nullptr, log); throw std::logic_error(std::string(__FILE__) + ":" + std::to_string(__LINE__) + ": " + log);}}
#define OpenGLCheckProgramError() {int success=1; glGetProgramiv(id, GL_LINK_STATUS, &success); if (!success) {char log[512] = {0}; glGetProgramInfoLog(id, 512, nullptr, log); throw std::logic_error(std::string(__FILE__) + ":" + std::to_string(__LINE__) + ": " + log);}}
Quad::Quad()
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
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)48/*sizeof(float) * 3 * 4*/);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 0, (void*)80/*(sizeof(float) * 5 * 4*/);
        glEnableVertexAttribArray(2);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        abort();
    }
}

Quad::~Quad()
{
    if (buffer)
        glDeleteBuffers(1, &buffer);
}

void Quad::draw()
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

void Quad::draw(const std::array<glm::vec3,4> &vertex, const std::array<glm::vec2,4> &uvs, const std::array<glm::vec4,4> &colors)
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

void Quad::set(const std::string &var, int value) { glUseProgram(id); glUniform1i(glGetUniformLocation(id, var.c_str()), value); }
void Quad::set(const std::string &var, float value) { glUseProgram(id); glUniform1f(glGetUniformLocation(id, var.c_str()), value); }
void Quad::set(const std::string &var, const glm::vec2 &value) { glUseProgram(id); glUniform2f(glGetUniformLocation(id, var.c_str()), value.x, value.y); }
void Quad::set(const std::string &var, const glm::vec3 &value) { glUseProgram(id); glUniform3f(glGetUniformLocation(id, var.c_str()), value.x, value.y, value.z); }
void Quad::set(const std::string &var, const glm::vec4 &value) { glUseProgram(id); glUniform4f(glGetUniformLocation(id, var.c_str()), value.x, value.y, value.z, value.w); }
void Quad::set(const std::string &var, const glm::mat4 &value) { glUseProgram(id); glUniformMatrix4fv(glGetUniformLocation(id, var.c_str()), 1, false, glm::value_ptr(value)); }
void Quad::set(const std::string &var, const Texture &value)
{
    glUseProgram(id);
    glUniform1i(glGetUniformLocation(id, var.c_str()), textureId);
    glActiveTexture(GL_TEXTURE0 + textureId++);
    glBindTexture(GL_TEXTURE_2D, value);
}