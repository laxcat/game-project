#pragma once
#include <functional>
#include <list>


// Animator, the animation manager
class Animator {
public:
    // forward declare
    class Animation;

    // Function used for all animator functions
    using AnimationFn = std::function<void(Animation const &)>;
    using SimpleFn = std::function<void(void)>;

    // Main guts of animation info, used for setup and animation object itself
    // Needs to be "POD" (C++ concept) for the {.tick=...} style initialization to work.
    class AnimationConfig {
    public:
        AnimationFn start = nullptr;
        AnimationFn tick = [](auto){};
        AnimationFn complete = nullptr;
        float duration = 1;
        float delay = 0;
    };

    // Animation object class for each animation
    class Animation : public AnimationConfig {
        friend class Animator;
        Animation(AnimationConfig const & config) {
            start = config.start;
            tick = config.tick;
            complete = config.complete;
            duration = config.duration;
            delay = config.delay;
        }
        size_t id = 0;
        float progress = 0;
        double startTime = 0;
    public:
        size_t getId() const { return id; }
        float getProgress() const { return progress; }
    };

    // Manage the animator
    static void tick(double nowtime);

    // Create and manage animations
    static size_t create(AnimationConfig const & anim);
    static size_t doAfter(float delay, SimpleFn const & fn);
    static bool cancel(size_t id);
    static bool completeNow(size_t id);

private:
    // Animator vars
    static size_t nextId;
    static std::list<Animation> all;
    static double nowTime;
    static double prevTime;
};
