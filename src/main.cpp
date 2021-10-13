#include "ssbpViewer.h"
#include "ssbpResource.h"
#include "Screenshot.hpp"

#include <GLFW/glfw3.h>
#include <Magick++.h>

#include <iostream>

#ifdef _DEBUG
#define debug(s, ...) printf(s, __VA_ARGS__)
#else
#define debug(s, ...) {}
#endif

int main(int n, char **argv)
{
#ifndef _DEBUG
    try {
#endif
        SsbpViewer(n, argv).run();
#ifndef _DEBUG
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
#endif
    return 0;
}
//TODO args

char *_ = R"==(
void handleArguments(int argc, char **argv) {
    for (int i = 1; i != argc; ++i, ++argv) {
        if ((i != argc-1 && strcmp(*argv, "-b") == 0) || strncmp(*argv, "--bind=", 7) == 0) {
            const char *arg;
            if (strncmp(*argv, "--", 2) != 0) {
                arg = *(++argv); ++i;
            } else
                arg = *argv + 7;
            const char *separator = strchr(arg, ':');
            if (separator != nullptr) {
                std::string part(arg, separator - arg);
                std::string ssbp;
                std::string newElem = separator + 1;
                separator = strrchr(newElem.c_str(), ':');
                if (separator != nullptr) {
                    ssbp = newElem.substr(0, separator - newElem.c_str());
                    newElem = separator + 1;
                }
                sprite.overrided_parts[replace(part, "\\", "/")] = {replace(ssbp, "\\", "/"), replace(newElem, "\\", "/")};
            }
        } else if ((i != argc-1 && strcmp(*argv, "-bg") == 0) || strncmp(*argv, "--background=", 13) == 0) {
            const char *arg;
            if (strncmp(*argv, "--", 2) != 0) {
                arg = *(++argv); ++i;
            } else
                arg = *argv + 13;
            backgroundPath = arg;
        } else if ((i != argc-1 && strcmp(*argv, "-s") == 0) || strncmp(*argv, "--stretch=", 10) == 0) {
            const char *arg;
            if (strncmp(*argv, "--", 2) != 0) {
                arg = *(++argv); ++i;
            } else
                arg = *argv + 13;
            backgroundStretch = atoi(arg);
            background.shader.use();
            background.shader.setInt("u_BgType", backgroundStretch);
        } else if ((i != argc-1 && strcmp(*argv, "-p") == 0) || strncmp(*argv, "--position=", 11) == 0) {
            const char *arg;
            if (strncmp(*argv, "--", 2) != 0) {
                arg = *(++argv); ++i;
            } else
                arg = *argv + 11;
            std::cmatch m;
            if (!std::regex_match(arg, m, std::regex(R"((-?\d+|-?\d*\.\d+)(px|%|),(-?\d+|-?\d*\.\d+)(px|%|))"))) {
                std::cerr << "Invalid argument " << arg << " for position" << std::endl;
                continue;
            }
            float v1 = (float)atof(m[1].first);
            float v2 = (float)atof(m[3].first);
            mover.x = v1 / (*m[2].first == 'p' ? windowWidth : *m[2].first == '%' ? 100 : 1) * 2 - 1;
            mover.y = v2 / (*m[4].first == 'p' ? windowHeight : *m[4].first == '%' ? 100 : 1) * 2 - 1;
        } else
            sprite.file_name = replace(*argv, "\\", "/");
    }
    if (sprite.file_name.empty()) {
        std::cout << "Drag an ssbp file here then press enter.\n";
        std::cin >> sprite.file_name;
    }
}

void applyArgument()
{
    for (auto &[part, pair] : sprite.overrided_parts) {
        if (std::regex_search(pair.second, std::regex(R"(\.png$|\.webp$)"))) {
            int textureId = -1;
            for (int i = 0; i < sprite.textures.size() && sprite.textures[i]; ++i)
                if (sprite.textures[i]->file_name == pair.second)
                    textureId = i+1;
            if (textureId == -1)
                textureId = ss::SSTextureLoad(pair.second.c_str(), ss::SsTexWrapMode::clamp, ss::SsTexFilterMode::linear);
            sprite.ssPlayer->setPartTexture(part, textureId, sprite.textures[textureId-1]->width, sprite.textures[textureId-1]->height);
        } else {
            std::string &ssbp = pair.first.empty() ? sprite.file_name : pair.first;
            if (ssbp.length() > 5 && strcmp(ssbp.c_str()+ssbp.length()-5, ".ssbp") == 0)
                ssbp = sprite.resman->addData(replace(ssbp, "\\", "/"));
            ss::Instance param;
            param.clear();
            param.refEndframe = sprite.resman->getMaxFrame(ssbp, pair.second) - 1;
            if (param.refEndframe < 0) continue;
            param.refloopNum = 0;
            param.infinity = true;
            param.independent = true;
            sprite.ssPlayer->setPartAnime(part, ssbp, pair.second, &param);
        }
    }
}
)==";