#pragma once
#include <stdint.h>
#include <stddef.h>

struct size2 {
	int w = 0;
	int h = 0;
};

using byte_t = uint8_t;

struct Args {
    int c = 0;
    char ** v = nullptr;
};

struct WindowLimits {
    // always passed directly into glfwSetWindowSizeLimits
    // these defaults match GLFW_DONT_CARE, which effectively means no constraints are set
    int minw = -1;
    int minh = -1;
    int maxw = -1;
    int maxh = -1;
    int ratioNumer = -1;
    int ratioDenom = -1;
};

struct WindowPlacement {
    int x = -1;
    int y = -1;
    int w = 1920;
    int h = 1080;
};

struct Event {
    enum Type {
        None,
        // Window,
        Key,
        Char,
        MousePos,
        MouseButton,
        MouseScroll
    };

    Type type = None;
    int key = 0;
    int scancode = 0;
    int button = -1;
    int action = -1;
    int mods = -1;
    unsigned int codepoint = 0;
    double x = -1.0;
    double y = -1.0;
    double scrollx = 0.0;
    double scrolly = 0.0;
    int width = 0;
    int height = 0;
    bool consume = false;   // When true will be consumed by first processor.
                            // For example debug UI might consume input events,
                            // so the main game doesn't even see them.
};
