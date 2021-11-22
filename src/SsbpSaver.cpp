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
    setBackgroundType(Original);
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
                    if (std::find(savedSprite.begin(), savedSprite.end(), anim.name) != savedSprite.end())
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
    }
#   if (defined(SAVE_SPRITE))
        try { play("body_anim/Idle"); } catch (...) { return; }
        replace("Wep_BaseR", "./no_wep_R.png");
        replace("Wep_BaseR_Add", "./no_wep_R2.png");
        replace("Wep_BaseL", "./no_wep_L.png");
        replace("Wep_BaseL_Add", "./no_wep_L2.png");
        setFrame(0);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        render(false);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        Magick::Image img = saver.screen({width, height});
        saver.save("Screenshots/"+_ssbp->_path.stem().string()+"/Idle_no_wep.png", img, saver.bounds(img));
#   endif
}

#define stringPattern(except) "((?:[^\\s\\\\" except "]|\\\\\\s|\\\\)+)"
#define matchArg(smallArg, longArg, expected) std::regex_search(args, m, std::regex(smallArg[0] ? "^\\s*(?:-" smallArg "\\s*|--" longArg "(?:=\\s*|\\s+))" expected : "^\\s*--" longArg "(?:=\\s*|\\s+)" expected, std::regex_constants::icase))
#define matchUniqArg(smallArg, longArg) std::regex_search(args, m, std::regex("^\\s*(?:-" smallArg "|--" longArg ")\\b", std::regex_constants::icase))
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
        } else if (matchArg("bg", "background", stringPattern(""))) {
            SsbpResource::addTexture("","",m[1]);
            background = &SsbpResource::getTexture("","",m[1]);
        } else if (matchUniqArg("f", "fit")) {
            setBackgroundType(Fit);
        } else if (matchUniqArg("fh","fitHeight")) {
            setBackgroundType(FitHeight);
        } else if (matchUniqArg("fw","fitWidth")) {
            setBackgroundType(FitWidth);
        } else if (matchUniqArg("s","stretch")) {
            setBackgroundType(Stretch);
        } else if (matchUniqArg("o","original")) {
            setBackgroundType(Original);
        } else if (matchArg("", "scale", "(\\d+(?:\\.\\d+)?)(?:x(\\d+(?:\\.\\d+)?))?")) {
            setBackgroundType(Scale, {std::stod(m[1]), std::stod(m[2].matched ? m[2] : m[1])});
        } else if (matchArg("", "size", "(\\d+)x(\\d+)")) {
            setBackgroundType(Size, {std::stol(m[1]), std::stol(m[2])});
        } else if (matchArg("", "shift", "(-?\\d+(?:\\.\\d+)?)x(-?\\d+(?:\\.\\d+)?)(px|%)")) {
            shiftBackground({std::stod(m[1]), std::stod(m[2])}, m[3] == "%");
        } else if (matchArg("p", "position", "(-?\\d+(?:\\.\\d+)?)x(-?\\d+(?:\\.\\d+)?)(px|%)")) {
            mover = glm::vec3{
                std::stod(m[1]) / (m[3] == "%" ? 100 : width) * 2 - 1,
                std::stod(m[2]) / (m[3] == "%" ? 100 : height) * 2 - 1,
                0
            };
            setViewMatrix();
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
                play(_ssbp->animePacks.front().name, _ssbp->animePacks.front().animations.front().name);
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
        if (std::regex_search(_ssbp->_path.stem().string(), std::regex("_PairSub$", std::regex_constants::icase)))
            return std::find(savedPairSub.begin(), savedPairSub.end(), anim) == savedPairSub.end();
        else if (std::regex_search(_ssbp->_path.stem().string(), std::regex("_PairMain$", std::regex_constants::icase)) && anim == "Pairpose")
            return true;
        return false;
#   endif
}