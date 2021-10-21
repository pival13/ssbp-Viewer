#include <iostream>
#include <regex>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "SsbpSaver.h"

SsbpSaver::SsbpSaver()
{
    glClearColor(0,0,0,0);
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glGenRenderbuffers(1, &renderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, renderbuffer);
}

SsbpSaver::~SsbpSaver()
{
    glDeleteRenderbuffers(1, &renderbuffer);
    glDeleteFramebuffers(1, &framebuffer);
}

void SsbpSaver::run()
{
    std::chrono::time_point time = std::chrono::system_clock::from_time_t(0);
    while (true) {
        std::string arg;
        std::getline(std::cin, arg);
        if (std::cin.eof()) break;
        handleArguments(arg);
        std::this_thread::sleep_until(time + std::chrono::seconds(1));
        if (_ssbp) {
            try {
                saveAnimations();
            } catch (const std::exception &e) {
                std::cerr << "Error while saving animations: " << e.what() << std::endl;
            }
            time = std::chrono::system_clock::now();
        }
        _ssbp = nullptr;
    }
}

void SsbpSaver::saveAnimations()
{
    using path = std::filesystem::path;
    for (const AnimePack &pack : _ssbp->animePacks) {
        if (pack.name != "body_anim") continue;
        for (const Animation &anim : pack.animations) {
            if (shouldIgnoreAnim(anim.name)) continue;
            bool isLooping = std::find(loopingAnimation.begin(), loopingAnimation.end(), anim.name) != loopingAnimation.end();
            play(pack.name, anim.name, false);
#           if (defined(SAVE_SPRITE))
                size_t savedFrame = getMaxFrame()-1;
                if (anim.name == "Cheer")
                    savedFrame /= 2;
                else if (isLooping || (anim.name == "Transform" && std::regex_search(_ssbp->_path.string(), std::regex("Dragon|TransBattle|TransMap", std::regex_constants::icase))))
                    savedFrame = 0;
#           endif
#           if (defined(SAVE_ANIM))
                std::vector<Magick::Image> imagesRGB; imagesRGB.reserve(getMaxFrame());
                std::vector<Magick::Image> imagesRGBA; imagesRGBA.reserve(getMaxFrame());
                std::vector<Magick::Geometry> bounds; bounds.reserve(getMaxFrame());
                for (size_t frame = 0; frame < getMaxFrame(); ++frame) {
                    setFrame(frame);
                    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
                    render(false);
                    glReadBuffer(GL_COLOR_ATTACHMENT0);
                    imagesRGBA.emplace_back(saver.screen({width, height}));
                    bounds.emplace_back(saver.bounds(imagesRGBA.at(frame)));
                    if (background && background->loaded) {
                        render(true);
                        imagesRGB.emplace_back(saver.screen({width, height}));
                    } else
                        imagesRGB.emplace_back(imagesRGBA.at(frame));
                    imagesRGB.at(frame).animationDelay(100 * (frame+1) / getFps() - 100 * frame / getFps());
                    imagesRGB.at(frame).gifDisposeMethod(MagickCore::DisposeType::BackgroundDispose);
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                }
#               if (defined(SAVE_SPRITE))
                    if (std::find(savedSprite.begin(), savedSprite.end(), anim) != savedSprite.end())
                        saver.save("Screenshots/"+_ssbp->_path.stem().string()+"/"+anim.name+"_"+std::to_string(savedFrame)+".png", imagesRGBA.at(savedFrame), bounds.at(savedFrame));
#               endif
                saver.save("Screenshots/"+_ssbp->_path.stem().string()+"/"+anim.name+".gif", imagesRGB, saver.bounds(bounds), isLooping ? Saver::Loop : Saver::SlowLoop);
#           elif (defined(SAVE_SPRITE))
                setFrame(savedFrame);
                glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
                render(false);
                glReadBuffer(GL_COLOR_ATTACHMENT0);
                Magick::Image img = saver.screen({width, height});
                saver.save("Screenshots/"+_ssbp->_path.stem().string()+"/"+anim.name+"_"+std::to_string(savedFrame)+".png", img, saver.bounds(img));
#           endif
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        //No wep
    }
#   if (defined(SAVE_SPRITE))
        try { play("body_anim/Idle"); } catch (...) { return; }
        replace("Wep_BaseR", "no_wep_R.png");
        replace("Wep_BaseR_Add", "no_wep_R2.png");
        replace("Wep_BaseL", "no_wep_L.png");
        replace("Wep_BaseL_Add", "no_wep_L2.png");
        setFrame(0);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        render(false);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        Magick::Image img = saver.screen({width, height});
        saver.save("Screenshots/"+_ssbp->_path.stem().string()+"/Idle_no_wep.png", img, saver.bounds(img));
#   endif
}

#define stringPattern(except) "((?:[^\\s\\\\" except "]|\\\\\\s|\\\\)+)"
#define matchArg(smallArg, longArg, expected) std::regex_search(args, m, std::regex("^\\s*(?:-" smallArg "\\s*|--" longArg "(?:=\\s*|\\s+))" expected, std::regex_constants::icase))
#define matchPrefix(prefix, expected) std::regex_search(args, m, std::regex("^\\s*" prefix "\\s*" expected, std::regex_constants::icase))
void SsbpSaver::handleArguments(std::string args)
{
    std::smatch m;
    while (!std::regex_match(args, std::regex("\\s*"))) {
        if        (matchArg("w", "width", "(\\d+)")) {
            width = std::stol(m[1]);
            glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, width, height);
            glViewport(0, 0, width, height);
            scaler = glm::vec3(2.f / width, 2.f / height, 1);
            setViewMatrix();
        } else if (matchArg("h", "height", "(\\d+)")) {
            height = std::stol(m[1]);
            glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, width, height);
            glViewport(0, 0, width, height);
            scaler = glm::vec3(2.f / width, 2.f / height, 1);
            setViewMatrix();
        } else if (matchArg("bg", "background", "(.+)")) {
            SsbpResource::addTexture("","",m[1]);
            background = &SsbpResource::getTexture("","",m[1]);
        /*} else if (std::regex_search(tmp, m, argPattern("s", "stretch"))) {
            tmp = m.suffix();
            background.shader.use();
            background.shader.setInt("u_BgType", stoul(m[1]));
        } else if (std::regex_search(tmp, m, argPattern("p", "position"))) {
            tmp = m.suffix();
            if (background.texture && m[2].first[0] == 'p' && m[4].first[0] == 'p') {
                background.shader.use();
                background.shader.setVec2("u_Shift",
                    (stof(m[1]) - 0.5 * windowWidth) / background.texture->width,
                    (-stof(m[3]) - 0.75 * windowHeight) / background.texture->height);
            } else {
                std::cout << "Unsupported offset" << std::endl;
            }*/
        } else if (matchPrefix("~", "([^,\\s]+)," stringPattern(",") "(?:,(\\S+))?")) {
            if (!m[3].matched)
                replace(m[1].str(), m[2].str());
            else
                replace(m[1].str(), m[2].str(), m[3].str());
        //} else if (matchPrefix("+", "([^,]+),([^,]+),([^,]+),(\\d+),(\\d+),(\\d+),(\\d+),(\\d+)")) {
        //    // Add arg
        //    // parentName,newName,texturePath,posX,posY,rotate,scaleX,scaleY
        } else if (std::regex_search(args, m, std::regex("\\s*(\\S+\\.ssbp)\\b"))) {
            try {
                _ssbp = &Ssbp::create(m[1]);
            } catch (const std::invalid_argument &e) {
                std::cerr << "Invalid SSBP file: " << m[1] << std::endl;
            }
        } else {
            std::regex_search(args, m, std::regex("\\s*" stringPattern("")));
            std::cerr << "Invalid argument \"" << m[1] << "\"" << std::endl;
        }
        args = m.suffix();
    }
}

bool SsbpSaver::shouldIgnoreAnim(const std::string &anim) const
{
#   if (!defined(SAVE_ANIM) && !defined(SAVE_SPRITE))
        return true;
#   else
#       if (!defined(SAVE_ANIM))
            if (std::find(savedSprite.begin(), savedSprite.end(), anim) == savedSprite.end())
                return true;
#       endif
        if (std::regex_search(_ssbp->_path.string(), std::regex("_PairSub$", std::regex_constants::icase)))
            return std::find(savedPairSub.begin(), savedPairSub.end(), anim) == savedPairSub.end();
        return false;
#   endif
}