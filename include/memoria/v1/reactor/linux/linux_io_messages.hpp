
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

#include "../../fiber/context.hpp"
#include "../message/fiber_io_message.hpp"

#include <tuple>
#include <exception>
#include <string>

#include <linux/aio_abi.h>
#include <sys/epoll.h>

namespace memoria {
namespace v1 {
namespace reactor {
 


class EPollIOMessage: public FiberIOMessage {
protected:
    
    uint32_t flags_;

    bool connection_closed_{false};
    
public:
    EPollIOMessage(int cpu): FiberIOMessage(cpu)
    {}
    
    virtual ~EPollIOMessage() {}
    void configure(uint32_t flags) {
        this->flags_ = flags;
        //std::cout << "EPoll!!! " << (flags & EPOLLRDHUP) << std::endl;
    }
    
    uint32_t flags() const {return flags_;}

    void reset()
    {
        flags_ = 0;
        connection_closed_ = false;
    }

    virtual void wait_for() {
        reset();
        FiberIOMessage::wait_for();
        connection_closed_ = connection_closed_ || (flags_ & (EPOLLRDHUP | EPOLLHUP | EPOLLERR));
    }

    bool connection_closed() const {
        return connection_closed_;
    }
};

class FileIOMessage: public Message {
public:
    FileIOMessage(int cpu): Message(cpu, false) {}
    virtual void report(io_event* status) = 0;
};

    
}}}
