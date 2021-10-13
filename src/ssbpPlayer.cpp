#include "ssbpPlayer.h"
#include "ssbpResource.h"
#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <algorithm>

SsbpPlayer::SsbpPlayer(const Ssbp &ssbp) : _ssbp(&ssbp) {}
SsbpPlayer::~SsbpPlayer() {}
SsbpPlayer &SsbpPlayer::operator=(const Ssbp &ssbp) { _ssbp = &ssbp; _animpack = nullptr; _animation = nullptr; _partsAnime.clear(); return *this; }

void SsbpPlayer::play(const std::string &anime, bool loop, int initFrame, int endFrame) { size_t p = anime.find('/'); play(anime.substr(0, p), anime.substr(p+1), loop, initFrame, endFrame); }
void SsbpPlayer::play(const std::string &pack, const std::string &name, bool loop_, int initframe, int endframe)
{
    _animpack = nullptr;
    _animation = nullptr;
    for (const auto &animpack : _ssbp->animePacks)
        if (animpack.name == pack) {
            _animpack = &animpack; break; }
    if (_animpack == nullptr) throw std::invalid_argument("No such animation pack: " + pack);
    for (const auto &anim : _animpack->animations)
        if (anim.name == name) {
            _animation = &anim; break; }
    if (_animation == nullptr) throw std::invalid_argument("No such animation: " + name);
    start = initframe;
    end = endframe;
    _t = 0;
    pause = false;
    loop = loop_;
    pingpong = false;
    reverse = false;
    for (const Cell &cell : _ssbp->cells)
        SsbpResource::addTexture(_ssbp->_path, _ssbp->imageBaseDir, cell.texturePath);
    _partsAnime.clear();
    for (const Part &part : _animpack->parts)
        if (part.type == PartType::Instance) {
            _partsAnime[part.index] = SsbpPlayer(*_ssbp);
            _partsAnime[part.index].play(part.extAnime);
        }
    update();
}

void SsbpPlayer::setFrame(size_t frame, bool force)
{
    _t = float(frame) / _animation->fps;
    update();
    if (force)
        for (auto &[_,player] : _partsAnime)
            player.setFrame(frame, force);
}

size_t SsbpPlayer::getFrame() const
{
    size_t frame = size_t(_t * _animation->fps);
    size_t maxframe = end < 0 ? getMaxFrame()-1 : end;
    bool rev = reverse;
    if (pingpong && frame + start > maxframe) { rev = !rev; frame-=maxframe+start; }
    frame = frame % (maxframe+1-start);
    return rev ? maxframe - frame : start + frame;
}

void SsbpPlayer::update(float dt)
{
    if (pause) return;
    _t += dt * speed;
    float maxframe = float((end < 0 ? getMaxFrame()-1 : end) - start) * (pingpong ? 2 : 1);
    if (_t * _animation->fps > maxframe+1 || _t < 0) {
        if (loop)
            _t -= maxframe / _animation->fps * (speed < 0 ? -1 : 1);
        else {
            _t -= dt * speed;
            return;
        }
    }
    update();
    for (auto &[_,player] : _partsAnime)
        player.update(dt);
}

void SsbpPlayer::update()
{
    size_t frame = getFrame();
    const std::vector<Part> &parts = _animpack->parts;
    const std::vector<InitData> &initPartsData = _animation->initialParts;
    const std::vector<FrameData> &partsData = _animation->partsPerFrames.at(frame);

    _matrices.resize(parts.size());
    for (int i = 0; i != parts.size(); ++i) {
        auto &initPartData = *std::find_if(initPartsData.begin(), initPartsData.end(), [i](const InitData &part) {return part.index == i;});
        auto &partData = *std::find_if(partsData.begin(), partsData.end(), [i](const FrameData &part) {return part.index == i;});

        int cellIndex = partData.cellIndex.value_or(initPartData.cellIndex);
        glm::mat4 mat = parts.at(i).parent ? _matrices[parts.at(i).parent->index] : glm::mat4(1);
        mat = glm::translate(mat, {
            partData.pos.x.value_or(initPartData.pos.x)/10.f,
            partData.pos.y.value_or(initPartData.pos.y)/10.f,
            0
        });
        mat = glm::rotate(mat, partData.rotation.z.value_or(initPartData.rotation.z) * glm::pi<float>() / 180, {0,0,1});
        mat = glm::scale(mat, {
            partData.scale.x.value_or(initPartData.scale.x),
            partData.scale.y.value_or(initPartData.scale.y),
            1
        });
        _matrices[i] = mat;

        if (parts.at(i).type == PartType::Instance) {
            SsbpPlayer &player = _partsAnime.at(partData.index);
            if (partData.instanceStart) player.start = *partData.instanceStart;
            if (partData.instanceEnd) player.end = *partData.instanceEnd;
            if (partData.instanceInfinity) player.loop = *partData.instanceInfinity;
            if (partData.instancePingpong) player.pingpong = *partData.instancePingpong;
            if (partData.instanceReverse) player.reverse = *partData.instanceReverse;
        }
    }
    
}

void SsbpPlayer::draw(float posX, float posY, float rotation, float scaleX, float scaleY)
{
    draw(glm::scale(glm::rotate(glm::translate(glm::mat4(1), {posX, posY, 0}), rotation, {0,0,1}), {scaleX, scaleY, 1}));
}

void SsbpPlayer::draw(const glm::mat4 &mat)
{
    size_t frame = getFrame();
    const std::vector<Part> &parts = _animpack->parts;
    const std::vector<InitData> &initPartsData = _animation->initialParts;
    const std::vector<FrameData> &partsData = _animation->partsPerFrames.at(frame);

    for (size_t i = 0; i != parts.size(); ++i) {
        auto &partData = partsData.at(i);
        auto &initPartData = *std::find_if(initPartsData.begin(), initPartsData.end(), [partData](const InitData &part) {return part.index == partData.index;});
        auto &matrix = _matrices.at(initPartData.index) * mat;

        if (parts.at(partData.index).type == PartType::Null || partData.invisible || partData.opacity.value_or(initPartData.opacity) == 0) {
            continue;
        } else if (parts.at(partData.index).type == PartType::Normal) {
            int cellIndex = partData.cellIndex.value_or(initPartData.cellIndex);
            int opacity = partData.opacity.value_or(initPartData.opacity);
            if (cellIndex == -1 || opacity == 0 || partData.invisible) continue;
            drawCell(_ssbp->cells[cellIndex], matrix, partData);
        } else if (parts.at(partData.index).type == PartType::Instance) {
            _partsAnime.at(partData.index).draw(matrix);
        } else {
            std::cerr << "Unsupported part type: " << getFullAnimeName() << ", frame " << frame << ", part " << parts.at(partData.index).name << " (" << parts.at(partData.index).type << ")" << std::endl;
        }
    }
}

void SsbpPlayer::drawCell(const Cell &cell, const glm::mat4 &mat, const FrameData &data)
{
    const InitData &initPartData = *std::find_if(_animation->initialParts.begin(), _animation->initialParts.end(), [&data](const InitData &part) {return part.index == data.index;});
    const Texture &texture = SsbpResource::getTexture(_ssbp->_path, _ssbp->imageBaseDir, cell.texturePath);
    if (!texture.loaded) return;
    SsbpResource::quad.set("u_Texture", texture);
    
    BlendType blending = data.colorBlend.value_or(BlendType::Mix);
    if (blending != Mix) {
        SsbpResource::quad.set("u_BlendType", blending);
        if (blending == Add)
            glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        else
            std::cerr << "Unsupported blending on: " << getFullAnimeName() << ", frame " << getFrame() << ", part " << _animpack->parts.at(data.index).name << ": " << blending << std::endl;
    }

    if (data.textureRotation.value_or(initPartData.textureRotation) != 0 ||
        data.textureShift.x.value_or(initPartData.textureShift.x) != 0 ||
        data.textureShift.y.value_or(initPartData.textureShift.y) != 0 ||
        data.textureScale.x.value_or(initPartData.textureScale.x) != 1 ||
        data.textureScale.y.value_or(initPartData.textureScale.y) != 1)
            std::cerr << "Unsupported transform on texture: " << getFullAnimeName() << ", frame " << getFrame() << ", part " << _animpack->parts.at(data.index).name << std::endl;

    float width = data.size.x.value_or(initPartData.size.x);
    float height = data.size.y.value_or(initPartData.size.y);
    float offsetX = (data.pivot.x.value_or(initPartData.pivot.x) + cell.pivot.x * (data.flipX ? -1 : 1)) * width;
    float offsetY = (data.pivot.y.value_or(initPartData.pivot.y) + cell.pivot.y * (data.flipY ? -1 : 1)) * height;
    std::array<glm::vec3, 4> vertex = {
        mat * (glm::vec4(-width / 2 - offsetX, height / 2 + offsetY, 0, 1) + (data.vertexTransformTL ? glm::vec4(data.vertexTransformTL->x, data.vertexTransformTL->y, 0, 0) : glm::vec4(0))),
        mat * (glm::vec4(-width / 2 - offsetX, -height / 2 + offsetY, 0, 1) + (data.vertexTransformBL ? glm::vec4(data.vertexTransformBL->x, data.vertexTransformBL->y, 0, 0) : glm::vec4(0))),
        mat * (glm::vec4(width / 2 - offsetX, height / 2 + offsetY, 0, 1) + (data.vertexTransformTR ? glm::vec4(data.vertexTransformTR->x, data.vertexTransformTR->y, 0, 0) : glm::vec4(0))),
        mat * (glm::vec4(width / 2 - offsetX, -height / 2 + offsetY, 0, 1) + (data.vertexTransformBR ? glm::vec4(data.vertexTransformBR->x, data.vertexTransformBR->y, 0, 0) : glm::vec4(0))),
    };
    std::array<glm::vec2, 4> uvs = {
        glm::vec2{float(cell.pos.x + (data.flipX ? cell.size.x : 0)) / texture.width, float(cell.pos.y + (data.flipY ? cell.size.y : 0)) / texture.height},
        glm::vec2{float(cell.pos.x + (data.flipX ? cell.size.x : 0)) / texture.width, float(cell.pos.y + (data.flipY ? 0 : cell.size.y)) / texture.height},
        glm::vec2{float(cell.pos.x + (data.flipX ? 0 : cell.size.x)) / texture.width, float(cell.pos.y + (data.flipY ? cell.size.y : 0)) / texture.height},
        glm::vec2{float(cell.pos.x + (data.flipX ? 0 : cell.size.x)) / texture.width, float(cell.pos.y + (data.flipY ? 0 : cell.size.y)) / texture.height},
    };
    std::array<glm::vec4, 4> colors = {
        (data.vertexColorTL ? glm::vec4{data.vertexColorTL->r, data.vertexColorTL->g, data.vertexColorTL->b, data.vertexColorTL->a} : glm::vec4{0xFF,0xFF,0xFF,0xFF}) / float(0xFF) * float(data.opacity.value_or(0xFF)) / float(0xFF),
        (data.vertexColorBL ? glm::vec4{data.vertexColorBL->r, data.vertexColorBL->g, data.vertexColorBL->b, data.vertexColorBL->a} : glm::vec4{0xFF,0xFF,0xFF,0xFF}) / float(0xFF) * float(data.opacity.value_or(0xFF)) / float(0xFF),
        (data.vertexColorTR ? glm::vec4{data.vertexColorTR->r, data.vertexColorTR->g, data.vertexColorTR->b, data.vertexColorTR->a} : glm::vec4{0xFF,0xFF,0xFF,0xFF}) / float(0xFF) * float(data.opacity.value_or(0xFF)) / float(0xFF),
        (data.vertexColorBR ? glm::vec4{data.vertexColorBR->r, data.vertexColorBR->g, data.vertexColorBR->b, data.vertexColorBR->a} : glm::vec4{0xFF,0xFF,0xFF,0xFF}) / float(0xFF) * float(data.opacity.value_or(0xFF)) / float(0xFF),
    };
    SsbpResource::quad.draw(vertex, uvs, colors);

    if (blending != Mix) {
        SsbpResource::quad.set("u_BlendType", Mix);
        glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    }
}