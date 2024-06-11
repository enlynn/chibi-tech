#pragma once 

#include "../Types.h"

#include <chrono>

namespace ct::os {
    struct Timer
    {
        using Clock = std::chrono::high_resolution_clock;
        using TimePoint = std::chrono::time_point<Clock>;
        // time duration in seconds
        using Duration = std::chrono::duration<f64, std::ratio<1>>;

        void start();
        void update();

        [[nodiscard]] f64 getSecondsElapsed()     const;
        [[nodiscard]] f64 getMilisecondsElapsed() const { return 1000.0 * getSecondsElapsed(); }

        TimePoint mStart;
        TimePoint mLastUpdate;
    };
}