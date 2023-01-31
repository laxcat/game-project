#pragma once
#include "types.h"

class EventQueue {
public:
    static constexpr size_t MAX_EVENTS = 8;

    Event events[MAX_EVENTS];
    int count;

    Event * push(Event const && newEvent) {
        if (count == MAX_EVENTS) return nullptr;
        ++count;
        events[count-1] = newEvent;
        return &events[count-1];
    }

    Event * pop() {
        if (count <= 0) return nullptr;
        --count;
        return &events[count];
    }

    void clear() {
        #if DEBUG
        for (int i = 0; i < count; ++i) {
            events[i] = {};
        }
        #endif // DEBUG
        count = 0;
    }
private:
};
