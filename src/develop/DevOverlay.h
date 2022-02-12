#pragma once
#include <bgfx/bgfx.h>
#include "print.h"


class DevOverlay {
public:
    enum States {
        None,
        DearImGUI,
        BGFXStats,
        BGFXDebugText
    };

    void init(size2 windowSize) { 
        charGridSize = {windowSize.w/8, windowSize.h/16};

        cursorY = 0;
        totalLines = 0;
        lineOffset = 0;

        dbgTextBufferSize = (charGridSize.w + 1) * charGridSize.h;
        dbgTextBuffer = (char *)malloc(dbgTextBufferSize);

        // fill buffer with all spaces
        for (size_t i = 0; i < dbgTextBufferSize; ++i) {
            dbgTextBuffer[i] = ' ';
        }
        // add \n at beginning of each line
        for (size_t i = 0; i < dbgTextBufferSize; i += charGridSize.w + 1) {
            dbgTextBuffer[i] = '\0';
        }
    }

    void shutdown() {
        free(dbgTextBuffer);
        dbgTextBuffer = nullptr;
    }

    void setState(int value) {
        state = value;

        // imgui display set by run loop (see MrManager::tick)

        // update bgfx
        if (value == BGFXStats) {
            bgfx::setDebug(BGFX_DEBUG_STATS);
        }
        else if (value == BGFXDebugText) {
            bgfx::setDebug(BGFX_DEBUG_TEXT);
        }
        else {
            bgfx::setDebug(BGFX_DEBUG_NONE);
        }
    }

    void showKeyboardShortcuts() {
        char const * summary = "\n"
        "PRESS    FOR DEBUG OVERLAY         \n"
        "0        none                      \n"
        "1        dear imgui overlay        \n"
        "2        bgfx stats                \n"
        "3        bgfx debgug text output   \n"
        "\n";
        print("%s", summary);
    }

    void tick() {
        if (state == BGFXDebugText) {
            bgfx::dbgTextClear();
            for (int i = 0; i < charGridSize.h; ++i) {
                char * c = dbgTextBuffer + (charGridSize.w + 1) * i;
                bgfx::dbgTextPrintf(0, (i + lineOffset) % charGridSize.h, 0x0f, "%s", c);
            }
        }
    }

    void vprint(char const * formatString, va_list args) {
        // write msg to temp buffer
        static char temp[1024];
        vsprintf(temp, formatString, args);
        // copy temp buffer to main buffer, converting \n to cursor
        int cursorX = 0;
        size_t size = strlen(temp);
        for (size_t i = 0; i < size; ++i) {
            if (temp[i] == '\n') {
                dbgTextBuffer[cursorY*(charGridSize.w+1)+cursorX] = '\0';
                cursorY += 1;
                totalLines += 1;
                if (totalLines > charGridSize.h) {
                    --lineOffset;
                    if (lineOffset > charGridSize.h) lineOffset = charGridSize.h - 1;
                }
                cursorX = 0;
            }
            else if (cursorX < charGridSize.w && cursorY < charGridSize.h) {
                dbgTextBuffer[cursorY*(charGridSize.w+1)+cursorX] = temp[i];
                cursorX += 1;
            }

            if (cursorY == charGridSize.h) cursorY = 0;
        }
    }

    bool isShowingImGUI() { return state == DearImGUI; }

private:
    int state;
    size2 charGridSize;
    size_t cursorY;
    size_t totalLines;
    size_t lineOffset;


    size_t dbgTextBufferSize = 0;
    char * dbgTextBuffer = nullptr;

    // char * stderrBufferBegin = &dbgTextBuffer[0];
    // char * stderrBufferEnd = &dbgTextBuffer[stderrBufferSize];
};
