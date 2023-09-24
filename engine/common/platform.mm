#include "platform.h"
#include "glfw.h"

#if PLATFORM_LINUX
    #include <EGL/egl.h>
#elif PLATFORM_MAC
    #include <Cocoa/Cocoa.h>
    #define GLFW_EXPOSE_NATIVE_COCOA
    #define GLFW_NATIVE_INCLUDE_NONE
    #include <GLFW/glfw3native.h>
#endif


void * getGLContext() {
    #if PLATFORM_LINUX
        // untested, but something like this.
        return eglGetCurrentContext();
    #elif PLATFORM_MAC
        fprintf(stderr, "OpenGL deprecated in BGFX for Apple platforms.");
        return 0;
        // return (void *)[NSOpenGLContext currentContext];
    #endif // PLATFORM_*
    return nullptr;
}

void * getNativeWindow(GLFWwindow * window) {
#if PLATFORM_MAC
    return glfwGetCocoaWindow(window);
#elif
    return NULL;
#endif
}
