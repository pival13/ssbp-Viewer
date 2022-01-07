#pragma once

#include "SsbpPlayer.h"
#include "Screenshot.hpp"
#include "SsbpResource.h"

#include <list>

class SsbpViewer : protected SsbpPlayer {
    public:
        SsbpViewer(int argc, char **argv);
        ~SsbpViewer();

        void run();

    private:
        inline void setViewMatrix() { SsbpResource::quad.set("u_View", glm::scale(glm::translate(glm::mat4(1), mover), scaler)); }

        void handleArguments(int argc, char **argv);
        void handleArguments(std::string args);
        void argumentReplace(const std::string &arg);

        void resizeCallback(int w, int h);
        void scrollCallback(double y);
        void keyCallback(int key, int scancode, int action, int modifier);
        void handleEvents();
        void render(bool renderBackground=true, bool swapBuffer=true);

    private:
        glm::vec3 mover;
        glm::vec3 scaler;
        int width, height;
        double time;
        glm::dvec2 mouse;

        Saver saver;
        const Texture *background;
};