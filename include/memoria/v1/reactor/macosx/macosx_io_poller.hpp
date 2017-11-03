
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


using IOBuffer = RingBuffer<Message*>;

// class KEventMessage: public Message {
//     bool eof_{};
//     size_t size_{};
//     size_t write_buffer_capacity_{};
//     
// public:
//     KEventMessage(int cpu): Message(cpu, false) {}
//     
//     bool is_eof() const {return eof_;}
//     size_t size() const {return size_;}
//     
//     void setup(bool eof, size_t size) {
//         eof_ = eof;
//         size_ = size;
//     }
// };


class IOPoller {
    
    static constexpr int BATCH_SIZE = 1024;
    
    int queue_fd_{};
        
    IOBuffer& buffer_;
    
public:
    IOPoller(IOBuffer& buffer);
    
    ~IOPoller();
    
    void poll();
    
    int queue_fd() const {return queue_fd_;}
};
    
}}}
