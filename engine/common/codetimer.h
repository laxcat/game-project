#include <time.h>

namespace codetimer {
    inline struct timespec genStartTime, genNowTime;

    inline void start() {
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &genStartTime);
    }

    // returns delta since last start in microseconds
    inline long delta() {
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &genNowTime);
        // convert to microseconds
        long now =   (  genNowTime.tv_sec * 1e6 +   genNowTime.tv_nsec * 1e-3);
        long start = (genStartTime.tv_sec * 1e6 + genStartTime.tv_nsec * 1e-3);
        return now - start;
    }
}
