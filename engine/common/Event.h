#pragma once
#include <stddef.h>

struct Event {
    int key = 0;
    int scancode = 0;
    int checkButton = 0;
    int button = 0;
    int action = 0;
    int mods = 0;
    unsigned int codepoint = 0;
    double x = -1.0;
    double y = -1.0;
    double scrollx = 0.0;
    double scrolly = 0.0;
    int width = 0;
    int height = 0;
};
