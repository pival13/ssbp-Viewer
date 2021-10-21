#pragma once

#include "ASsbpViewer.h"

class SsbpSaver : public ASsbpViewer {
    public:
        SsbpSaver();
        ~SsbpSaver();

        void run() override;

    private:
        void handleArguments(std::string args);
        void saveAnimations();

        bool shouldIgnoreAnim(const std::string &anim) const;
    
    private:
        unsigned int framebuffer;
        unsigned int renderbuffer;

        const std::vector<std::string> savedSprite = {"Idle", "Ok", "Ready", "Jump", "Attack1", "Attack2", "AttackF", "Damage", "Pairpose", "Cheer", "Transform"};
        const std::vector<std::string> savedPairSub = {"Idle", "Start", "Ok", "Attack1", "Attack1_Loop", "Damage"};
        const std::vector<std::string> loopingAnimation = {"Idle", "Ok", "Pairpose", "Attack1_Loop", "Attack2_Loop", "AttackF_Loop"};
};