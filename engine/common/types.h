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
    float x = -1.0;
    float y = -1.0;
    float scrollx = 0.0;
    float scrolly = 0.0;
    int width = 0;
    int height = 0;
    bool consume = false;   // When true will be consumed by first processor.
                            // For example debug UI might consume input events,
                            // so the main game doesn't even see them.
};

enum MemBlockType {
    // unsued
    MEM_BLOCK_FREE,    // empty space to be claimed
    MEM_BLOCK_CLAIMED, // claimed but type not set yet
    // Special Fixed Sized Allocator type
    // if FSA blocks are configured at init, special checks will happen at new allocation/lookup
    MEM_BLOCK_FSA,
    // internal types, with special treatment
    MEM_BLOCK_POOL,
    MEM_BLOCK_STACK,
    MEM_BLOCK_FILE,
    MEM_BLOCK_ENTITY,
    // requested by BGFX. (no special treatment atm)
    MEM_BLOCK_BGFX,
    // externally requested of any type
    MEM_BLOCK_EXTERNAL
};

struct MemBlockSetup {
    size_t size = 0;
    MemBlockType type = MEM_BLOCK_FREE;
    uint8_t subBlockSize = 0;
    uint16_t nSubBlocks = 0;
};
