
#include <iostream>

#ifdef _DEBUG
#  define TRY try
#  define CATCH catch (const std::exception &e) { std::cerr << e.what() << std::endl; return 1; }
#else
#  define TRY
#  define CATCH
#endif

#if !defined(SAVE_ANIM) && !defined(SAVE_SPRITE)

#include "SsbpViewer.h"

int main(int n, char **argv)
{
    TRY {
        SsbpViewer(n, argv).run();
    } CATCH
    return 0;
}

#else // defined(SAVE_ANIM) || defined(SAVE_SPRITE)

#include "SsbpSaver.h"

int main(int n, char **argv)
{
    TRY {
        SsbpSaver().run();
    } CATCH
    return 0;
}

#endif
