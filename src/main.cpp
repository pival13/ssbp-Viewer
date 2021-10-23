#if !defined(SAVE_ANIM) && !defined(SAVE_SPRITE)
#   include "SsbpViewer.h"
#else
#   include "SsbpSaver.h"
#endif

#include <iostream>

int main(int n, char **argv)
{
#ifndef _DEBUG
    try {
#endif
#if !defined(SAVE_ANIM) && !defined(SAVE_SPRITE)
        SsbpViewer(n, argv).run();
#else
        SsbpSaver().run();
#endif
#ifndef _DEBUG
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
#endif
    return 0;
}