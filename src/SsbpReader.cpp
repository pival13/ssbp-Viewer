#include "ssbpReader.h"
#include "ssbpResource.h"

#include <fstream>
#include <functional>

Ssbp &Ssbp::create(const std::string &path)
{
    auto it = SsbpResource::_ssbps.find(path);
    if (it != SsbpResource::_ssbps.end())
        return it->second;
    return (SsbpResource::_ssbps[path] = Ssbp(path));
}

void Ssbp::release(const Ssbp &ssbp)
{
    SsbpResource::_ssbps.erase(ssbp._path.string());
}

Ssbp::Ssbp(const std::string &path)
{
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file) throw std::invalid_argument(path);

    std::vector<uint8_t> content;
    file.seekg(0, std::ios::end);
    content.resize(file.tellg());
    file.seekg(0, std::ios::beg);
    file.read((char*)content.data(), content.size());
    file.close();

    uint8_t *data = content.data();
    SsbpData ssbp = *reinterpret_cast<SsbpData*>(data);
    if (std::memcmp(ssbp.magickWord, "SSPB", 4) || ssbp.version != 3)
        throw std::invalid_argument("Invalid SSBP file");

    CellData *cellDatas = reinterpret_cast<CellData*>(data + ssbp.cellDataArray);
    AnimePackData *packDatas = reinterpret_cast<AnimePackData*>(data + ssbp.animePackDataArray);

    _path = path;
    imageBaseDir = ssbp.imageBaseDir ? (char*)data + ssbp.imageBaseDir : "";
    cells.clear();
    if (ssbp.cellSize > 0) {
        cells.reserve(ssbp.cellSize);
        for (size_t i = 0; i < ssbp.cellSize; ++i)
            cells.emplace_back(data, cellDatas[i]);
    }
    animePacks.clear();
    if (ssbp.animePackSize > 0) {
        animePacks.reserve(ssbp.animePackSize);
        for (size_t i = 0; i < ssbp.animePackSize; ++i) {
            AnimePack &pack = animePacks.emplace_back(data, packDatas[i]);
            for (Part &part : pack.parts)
                part._anime = this;
        }
    }
}

Ssbp &Ssbp::operator=(Ssbp &&r)
{
    _path = r._path;
    imageBaseDir = r.imageBaseDir;
    cells = std::move(r.cells);
    animePacks = std::move(r.animePacks);
    for (AnimePack &pack : animePacks)
        for (Part &part : pack.parts)
            if (part._anime == &r)
                part._anime = this;
    return *this;
}

AnimePack::AnimePack(uint8_t *data, const AnimePackData &ref)
{
    PartData *partDatas = reinterpret_cast<PartData*>(data + ref.partDataArray);
    AnimeData *animeDatas = reinterpret_cast<AnimeData*>(data + ref.animeDataArray);
    
    name = ref.name ? (char*)data + ref.name : "";
    parts.clear();
    if (ref.partSize > 0) {
        parts.reserve(ref.partSize);
        for (size_t i = 0; i < ref.partSize; ++i)
            parts.emplace_back(data, partDatas[i]);
        for (size_t i = 0; i < ref.partSize; ++i)
            if (partDatas[i].parentIndex >= 0)
                parts.at(i).parent = &parts.at(partDatas[i].parentIndex);
    }
    animations.clear();
    if (ref.animeSize > 0) {
        animations.reserve(ref.animeSize);
        for (size_t i = 0; i < ref.animeSize; ++i)
            animations.emplace_back(data, animeDatas[i], ref.partSize);
    }
}

Part::Part(uint8_t *data, const PartData &ref)
{
    name = ref.name ? (char*)data + ref.name : "";
    index = ref.index;
    type = (PartType)ref.type;
    extAnime = ref.animation ? (char*)data + ref.animation : "";
    extEffect = ref.effect ? (char*)data + ref.effect : "";
    color = ref.colorLabel ? (char*)data + ref.colorLabel : "";
    parent = nullptr;
    _anime = nullptr;
}

Cell::Cell(uint8_t *data, const CellData &ref)
{
    CellMapData *cellMap = reinterpret_cast<CellMapData*>(data + ref.cellMapData);

    name = ref.name ? (char*)data + ref.name : "";
    cellIndex = ref.cellMapIndex;
    textureName = cellMap->name ? (char*)data + cellMap->name : "";
    texturePath = cellMap->imagePath ? (char*)data + cellMap->imagePath : "";
    textureIndex = cellMap->index;
    std::memcpy(&pos, &ref.pos, sizeof(ref.pos));
    std::memcpy(&size, &ref.size, sizeof(ref.size));
    std::memcpy(&pivot, &ref.pivot, sizeof(ref.pivot));
}

template<typename T>
std::function<T()> readT(uint8_t *&data) { return [&data]() { T n = *reinterpret_cast<T*>(data); data += sizeof(T); return n; }; }

enum {
    PART_FLAG_INVISIBLE         = 1 << 0, 
    PART_FLAG_FLIP_H            = 1 << 1, 
    PART_FLAG_FLIP_V            = 1 << 2, 

    // optional parameter flags
    PART_FLAG_CELL_INDEX        = 1 << 3, 
    PART_FLAG_POSITION_X        = 1 << 4, 
    PART_FLAG_POSITION_Y        = 1 << 5, 
    PART_FLAG_POSITION_Z        = 1 << 6, 
    PART_FLAG_PIVOT_X           = 1 << 7, 
    PART_FLAG_PIVOT_Y           = 1 << 8, 
    PART_FLAG_ROTATION_X         = 1 << 9, 
    PART_FLAG_ROTATION_Y         = 1 << 10,
    PART_FLAG_ROTATION_Z         = 1 << 11,
    PART_FLAG_SCALE_X           = 1 << 12,
    PART_FLAG_SCALE_Y           = 1 << 13,
    PART_FLAG_OPACITY           = 1 << 14,
    PART_FLAG_COLOR_BLEND       = 1 << 15,
    PART_FLAG_VERTEX_TRANSFORM  = 1 << 16,

    PART_FLAG_SIZE_X            = 1 << 17,
    PART_FLAG_SIZE_Y            = 1 << 18,

    PART_FLAG_U_MOVE            = 1 << 19,
    PART_FLAG_V_MOVE            = 1 << 20,
    PART_FLAG_UV_ROTATION       = 1 << 21,
    PART_FLAG_U_SCALE           = 1 << 22,
    PART_FLAG_V_SCALE           = 1 << 23,
    PART_FLAG_BOUNDINGRADIUS    = 1 << 24,

    PART_FLAG_INSTANCE_KEYFRAME = 1 << 25,
    PART_FLAG_INSTANCE_START    = 1 << 26,
    PART_FLAG_INSTANCE_END      = 1 << 27,
    PART_FLAG_INSTANCE_SPEED    = 1 << 28,
    PART_FLAG_INSTANCE_LOOP     = 1 << 29,
    PART_FLAG_INSTANCE_LOOP_FLG = 1 << 30,

    NUM_PART_FLAGS
} partFlags;

enum {
    VERTEX_FLAG_LT = 1 << 0,
    VERTEX_FLAG_RT = 1 << 1,
    VERTEX_FLAG_LB = 1 << 2,
    VERTEX_FLAG_RB = 1 << 3,
    VERTEX_FLAG_ONE = 1 << 4   // color blend only
} vertexFlags;

enum {
    INSTANCE_LOOP_FLAG_INFINITY = 1 << 0,
    INSTANCE_LOOP_FLAG_REVERSE = 1 << 1,
    INSTANCE_LOOP_FLAG_PINGPONG = 1 << 2,
    INSTANCE_LOOP_FLAG_INDEPENDENT = 1 << 3,
} instanceFlags;

Animation::Animation(uint8_t *data, const AnimeData &ref, int nbParts)
{
    InitData *initDatas = reinterpret_cast<InitData*>(data + ref.initDataArray);
    offset_t *frameDatas = reinterpret_cast<offset_t*>(data + ref.frameDataIndexArray);

    name = ref.name ? (char*)data + ref.name : "";
    fps = ref.fps;
    std::memcpy(&canvasSize, &ref.canvasSize, sizeof(ref.canvasSize));
    initialParts.clear();
    if (nbParts > 0) {
        initialParts.resize(nbParts);
        for (size_t i = 0; i < nbParts; ++i)
            std::memcpy(&initialParts.at(i), initDatas+i, sizeof(*initDatas));
    }

    partsPerFrames.clear();
    partsPerFrames.resize(ref.nbFrame);
    for (size_t frame = 0; frame < ref.nbFrame; ++frame) {
        uint8_t *frameData = data + frameDatas[frame];
        auto readS16 = readT<int16_t>(frameData);
        auto readS32 = readT<int32_t>(frameData);
        auto readU8 = readT<uint8_t>(frameData);
        auto readU16 = readT<uint16_t>(frameData);
        auto readU32 = readT<uint32_t>(frameData);
        auto readFloat = readT<float>(frameData);
        auto readVec2 = readT<vec2T_t<int16_t>>(frameData);
        auto readColor = readT<color_t>(frameData);

        partsPerFrames.at(frame).reserve(nbParts);
        for (size_t i = 0; i < nbParts; ++i) {
            FrameData part;
            part.index = readS16();
            part.flags = readU32();
            part.invisible = part.flags & PART_FLAG_INVISIBLE;
            part.flipX = part.flags & PART_FLAG_FLIP_H;
            part.flipY = part.flags & PART_FLAG_FLIP_V;
            if (part.flags & PART_FLAG_CELL_INDEX) part.cellIndex = readS16();
            if (part.flags & PART_FLAG_POSITION_X) part.pos.x = readS16();
            if (part.flags & PART_FLAG_POSITION_Y) part.pos.y = readS16();
            if (part.flags & PART_FLAG_POSITION_Z) part.pos.z = readS16();
            if (part.flags & PART_FLAG_PIVOT_X) part.pivot.x = readFloat();
            if (part.flags & PART_FLAG_PIVOT_Y) part.pivot.y = readFloat();
            if (part.flags & PART_FLAG_ROTATION_X) part.rotation.x = readFloat();
            if (part.flags & PART_FLAG_ROTATION_Y) part.rotation.y = readFloat();
            if (part.flags & PART_FLAG_ROTATION_Z) part.rotation.z = readFloat();
            if (part.flags & PART_FLAG_SCALE_X) part.scale.x = readFloat();
            if (part.flags & PART_FLAG_SCALE_Y) part.scale.y = readFloat();
            if (part.flags & PART_FLAG_OPACITY) part.opacity = readU16();
            if (part.flags & PART_FLAG_SIZE_X) part.size.x = readFloat();
            if (part.flags & PART_FLAG_SIZE_Y) part.size.y = readFloat();
            if (part.flags & PART_FLAG_U_MOVE) part.textureShift.x = readFloat();
            if (part.flags & PART_FLAG_V_MOVE) part.textureShift.y = readFloat();
            if (part.flags & PART_FLAG_UV_ROTATION) part.textureRotation = readFloat();
            if (part.flags & PART_FLAG_U_SCALE) part.textureScale.x = readFloat();
            if (part.flags & PART_FLAG_V_SCALE) part.textureScale.y = readFloat();
            if (part.flags & PART_FLAG_BOUNDINGRADIUS) part._boundingRadius = readFloat();
            if (part.flags & PART_FLAG_VERTEX_TRANSFORM) {
                uint16_t flag = readU16();
                if (flag & VERTEX_FLAG_LT) part.vertexTransformTL = readVec2();
                if (flag & VERTEX_FLAG_RT) part.vertexTransformTR = readVec2();
                if (flag & VERTEX_FLAG_LB) part.vertexTransformBL = readVec2();
                if (flag & VERTEX_FLAG_RB) part.vertexTransformBR = readVec2();
            }
            if (part.flags & PART_FLAG_COLOR_BLEND) {
                part.colorBlend = (BlendType)readU8();
                uint8_t flag = readU8();
                if (flag & VERTEX_FLAG_ONE) {
                    part.blendRate = readFloat();
                    part.vertexColorTL = part.vertexColorTR = part.vertexColorBL = part.vertexColorBR = readColor();
                } else {
                    if (flag & VERTEX_FLAG_LT) throw std::logic_error("Unsupported flag");
                    if (flag & VERTEX_FLAG_LB) throw std::logic_error("Unsupported flag");
                    if (flag & VERTEX_FLAG_RT) throw std::logic_error("Unsupported flag");
                    if (flag & VERTEX_FLAG_RB) throw std::logic_error("Unsupported flag");
                }
            }
            if (part.flags & PART_FLAG_INSTANCE_KEYFRAME) part.instanceKeyframe = readS16();
            if (part.flags & PART_FLAG_INSTANCE_START) part.instanceStart = readS16();
            if (part.flags & PART_FLAG_INSTANCE_END) part.instanceEnd = readS16();
            if (part.flags & PART_FLAG_INSTANCE_SPEED) part.instanceSpeed = readFloat();
            if (part.flags & PART_FLAG_INSTANCE_LOOP) part.instanceNbLoop = readS16();
            if (part.flags & PART_FLAG_INSTANCE_LOOP_FLG) {
                int flag = readS16();
                part.instanceInfinity = flag & INSTANCE_LOOP_FLAG_INFINITY;
                part.instanceReverse = flag & INSTANCE_LOOP_FLAG_REVERSE;
                part.instancePingpong = flag & INSTANCE_LOOP_FLAG_PINGPONG;
                part.instanceIndependent = flag & INSTANCE_LOOP_FLAG_INDEPENDENT;
            }
            partsPerFrames.at(frame).push_back(part);
        }
    }
}
