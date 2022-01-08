#include "Screenshot.hpp"
#include "ssbpResource.h"

#include <GLFW/glfw3.h>
#include <iostream>
#include <glm/ext/matrix_int2x2_sized.hpp>

using namespace Magick;

Saver::Saver()
{
    t = std::thread([this]() {
        while (SsbpResource::window != nullptr || !_images.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            if (!_images.empty()) {
                auto &[toSave, name] = _images.front();
                if (std::holds_alternative<Image>(toSave))
                    std::get<Image>(toSave).write(name);
                else
                    writeImages(std::get<std::vector<Image>>(toSave).begin(), std::get<std::vector<Image>>(toSave).end(), name);
                std::cout << "  > File saved: " << name << std::endl;
                std::unique_lock lock(_mutex);
                _images.pop();
            }
        }
    });
}

Saver::~Saver()
{
    if (t.joinable()) t.join();
}

void Saver::save(const std::string &name, const Image &image, const Geometry &bound)
{
    std::filesystem::create_directories(std::filesystem::path(name).parent_path());

    Image toSave = image;
    if (bound.isValid() && bound.width() > 0 && bound.height() > 0)
        toSave.crop(bound);
    std::unique_lock lock(_mutex);
    _images.emplace(std::move(toSave), name);
}

void Saver::save(const std::string &name, const std::vector<Image> &images, const Geometry &bound, LoopState looping)
{
    std::filesystem::create_directories(std::filesystem::path(name).parent_path());

    std::vector<Image> imgs;
    for (auto &image : images) {
        imgs.push_back(Image(image));
        if (!bound.isValid() || bound.width() == 0 || bound.height() == 0) continue;
        imgs.back().crop(bound);
        imgs.back().page(Geometry(bound.width(), bound.height()));
    }
    imgs.front().animationIterations(looping == NoLoop ? 1 : 0);
    if (looping == SlowLoop) {
        imgs.front().animationDelay(30);
        imgs.back().animationDelay(100);
    }

    std::unique_lock lock(_mutex);
    _images.emplace(std::move(imgs), name);
}

Image Saver::screen() const
{
    static std::vector<uint8_t> buffer;
    int width, height;
    glfwGetFramebufferSize(SsbpResource::window, &width, &height);
    buffer.resize(width * height * 4);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, buffer.data());

    // Reverse premultiplied alpha
    for (int i = 0; i != width * height; ++i) {
        if (buffer[i*4+3] == 0) continue;
        buffer[i*4] = GLubyte(std::min(int(buffer[i*4] * 0xFF) / buffer[i*4+3], 0xFF));
        buffer[i*4+1] = GLubyte(std::min(int(buffer[i*4+1] * 0xFF) / buffer[i*4+3], 0xFF));
        buffer[i*4+2] = GLubyte(std::min(int(buffer[i*4+2] * 0xFF) / buffer[i*4+3], 0xFF));
    }

    Image img = Image(width, height, "RGBA", CharPixel, buffer.data());
    img.flip();
    return img;
}

Geometry Saver::bounds(const Image &image) const
{
    Geometry area;
    try {
        area = image.boundingBox();
    } catch (const std::exception &e) {
        return Geometry(0, 0);
    }
    size_t extraW = 10 + area.width() / 10 * 10 - area.width();
    size_t extraH = 10 + area.height() / 10 * 10 - area.height();
    return Geometry(
        area.width() + extraW,
        area.height() + extraH,
        area.xOff() - extraW / 2,
        area.yOff() - extraH / 2
    );
}

Geometry Saver::bounds(const std::vector<Geometry> &bounds, const Geometry &ref) const
{
    glm::i64mat2x2 size = glm::i64mat2x2(0x7FFFFFFFFFFFFFFF, 0x7FFFFFFFFFFFFFFF, 0, 0);
    for (const auto &bound : bounds) {
        if (size[0].x > bound.xOff())                        size[0].x = bound.xOff();
        if (size[1].x < bound.xOff()+(ssize_t)bound.width()) size[1].x = bound.xOff()+bound.width();
        if (size[0].y > bound.yOff())                        size[0].y = bound.yOff();
        if (size[1].y < bound.yOff()+(ssize_t)bound.height())size[1].y = bound.yOff()+bound.height();
    }
    if (size[1].x - size[0].x >= (ssize_t)ref.width() && size[1].y - size[0].y >= (ssize_t)ref.height())
        return ref;
    size_t extraW = 10 + (size[1].x - size[0].x) / 10 * 10 - (size[1].x - size[0].x);
    size_t extraH = 10 + (size[1].y - size[0].y) / 10 * 10 - (size[1].y - size[0].y);
    return Geometry(
        size[1].x - size[0].x + extraW,
        size[1].y - size[0].y + extraH,
        size[0].x - extraW / 2,
        size[0].y - extraH / 2
    );
}

Geometry Saver::bounds(const std::vector<Image> &images) const
{
    std::vector<Geometry> boxes;
    for (const Image &img : images) boxes.push_back(bounds(img));
    return bounds(boxes, images.front().size());
}
/*
static Magick::Image getSprite(bool useBackground)
{
    //glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    float colors[4];
    glGetFloatv(GL_COLOR_CLEAR_VALUE, colors);
    glClearColor(0,0,0,0);
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(colors[0], colors[1], colors[2], colors[3]);
    //if (useBackground && background.texture && background.texture->loaded) {
    //    debug(" with background");
    //    background.shader.use();
    //    background.shader.setVec2("u_Coef", float(background.texture->width) / windowWidth,
    //                                        float(background.texture->height) / windowHeight);
    //    background.shader.setTexture2D("u_Texture", background.texture->id);
    //    background.shader.setBool("u_UseTexture", true);
    //    background.draw();
    //}
    player.draw();
    //glReadBuffer(GL_COLOR_ATTACHMENT0);
    buffer.resize(windowWidth * windowHeight * 4);
    glReadPixels(0, 0, windowWidth, windowHeight, GL_RGBA, GL_UNSIGNED_BYTE, buffer.data());

    // Reverse premultiplied alpha
    for (int i = 0; i != windowWidth * windowHeight; ++i) {
        if (buffer[i*4+3] == 0) continue;
        buffer[i*4] = GLubyte(std::min(int(buffer[i*4] * 0xFF) / buffer[i*4+3], 0xFF));
        buffer[i*4+1] = GLubyte(std::min(int(buffer[i*4+1] * 0xFF) / buffer[i*4+3], 0xFF));
        buffer[i*4+2] = GLubyte(std::min(int(buffer[i*4+2] * 0xFF) / buffer[i*4+3], 0xFF));
    }
    Magick::Image img = Magick::Image(windowWidth, windowHeight, "RGBA", Magick::CharPixel, buffer.data());
    img.flip();
    
    return img;
}

std::tuple<std::vector<Magick::Image>, std::vector<Magick::Image>, std::vector<Magick::Geometry>>
    getAnimation(const std::string &anim)
{
    //sprite.ssPlayer->play(anim, 1);
    //applyArgument();
    //int animFps = sprite.ssPlayer->getAnimFps();
    //int nbFrame = sprite.ssPlayer->getMaxFrame();
//
    //std::vector<Magick::Image> imagesRGB; imagesRGB.reserve(nbFrame);
    //std::vector<Magick::Image> imagesRGBA; imagesRGBA.reserve(nbFrame);
    //std::vector<Magick::Geometry> bounds; bounds.reserve(nbFrame);
    //for (int i = 0; i != nbFrame; ++i) {
    //    sprite.ssPlayer->setFrameNo(i);
    //    sprite.ssPlayer->update(0);
    //    debug("Frame %d: ", i);
    //    imagesRGB.emplace_back(getSprite(true));
    //    debug("\n    ");
    //    imagesRGBA.emplace_back(getSprite(false));
    //    SSLOG("");
    //    bounds.emplace_back(getBound(imagesRGBA.at(i)));
    //    imagesRGBA.at(i).gifDisposeMethod(MagickCore::DisposeType::BackgroundDispose);
    //    imagesRGB.at(i).gifDisposeMethod(MagickCore::DisposeType::BackgroundDispose);
    //    imagesRGB.at(i).animationDelay(100 * i / animFps - 100 * (i-1) / animFps);
    //    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    //}
    //return std::make_tuple(imagesRGBA, imagesRGB, bounds);
    return {};
}

void saveSprite(Magick::Image image, const Magick::Geometry &bound, const std::string &name)
{
    Magick::Image img1 = getSprite(true);
    //Magick::Image img2 = getSprite(false);
    Magick::Geometry bound = getBound(img1);
    std::filesystem::create_directories("Screenshots/" + player.getFileName());

    img1.crop(bound);

    debug("Saving image...");
    std::unique_lock lock(saveMutex);
    saveImages.emplace(img1, "Screenshots/" + player.getFileName() + "/" + player.getAnimeName() + "_" + std::to_string(player.getFrame()) + ".png");
}

void saveAnimation(const std::vector<Magick::Image> &images, const std::vector<Magick::Geometry> &bounds, const std::string &name)
{
    std::filesystem::create_directories(name.substr(0, name.find_last_of("/\\")));
    std::string anim = name.substr(name.find_last_of("\\/")+1);
    anim = anim.substr(0, anim.find_last_of('.'));

    Magick::Geometry bound = getBound(bounds);
    std::vector<Magick::Image> imgs;
    for (auto &image : images) {
        imgs.push_back(Magick::Image(image));
        imgs.back().crop(bound);
        imgs.back().page(Magick::Geometry(bound.width(), bound.height()));
    }

    if (anim != "Ok" && anim != "Idle" && anim != "Pairpose" && (anim.size() <= 5 || anim.substr(anim.size()-5) != "_Loop")) {
        imgs.front().animationDelay(30);
        imgs.back().animationDelay(100);
    }

    std::unique_lock lock(saveMutex);
    saveImages.emplace(std::move(imgs), name);
}
*/