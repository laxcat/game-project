#include "Animator.h"
#include <float.h>


void Animator::tick(double nowTime_) {
    // if first time fake prev time
    prevTime = (nowTime) ? nowTime : nowTime_ - 1.0/60.0;
    nowTime = nowTime_;

    // loop through current animations
    auto i = all.begin();
    while (i != all.end()) {
        // percentage complete, could be <0 or >1
        i->progress = (nowTime - i->startTime) / i->duration;

        // not started yet
        if (i->progress < 0) { ++i; continue; }

        // progress is over 0 for the first time
        // (if last time it was below, and we already know progress >= 0)
        if (i->start && prevTime < i->startTime) i->start(*i);

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
    a.startTime = nowTime + a.delay;
    if (a.duration <= 0) a.duration = 0.000000000001;
    all.push_back(a);
    return a.id;
}

size_t Animator::doAfter(float delay, SimpleFn const & fn) {
    return Animator::create({
        .complete = [fn](auto) { fn(); },
        .delay = delay,
        .duration = 0,
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

size_t Animator::nextId = 0;
std::list<Animator::Animation> Animator::all;
double Animator::nowTime = 0;
double Animator::prevTime = 0;
