#include "SsbpViewer.h"
#include "SsbpSaver.h"

#include <iostream>

int main(int n, char **argv)
{
#ifndef _DEBUG
    try {
#endif
#if defined(SAVE_ANIM) || defined(SAVE_SPRITE)
        SsbpSaver().run();
#else
        SsbpViewer(n, argv).run();
#endif
#ifndef _DEBUG
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
#endif
    return 0;
}