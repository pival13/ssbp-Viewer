#include "texture.h"
#include <filesystem>

#ifdef _MSC_VER
#pragma warning(disable: 4251)
#pragma warning(disable: 4267)
#pragma warning(disable: 4275)
#endif

//#define STB_IMAGE_IMPLEMENTATION
//#include <stb_image.h>
#include <Magick++.h>

void Texture::openFile() {
    if (!std::filesystem::exists(file_name)) {
        loaded = false;
        return;
    }
    // load and generate the texture
    try {
        Magick::Image img(file_name);
        width = img.columns();
        height = img.rows();
        nrChannels = img.channels();

        uint8_t* pixel_array = img.getPixels(0, 0, width, height);
        if (nrChannels == 4)
            for (int i = 0; i != width * height; ++i) {
                pixel_array[i*4] = uint8_t(int(pixel_array[i*4] * pixel_array[i*4+3]) / 0xFF);
                pixel_array[i*4+1] = uint8_t(int(pixel_array[i*4+1] * pixel_array[i*4+3]) / 0xFF);
                pixel_array[i*4+2] = uint8_t(int(pixel_array[i*4+2] * pixel_array[i*4+3]) / 0xFF);
            }

        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);

        // wrapping & filtering options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        GLenum format;
        switch (nrChannels) {
        case 3:
            format = GL_RGB;
            break;
        case 4:
            format = GL_RGBA;
            break;
        default:
            throw std::runtime_error("Unsupported format image");
        }
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, pixel_array);
        glGenerateMipmap(GL_TEXTURE_2D);

        loaded = true;
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
}

namespace Test {
void textureSettings7() {
    // Wrap modes GL_REPEAT, GL_MIRRORED_REPEAT, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_BORDER
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);

    // Texture filtering without mipmap filtering: GL_NEAREST, GL_LINEAR
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Texture/MipMap filtering: GL_NEAREST_MIPMAP_NEAREST, GL_LINEAR_MIPMAP_NEAREST, GL_NEAREST_MIPMAP_LINEAR, GL_LINEAR_MIPMAP_LINEAR
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}
};
