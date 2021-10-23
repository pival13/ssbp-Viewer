#pragma once

#include "SsbpPlayer.h"
#include "Screenshot.h"
#include "SsbpResource.h"

#include <list>

class ASsbpViewer : protected SsbpPlayer {
    public:
        ASsbpViewer();
        virtual ~ASsbpViewer();

        virtual void run() = 0;

    protected:
        inline void setViewMatrix() { SsbpResource::quad.set("u_View", glm::scale(glm::translate(glm::mat4(1), mover), scaler)); }

        void render(bool renderBackground=true);

        void replace(const std::string &name, const std::filesystem::path &texture);
        void replace(const std::string &name, const std::filesystem::path &ssbp, const std::string &animation);
        enum BackgroundType { Fit, FitWidth, FitHeight, Stretch, Original, Scale, Size };
        void setBackgroundType(BackgroundType type);
        void setBackgroundType(BackgroundType type, double x, double y);
        //void setBackgroundShift();

    protected:
        glm::vec3 mover;
        glm::vec3 scaler;
        int width, height;

        const Texture *background;
        Magick::Geometry backgroundSize;

        Saver saver;
};