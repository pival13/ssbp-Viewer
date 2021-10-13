#pragma once

#include "ssbpReader.h"
#include <map>
#include <glm/mat4x4.hpp>
#include <glm/ext.hpp>

class SsbpPlayer {
    public:
        SsbpPlayer() = default;
        SsbpPlayer(const Ssbp &ssbp);
        ~SsbpPlayer();
        SsbpPlayer &operator=(const Ssbp &ssbp);
    
    public:
        void play(const std::string &pack, const std::string &anime, bool loop=true, int startFrame=0, int endFrame=-1);
        void play(const std::string &anime, bool loop=true, int startFrame=0, int endFrame=-1);
        void update(float dt);
        void setFrame(size_t frame, bool setIndependent=true);
        void draw(float posX=0, float posY=0, float rotation=0, float scaleX=1, float scalY=1);
        void draw(const glm::mat4 &mat);

    public:
        inline std::string getFileName() const { return _ssbp->_path.stem().string(); }
        inline std::string getAnimeName() const { return _animation->name; }
        inline std::string getAnimePackName() const { return _animpack->name; }
        inline std::string getFullAnimeName() const { return _animpack->name + "/" + _animation->name; }
        inline size_t getFps() const { return _animation->fps; }
        size_t getFrame() const;
        inline size_t getMaxFrame() const { return _animation->partsPerFrames.size(); }

    private:
        void update();
        void drawCell(const Cell &cell, const glm::mat4 &mat, const FrameData &data);

    public:
        float speed = 1.f;
        bool pause = false;
        bool loop = false;
        bool pingpong = false;
        bool reverse = false;
        int start = 0;
        int end = -1;

    protected:
        const Ssbp *_ssbp;
        const AnimePack *_animpack;
        const Animation *_animation;
        float _t;
        std::map<size_t, SsbpPlayer> _partsAnime;
    
    private:
        std::vector<glm::mat4> _matrices;
};