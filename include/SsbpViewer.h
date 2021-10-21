#pragma once

#include "ASsbpViewer.h"

class SsbpViewer : public ASsbpViewer {
    public:
        SsbpViewer(int argc, char **argv);
        ~SsbpViewer();

        void run() override;

    private:
        void handleArguments(int argc, char **argv);

        void resizeCallback(int w, int h);
        void scrollCallback(double y);
        void keyCallback(int key, int scancode, int action, int modifier);
        void handleEvents();

    private:
        double time;
        glm::dvec2 mouse;
};