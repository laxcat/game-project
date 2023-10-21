#pragma once
#include "types.h"

class InputQueue {
public:
    static constexpr size_t MAX_INPUTS = 8;

    Input inputs[MAX_INPUTS];
    int count = 0;

    Input * push(Input const && newInput) {
        if (count == MAX_INPUTS) return nullptr;
        ++count;
        inputs[count-1] = newInput;
        return &inputs[count-1];
    }

    Input * pop() {
        if (count <= 0) return nullptr;
        --count;
        return &inputs[count];
    }

    void clear() {
        #if DEBUG
        for (int i = 0; i < count; ++i) {
            inputs[i] = {};
        }
        #endif // DEBUG
        count = 0;
    }

private:
};
