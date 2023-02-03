#pragma once
#include "types.h"
// #include <functional>

class EventQueue {
public:
    static constexpr size_t MAX_EVENTS = 8;

    Event events[MAX_EVENTS];
    int count = 0;

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

    // void foreach(std::function<void(Event &)> const & fn) {
    //     for (int i = 0; i < count; ++i) {
    //         if (events[i].type == Event::None) continue;
    //         fn(events[i]);
    //     }
    // }
private:
};
