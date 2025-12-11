#pragma once

#include "SsbpPlayer.h"
#include "tools/ImageSaver.h"

class ASsbpViewer : protected SsbpPlayer {
    public:
        ASsbpViewer();
        virtual ~ASsbpViewer();

        virtual void run() = 0;

    protected:
        void setViewMatrix();

        void render(bool renderBackground=true, bool swapBuffer=true);

        void replace(const std::string &name, const std::filesystem::path &texture);
        void replace(const std::string &name, const std::filesystem::path &ssbp, const std::string &animation);
        enum BackgroundType { Fit, FitWidth, FitHeight, Stretch, Original, Scale, Size };
        void setBackgroundType(BackgroundType type);
        void setBackgroundType(BackgroundType type, const glm::vec2 &size);
        void shiftBackground(const glm::vec2 &shift, bool isPercent);

    protected:
        glm::vec3 mover;
        glm::vec3 scaler;
        int width, height;

        const struct GLTexture *background;
        Magick::Geometry backgroundSize;

        Saver *saver;

    public:
        static void addTexture(const std::filesystem::path &ssbpPath, const std::string &imageBaseDir, const std::string &texturePath);
        static const GLTexture &getTexture(const std::filesystem::path &ssbpPath, const std::string &imageBaseDir, const std::string &texturePath);

        static struct GLFWwindow *window;
        static struct GLQuad *quad;
        static std::map<std::string, struct Ssbp> _ssbps;
        static std::map<std::string, const GLTexture> _textures;
};
