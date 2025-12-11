#include "GLTexture.h"

#include <iostream>
#include <type_traits>
#include <glad/glad.h>
#include <Magick++.h>

constexpr GLenum GL_PIXEL_FORMAT =
    std::is_same_v<Magick::Quantum, float>    ? GL_FLOAT :
    std::is_same_v<Magick::Quantum, uint8_t>  ? GL_UNSIGNED_BYTE :
    std::is_same_v<Magick::Quantum, uint16_t> ? GL_UNSIGNED_SHORT :
    std::is_same_v<Magick::Quantum, uint32_t> ? GL_UNSIGNED_INT :
    GL_NONE;

static_assert(GL_PIXEL_FORMAT != GL_NONE, "Unsupported format for Magick::Quantum");

GLTexture::GLTexture(const std::filesystem::path &path)
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
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        MagickCore::Quantum *pixel_array = img.getPixels(0, 0, width, height);
        // For floating point pixel, convert to range [0,1]
        if constexpr (GL_PIXEL_FORMAT == GL_FLOAT) {
            for (size_t i = 0; i < width*height*nbChannels; ++i)
                pixel_array[i] /= MaxMap;
        }
        if (nbChannels == 3)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, (int)width, (int)height, 0, GL_RGB, GL_PIXEL_FORMAT, pixel_array);
        else if (nbChannels == 4) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (int)width, (int)height, 0, GL_RGBA, GL_PIXEL_FORMAT, pixel_array);
        } else
            throw std::runtime_error("Unsupported format image");
        // glGenerateMipmap(GL_TEXTURE_2D);

        loaded = true;
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
};

GLTexture::GLTexture(GLTexture &&rhs)
{
    file_name = std::move(rhs.file_name);
    id = rhs.id; rhs.id = 0;
    loaded = rhs.loaded; rhs.loaded = false;
    width = rhs.width; height = rhs.height; nbChannels = rhs.nbChannels;
}

GLTexture::~GLTexture()
{
    if (loaded)
        glDeleteTextures(1, &id);
};
