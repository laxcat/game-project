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
    // special types
    MEM_BLOCK_FREE,     // empty space to be claimed
    MEM_BLOCK_REQUEST,  // Special block for allocation request/result parameters; configured at init
    MEM_BLOCK_FSA,      // Special Fixed Sized Allocator block; configured at init

    // internal container types, with special treatment
    MEM_BLOCK_CLAIMED,  // claimed but type not set yet or generic
    MEM_BLOCK_ARRAY,
    MEM_BLOCK_CHARKEYS,
    MEM_BLOCK_FILE,
    MEM_BLOCK_FRAMESTACK,
    MEM_BLOCK_FREELIST,
    MEM_BLOCK_GOBJ,
    MEM_BLOCK_POOL,

    // requested by BGFX. (no special treatment atm)
    MEM_BLOCK_BGFX,

    // unspecified request of any type
    MEM_BLOCK_GENERIC,

    // filtering and admin
    MEM_BLOCK_FILTER_ALL,
};

struct MemManFSASetup {
    static constexpr size_t Max = 12;
    union {
        struct {
            uint16_t n2byteSubBlocks;
            uint16_t n4byteSubBlocks;
            uint16_t n8byteSubBlocks;
            uint16_t n16byteSubBlocks;
            uint16_t n32byteSubBlocks;
            uint16_t n64byteSubBlocks;
            uint16_t n128byteSubBlocks;
            uint16_t n256byteSubBlocks;
            uint16_t n512byteSubBlocks;
            uint16_t n1024byteSubBlocks;
            uint16_t n2048byteSubBlocks;
            uint16_t n4096byteSubBlocks;
        };
        uint16_t nSubBlocks[Max] = {0};
    };
    size_t align = 0;
};
