#pragma once
#include <functional>
#include <list>


// Animator, the animation manager
class Animator {
public:
    // forward declare
    class Animation;

    // Function used for all animator functions
    using StandardFn = std::function<void(Animation const &)>;
    using FloatFn = std::function<float(Animation const &)>;
    using SimpleFn = std::function<void(void)>;

    // Main guts of animation info, used for setup and animation object itself
    // Needs to be "POD" (C++ concept) for the {.tick=...} style initialization to work.
    class AnimationConfig {
    public:
        float duration = 1;
        float delay = 0;
        int durationTicks = 0;
        StandardFn start = nullptr;
        FloatFn calcProgress = [](auto const & a){ return a.elapsedTime() / a.duration; };
        StandardFn tick = [](auto){};
        StandardFn complete = nullptr;
    };

    // Animation object class for each animation
    class Animation : public AnimationConfig {
        friend class Animator;
        Animation(AnimationConfig const & config) {
            start = config.start;
            calcProgress = config.calcProgress;
            tick = config.tick;
            complete = config.complete;
            duration = config.duration;
            durationTicks = config.durationTicks;
            delay = config.delay;
        }
        size_t id = 0;
        float progress = 0;
        double startTime = 0;
        Animator * animator = nullptr;
    public:
        size_t getId() const { return id; }
        float getProgress() const { return progress; }
        float elapsedTime() const { return animator->nowTime - startTime; }
    };

    // Manage the animator
    void tick(double nowtime);
    void cancelAll();
    void completeAllNow();

    // Create and manage animations
    size_t create(AnimationConfig const & anim);
    size_t doAfter(float delay, SimpleFn const & fn);
    bool cancel(size_t id);
    bool completeNow(size_t id);

private:
    // Animator vars
    size_t nextId = 0;
    std::list<Animation> all;
    double nowTime = 0;
    double prevTime = 0;
};
