#include <time.h>

inline struct timespec genStartTime, genEndTime;

inline void code_timer_start() {
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &genStartTime);
}

// returns delta since last code_timer_start in microseconds
inline long code_timer_end() {
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &genEndTime);
    // microseconds
    long end =   (  genEndTime.tv_sec * 1e6 +   genEndTime.tv_nsec * 1e-3);
    long start = (genStartTime.tv_sec * 1e6 + genStartTime.tv_nsec * 1e-3);
    return end - start;
}

