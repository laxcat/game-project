#include "platform.h"
#include "../dev/print.h"

#if PLATFORM_LINUX
    #include <EGL/egl.h>
#elif PLATFORM_MAC
    #include <Cocoa/Cocoa.h>
#endif


void * getGLContext() {
    #if PLATFORM_LINUX
        // untested, but something like this.
        void * ret = eglGetCurrentContext();
        print("did this run, really? %p\n", ret);
        return ret;
    #elif PLATFORM_MAC
        return (void *)[NSOpenGLContext currentContext];
    #endif // PLATFORM_*
    return nullptr;
}
