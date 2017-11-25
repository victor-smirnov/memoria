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

#include <memoria/v1/core/tools/bzero_struct.hpp>
#include <memoria/v1/reactor/message/fiber_io_message.hpp>
#include <memoria/v1/reactor/timer.hpp>
#include <memoria/v1/reactor/linux/linux_io_poller.hpp>

#include <memoria/v1/fiber/fiber.hpp>


#include "linux_io_messages.hpp"


#include <memory>

namespace memoria {
namespace v1 {
namespace reactor {    

class TimerImpl {
    int timer_fd_{-1};

    TimerMessage epoll_message_;

    bool stopped_{false};

    uint64_t count_;

public:
    TimerImpl(Timer::TimeUnit start_after, Timer::TimeUnit repeat_after, uint64_t count, TimerFn fn);
    ~TimerImpl() noexcept;

    void cancel();

    bool is_running() const {
        return !stopped_;
    }
};

}}}
