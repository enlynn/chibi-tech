#include "stdafx.h"
#include "Timer.h"

namespace ct::os {
    void Timer::start()
    {
        mStart = Clock::now();
        mLastUpdate = mStart;
    }

    void Timer::update()
    {
        mStart = mLastUpdate;
        mLastUpdate = Clock::now();
    }

    f64 Timer::getSecondsElapsed() const
    {
        const Duration diff = mLastUpdate - mStart;
        return diff.count();
    }
}
