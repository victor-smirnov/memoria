
// Copyright 2017 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#ifdef _WIN32
#include "msvc/msvc_timer.hpp"
#elif __APPLE__
#include "macosx/macosx_timer.hpp"
#elif __linux__
#include "linux/linux_timer.hpp"
#else
#error "Unsupported platform"
#endif

#include <memoria/v1/core/tools/pimpl_base.hpp>


namespace memoria {
namespace v1 {
namespace reactor {

using TimerFn = std::function<void(void)>;

class TimerImpl;

class Timer: public PimplBase<TimerImpl> {
    using Base = PimplBase<TimerImpl>;
public:
    using TimeUnit = std::chrono::milliseconds;

    MMA1_PIMPL_DECLARE_DEFAULT_FUNCTIONS(Timer)

    static Timer schedule(TimeUnit start_after, TimeUnit repeat_after, uint64_t count, TimerFn fn);

    static Timer schedule(TimeUnit start_after, TimeUnit repeat_after, TimerFn fn) {
        return Timer::schedule(start_after, repeat_after, 0, fn);
    }

    static Timer schedule(TimeUnit repeat_after, uint64_t count, TimerFn fn) {
        return Timer::schedule(repeat_after, repeat_after, count, fn);
    }

    static Timer schedule(TimeUnit repeat_after, TimerFn fn) {
        return Timer::schedule(repeat_after, repeat_after, 0, fn);
    }

    static Timer schedule_one(TimeUnit start_after, TimerFn fn) {
        return Timer::schedule(start_after, start_after, 1, fn);
    }

    bool is_running() const;
    void cancel();
};

}}}
