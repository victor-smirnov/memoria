
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

#include "macosx_smp.hpp"
#include "../message/message.hpp"
#include "../ring_buffer.hpp"

#include <memory>
#include <thread>

#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>


namespace memoria {
namespace v1 {
namespace reactor {


void KEvent64(int poller_fd, int fd, int filter, int flags, void * fiber_message_ptr = nullptr, bool throw_ex = true);

using IOBuffer = RingBuffer<Message*>;

class IOPoller {
    
    static constexpr int BATCH_SIZE = 1024;
    
    int queue_fd_{};
        
    IOBuffer& buffer_;

    uint64_t timer_fds_{1};
    
    int cpu_;

public:
    IOPoller(int cpu, IOBuffer& buffer);
    
    ~IOPoller();
    
    void poll(uint64_t timeout_ms);
    
    int queue_fd() const {return queue_fd_;}

    void sleep_for(const std::chrono::milliseconds& time);

    uint64_t new_timer_fd() {
        return timer_fds_++;
    }
};
    
}}}
