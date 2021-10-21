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

        void replace(const std::string &name, const std::filesystem::path &texture);
        void replace(const std::string &name, const std::filesystem::path &ssbp, const std::string &animation);

        void render(bool renderBackground=true);

    protected:
        glm::vec3 mover;
        glm::vec3 scaler;
        int width, height;

        Saver saver;
        const Texture *background;
};