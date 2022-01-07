#pragma once

#include <cstdint>

template<typename T>
struct vec2T_t { T x; T y; };
template<typename T>
struct vec3T_t { T x; T y; T z; };

typedef uint32_t offset_t;
typedef offset_t string_t;
typedef vec2T_t<uint16_t> vec2_t;
typedef vec3T_t<uint16_t> vec3_t;
typedef vec2T_t<float> vec2f_t;
typedef vec3T_t<float> vec3f_t;
typedef struct {uint8_t a, r, g, b;} color_t;

enum PartType : int16_t {
    Invalid = -1,
    Null,
    Normal,
    Text,
    Instance,
    Effect
};

enum BlendType : uint8_t {
    Mix,
    Mul,
    Add,
    Sub
};

struct SsbpData {
    uint8_t     magickWord[4];
    uint32_t    version;
    uint32_t    flags;
    string_t    imageBaseDir;
    offset_t    cellDataArray;
    offset_t    animePackDataArray;
    offset_t    effectFileDataArray;
    int16_t     cellSize;
    int16_t     animePackSize;
    int16_t     effectFileSize;
};

struct CellData {
    string_t    name;
    offset_t    cellMapData;
    int16_t     cellMapIndex;
    vec2_t      pos;
    vec2_t      size;
    int16_t     _;
    vec2f_t     pivot;
};

struct CellMapData {
    string_t    name;
    string_t    imagePath;
    int16_t     index;
    int16_t     wrapMode;
    int16_t     filterMode;
    int16_t     _;
};

struct AnimePackData {
    string_t    name;
    offset_t    partDataArray;
    offset_t    animeDataArray;
    int16_t     partSize;
    int16_t     animeSize;
};

struct PartData {
    string_t    name;
    int16_t     index;
    int16_t     parentIndex;
    PartType    type;
    int16_t     boundsType;
    int16_t     alphaBlendType;
    int16_t     _refAnimeSize;//TODO
    string_t    animation;
    string_t    effect;
    string_t    colorLabel;
};

struct AnimeData {
    string_t    name;
    offset_t    initDataArray;
    offset_t    frameDataIndexArray;
    offset_t    userDataIndexArray;
    offset_t    labelDataIndexArray;
    int16_t     nbFrame;
    int16_t     fps;
    int16_t     labelSize;
    vec2_t      canvasSize;
    int16_t     _;
};

struct InitData {
    int16_t     index;
    int16_t     _;
    int32_t     flags;
    int16_t     cellIndex;
    vec3_t      pos;
    int16_t     opacity;
    int16_t     __;
    vec2f_t     pivot;
    vec3f_t     rotation;
    vec2f_t     scale;
    vec2f_t     size;
    vec2f_t     textureShift;
    float       textureRotation;
    vec2f_t     textureScale;
    float       _boundingRadius;
};

#include <optional>
struct FrameData {
    int16_t     index;
    uint32_t    flags;
    bool        invisible;
    bool        flipX;
    bool        flipY;
    std::optional<int16_t>          cellIndex;
    vec3T_t<std::optional<int16_t>> pos;
    vec2T_t<std::optional<float>>   pivot;
    vec3T_t<std::optional<float>>   rotation;
    vec2T_t<std::optional<float>>   scale;
    std::optional<uint16_t>         opacity;
    vec2T_t<std::optional<float>>   size;
    vec2T_t<std::optional<float>>   textureShift;
    std::optional<float>            textureRotation;
    vec2T_t<std::optional<float>>   textureScale;
    std::optional<float>            _boundingRadius;

    std::optional<vec2T_t<int16_t>> vertexTransformTL;
    std::optional<vec2T_t<int16_t>> vertexTransformTR;
    std::optional<vec2T_t<int16_t>> vertexTransformBL;
    std::optional<vec2T_t<int16_t>> vertexTransformBR;
    std::optional<BlendType>        colorBlend;
    std::optional<float>            blendRate;
    std::optional<color_t>          vertexColorTL;
    std::optional<color_t>          vertexColorTR;
    std::optional<color_t>          vertexColorBL;
    std::optional<color_t>          vertexColorBR;

    std::optional<int16_t>          instanceKeyframe;
    std::optional<int16_t>          instanceStart;
    std::optional<int16_t>          instanceEnd;
    std::optional<float>            instanceSpeed;
    std::optional<int16_t>          instanceNbLoop;
    std::optional<bool>             instanceInfinity;
    std::optional<bool>             instanceReverse;
    std::optional<bool>             instancePingpong;
    std::optional<bool>             instanceIndependent;
};

struct LabelData {
    string_t    label;
    int32_t     frame;
};