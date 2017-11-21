
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

#include "linux_smp.hpp"
#include "../message/message.hpp"
#include "../ring_buffer.hpp"
#include "linux_io_messages.hpp"

#include <memory>
#include <thread>

#include <linux/aio_abi.h>


namespace memoria {
namespace v1 {
namespace reactor {

int io_setup(unsigned nr, aio_context_t *ctxp);
int io_destroy(aio_context_t ctx);
int io_submit(aio_context_t ctx, long nr,  struct iocb **iocbpp); 

using IOBuffer = RingBuffer<Message*>;


class IOPoller {
    
    static constexpr int BATCH_SIZE = 128;
    
    int epoll_fd_{};
    int event_fd_{};
    
    aio_context_t aio_context_{};
    
    
    
    IOBuffer& buffer_;
    
public:
    IOPoller(IOBuffer& buffer);
    
    ~IOPoller();
    
    void poll();
    
    int epoll_fd() const {return epoll_fd_;}
    int event_fd() const {return event_fd_;}
    
    aio_context_t aio_context() const {return aio_context_;}

private:
    void poll_file_events(int buffer_capacity, int other_events);
    
    ssize_t read_eventfd();
};
    
}}}
