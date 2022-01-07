#include <iostream>
#include <set>
#include <GLFW/glfw3.h>

#include "ASsbpViewer.h"

ASsbpViewer::ASsbpViewer()
{
    background = nullptr;
    glfwGetFramebufferSize(SsbpResource::window, &width, &height);
    mover = glm::vec3(0, -0.5, 0);
    scaler = glm::vec3(2.f / width, 2.f / height, 1);
    setViewMatrix();
    setBackgroundType(Fit);
}

ASsbpViewer::~ASsbpViewer()
{
    SsbpResource::window = nullptr;
    glfwTerminate();
}

void ASsbpViewer::render(bool useBackground, bool swap)
{
    glClear(GL_COLOR_BUFFER_BIT);

    if (useBackground && background && background->loaded) {
        SsbpResource::quad.set("u_Texture", *background);
        float w, h;
        if (backgroundSize.percent()) {
            w = 1; h = 1;
        } else if (backgroundSize.less()) {
            w = 1; h = float(background->height * width) / height / background->width;
        } else if (backgroundSize.greater()) {
            w = float(background->width * height) / width / background->height; h = 1;
        } else if (backgroundSize.aspect()) {
            w = float(backgroundSize.width() == 0 ? background->width : backgroundSize.width()) / width;
            h = float(backgroundSize.height() == 0 ? background->height : backgroundSize.height()) / height;
        } else if (background->width * height / background->height > width) {
            w = float(background->width * height) / width / background->height; h = 1;
        } else {
            w = 1; h = float(background->height * width) / height / background->width;
        }
        float x = float(backgroundSize.xOff()) / width;
        float y = float(backgroundSize.yOff()) / height;
        SsbpResource::quad.draw({
            glm::vec3{(x*2-1 - mover.x) / scaler.x, (y*2+1 - mover.y) / scaler.y, 0},
            glm::vec3{(x*2-1 - mover.x) / scaler.x, ((y-h)*2+1 - mover.y) / scaler.y, 0},
            glm::vec3{((x+w)*2-1 - mover.x) / scaler.x, (y*2+1 - mover.y) / scaler.y, 0},
            glm::vec3{((x+w)*2-1 - mover.x) / scaler.x, ((y-h)*2+1 - mover.y) / scaler.y, 0},
        });
    }

    SsbpPlayer::draw();

    if (swap)
        glfwSwapBuffers(SsbpResource::window);
}

void ASsbpViewer::replace(const std::string &name, const std::filesystem::path &texture)
{
    if (!_ssbp || !_animpack || !_animation) throw std::runtime_error("replace cannot ba called without initializing an animation.");
    std::filesystem::path relativePath = std::filesystem::relative(std::filesystem::absolute(texture), std::filesystem::absolute(_ssbp->_path.parent_path() / _ssbp->imageBaseDir));
    SsbpResource::addTexture(_ssbp->_path, _ssbp->imageBaseDir, relativePath.string());
    bool found = false;
    for (Cell &cell : _ssbp->cells)
        if (cell.textureName == name) {
            cell.texturePath = relativePath.string();
            cell.textureName = relativePath.stem().string();
            found = true;
        }
    if (found) return;
    for (AnimePack &pack : _ssbp->animePacks) {
        for (Part &part : pack.parts)
            if (part.name == name)
                for (Animation &anim : pack.animations)
                    if (anim.initialParts.at(part.index).cellIndex >= 0) {
                        _ssbp->cells.at(anim.initialParts.at(part.index).cellIndex).texturePath = relativePath.string();
                        _ssbp->cells.at(anim.initialParts.at(part.index).cellIndex).textureName = relativePath.stem().string();
                        part.type = PartType::Normal;
                        part.extAnime.clear();
                        part._anime = nullptr;
                        goto nextPack;
                    }
        nextPack: continue;
    }
}

void ASsbpViewer::replace(const std::string &name, const std::filesystem::path &ssbppath, const std::string &anim)
{
    if (!_ssbp || !_animpack || !_animation) throw std::runtime_error("replace cannot ba called without initializing an animation.");
    std::set<Part*> parts;
    for (AnimePack &pack : _ssbp->animePacks)
        for (Part &part : pack.parts)
            if (part.name == name)
                parts.insert(&part);
    if (parts.size() == 0) return;
    try {
        Ssbp &ssbp = Ssbp::create(ssbppath.string());
        std::string packName = anim.substr(0, anim.find('/'));
        auto packIt = std::find_if(ssbp.animePacks.begin(), ssbp.animePacks.end(), [&packName](const AnimePack &pack) { return pack.name == packName; });
        if (packIt == ssbp.animePacks.end()) throw std::range_error("");
        std::string animName = anim.substr(anim.find('/')+1);
        auto animIt = std::find_if(packIt->animations.begin(), packIt->animations.end(), [&animName](const Animation &anim) { return anim.name == animName; });
        if (animIt == packIt->animations.end()) throw std::range_error("");
        for (Part *part : parts)
            part->_anime = &ssbp;
    } catch (const std::invalid_argument &e) {
        std::cerr << "Failed to open requested SSBP file: " << ssbppath << std::endl; return;
    } catch (const std::range_error &e) {
        std::cerr << "Failed to access requested animation: " << anim << std::endl; return;
    }
    for (Part *part : parts) {
        part->type = PartType::Instance;
        part->extAnime = anim;
        _partsAnime[part->index] = SsbpPlayer(*part->_anime);
        _partsAnime[part->index].play(anim);
    }
}

void ASsbpViewer::setBackgroundType(BackgroundType type)
{
    ssize_t x = backgroundSize.xOff(), y = backgroundSize.yOff();
    if (type == BackgroundType::Fit)
        backgroundSize = Magick::Geometry("0x0%");
    else if (type == BackgroundType::FitWidth)
        backgroundSize = Magick::Geometry("0x0<");
    else if (type == BackgroundType::FitHeight)
        backgroundSize = Magick::Geometry("0x0>");
    else if (type == BackgroundType::Stretch)
        backgroundSize = Magick::Geometry("0x0");
    else if (type == BackgroundType::Original)
        backgroundSize = Magick::Geometry("0x0!");
    else
        throw std::invalid_argument("Background scale and size arguments must be specified with size");
    backgroundSize.xOff(x), backgroundSize.yOff(y);
}

void ASsbpViewer::setBackgroundType(BackgroundType type, const glm::vec2 &size)
{
    if (type == BackgroundType::Scale) {
        if (background == nullptr)
            throw std::invalid_argument("Argument scale must be used after the initialization of the background");
        else if (!background->loaded)
            return;
        backgroundSize = Magick::Geometry(size_t(background->width * size.x), size_t(background->height * size.y));
        backgroundSize.aspect(true);
    } else if (type == BackgroundType::Size) {
        backgroundSize = Magick::Geometry(size_t(size.x), size_t(size.y));
        backgroundSize.aspect(true);
    } else
        setBackgroundType(type);
}

void ASsbpViewer::shiftBackground(const glm::vec2 &shift, bool isPercent)
{
    if (isPercent) {
        if (!background) throw std::invalid_argument("Argument shift must be used after the initialization of the background");
        else if (!background->loaded) return;
        backgroundSize.xOff(ssize_t(shift.x * background->width / 100.f));
        backgroundSize.yOff(ssize_t(shift.y * background->height / 100.f));
    } else {
        backgroundSize.xOff(ssize_t(shift.x));
        backgroundSize.yOff(ssize_t(shift.y));
    }
}