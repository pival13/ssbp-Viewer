#pragma once

#include <string>
#include <filesystem>
#include <vector>

#include "ssbpData.h"

class Cell;
class Part;
class AnimePack;
class Animation;

class Ssbp {
    public:
        static Ssbp &create(const std::string &path);
        static void release(const Ssbp &ssbp);
        Ssbp() = default;
        explicit Ssbp(const Ssbp &) = delete;
        Ssbp &operator=(const Ssbp &) = delete;
        Ssbp &operator=(Ssbp &&r);

    private:
        Ssbp(const std::string &path);

    public:
        std::filesystem::path _path;
        std::string imageBaseDir;
        std::vector<Cell> cells;
        std::vector<AnimePack> animePacks;
        //std::vector<Effect>
};

class AnimePack {
    public:
        AnimePack(uint8_t *data, const AnimePackData &ref);
    
    public:
        std::string name;
        std::vector<Part> parts;
        std::vector<Animation> animations;
};

class Animation {
    public:
        Animation(uint8_t *data, const AnimeData &ref, int nbParts);
    
    public:
        std::string name;
        std::vector<InitData> initialParts;
        std::vector<std::vector<FrameData>> partsPerFrames;
        //std::vector<UserData>
        //std::vector<Label>
        int fps;
        vec2T_t<uint32_t> canvasSize;
};

class Part {
    public:
        Part(uint8_t *data, const PartData &ref);

    public:
        std::string name;
        int index;
        Part *parent;
        PartType type;
        BlendType blend;
        Ssbp *_anime;
        std::string extAnime;
        std::string extEffect;
        std::string color;
};

class Cell {
    public:
        Cell(uint8_t *data, const CellData &ref);

    public:
        std::string name;
        int16_t cellIndex; // relativ to the texture
        std::string textureName;
        std::string texturePath;
        int16_t textureIndex;
        vec2_t pos;
        vec2_t size;
        vec2f_t pivot;
};