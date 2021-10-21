#include <iostream>
#include <GLFW/glfw3.h>

#include "ASsbpViewer.h"

ASsbpViewer::ASsbpViewer()
{
    background = nullptr;
    glfwGetFramebufferSize(SsbpResource::window, &width, &height);
    mover = glm::vec3(0, -0.75, 0);
    scaler = glm::vec3(2.f / width, 2.f / height, 1);
    setViewMatrix();
}

ASsbpViewer::~ASsbpViewer()
{
    SsbpResource::window = nullptr;
    glfwTerminate();
}

void ASsbpViewer::render(bool useBackground)
{
    glClear(GL_COLOR_BUFFER_BIT);

    if (useBackground && background && background->loaded) {
        SsbpResource::quad.set("u_Texture", *background);
        SsbpResource::quad.draw({
            glm::vec3{(-1 - mover.x) / scaler.x, (-1 - mover.y) / scaler.y, 0},
            glm::vec3{(-1 - mover.x) / scaler.x, (1 - mover.y) / scaler.y, 0},
            glm::vec3{(1 - mover.x) / scaler.x, (-1 - mover.y) / scaler.y, 0},
            glm::vec3{(1 - mover.x) / scaler.x, (1 - mover.y) / scaler.y, 0},
        });
    }

    SsbpPlayer::draw();
    
    glfwSwapBuffers(SsbpResource::window);
}

void ASsbpViewer::replace(const std::string &name, const std::filesystem::path &texture)
{
    if (!_ssbp) return;
    std::filesystem::path relativePath = std::filesystem::relative(texture, _ssbp->_path.parent_path() / _ssbp->imageBaseDir);
    SsbpResource::addTexture(_ssbp->_path, _ssbp->imageBaseDir, relativePath.string());
    bool found = false;
    for (Cell &cell : _ssbp->cells)
        if (cell.textureName == name) {
            cell.texturePath = relativePath.string();
            cell.textureName = relativePath.stem().string();
            found = true;
        }
    if (found) return;
    for (AnimePack &pack : _ssbp->animePacks)
        for (Part &part : pack.parts)
            if (part.name == name && _animation->initialParts.at(part.index).cellIndex >= 0) {
                _ssbp->cells.at(_animation->initialParts.at(part.index).cellIndex).texturePath = relativePath.string();
                _ssbp->cells.at(_animation->initialParts.at(part.index).cellIndex).textureName = relativePath.stem().string();
            }
}

void ASsbpViewer::replace(const std::string &name, const std::filesystem::path &ssbppath, const std::string &anim)
{
    auto partIt = std::find_if(_animpack->parts.begin(), _animpack->parts.end(), [&name](const Part &part) { return part.name == name; });
    if (partIt == _animpack->parts.end()) return;
    try {
        Ssbp &ssbp = Ssbp::create(ssbppath.string());
        std::string packName = anim.substr(0, anim.find('/'));
        auto packIt = std::find_if(ssbp.animePacks.begin(), ssbp.animePacks.end(), [&packName](const AnimePack &pack) { return pack.name == packName; });
        if (packIt == ssbp.animePacks.end()) throw std::range_error("");
        std::string animName = anim.substr(anim.find('/')+1);
        auto animIt = std::find_if(packIt->animations.begin(), packIt->animations.end(), [&animName](const Animation &anim) { return anim.name == animName; });
        if (animIt == packIt->animations.end()) throw std::range_error("");
        partIt->_anime = &ssbp;
    } catch (const std::invalid_argument &e) {
        std::cerr << "Failed to open requested SSBP file: " << ssbppath << std::endl; return;
    } catch (const std::range_error &e) {
        std::cerr << "Failed to access requested animation: " << anim << std::endl; return;
    }
    partIt->type = PartType::Instance;
    partIt->extAnime = anim;
    _partsAnime[partIt->index] = SsbpPlayer(*partIt->_anime);
    _partsAnime[partIt->index].play(anim);
}
