#include "Animator.h"
#include <float.h>
#include "../dev/print.h"


void Animator::tick(double nowTime_) {
    // if first time fake prev time
    prevTime = (nowTime) ? nowTime : nowTime_ - 1.0/60.0;
    nowTime = nowTime_;

    // loop through current animations
    auto i = all.begin();
    while (i != all.end()) {
        // simple tick interface. ignores duration, delay and all seconds-based time
        if (i->durationTicks) {
            --i->durationTicks;
            i->progress = (i->durationTicks) ? 0 : 1;

            i->tick(*i);

            // complete
            if (!i->durationTicks) {
                if (i->complete) i->complete(*i);
                i = all.erase(i);
            }
            // not complete yet
            else {
                ++i;
            }

            continue;
        }

        // run update fn, (see default in AnimationConfig)
        // default fn updates progress could be <0 or >1
        i->progress = i->calcProgress(*i);
        // print("but why (%zu)?? %f\n", i->id, i->progress);

        // not started yet
        if (i->progress < 0) { ++i; continue; }

        // progress >= 0 (we know this from above), and start hasn't run yet
        if (i->start) {
            i->start(*i);
            i->start = nullptr;
        }

        // completed
        if (i->progress >= 1) {
            i->progress = 1;
            i->tick(*i);
            if (i->complete) i->complete(*i);
            i = all.erase(i);
            continue;
        }

        // in progress
        i->tick(*i);

        ++i;
    }
}

size_t Animator::create(Animator::AnimationConfig const & config) {
    Animation a = config;
    a.id = ++nextId;
    a.animator = this;
    a.startTime = nowTime + a.delay;
    if (a.duration <= 0) a.duration = 0.000000000001;
    all.push_back(a);
    return a.id;
}

size_t Animator::doAfter(float delay, SimpleFn const & fn) {
    return Animator::create({
        .duration = 0,
        .delay = delay,
        .complete = [fn](auto) { fn(); },
    });
}

bool Animator::cancel(size_t id) {
    auto i = all.begin();
    while (i != all.end()) {
        if (i->id == id) {
            all.erase(i);
            return true;
        }
        ++i;
    }
    return false;
}

bool Animator::completeNow(size_t id) {
    auto i = all.begin();
    while (i != all.end()) {
        if (i->id == id) {
            i->progress = 1;
            i->tick(*i);
            if (i->complete) i->complete(*i);
            all.erase(i);
            return true;
        }
        ++i;
    }
    return false;
}

void Animator::cancelAll() {
    while (all.size()) all.erase(all.begin());
}

void Animator::completeAllNow() {
    while (all.size()) {
        auto i = all.begin();
        i->progress = 1;
        i->tick(*i);
        if (i->complete) i->complete(*i);
        all.erase(i);
    }
}