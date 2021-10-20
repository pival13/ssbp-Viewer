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