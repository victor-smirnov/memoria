
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
#include "../message/message.hpp"

#include <memoria/v1/reactor/timer.hpp>

#include <tuple>
#include <exception>
#include <string>

#include <sys/event.h>

namespace memoria {
namespace v1 {
namespace reactor {
 


class KEventIOMessage: public Message {
public:
    KEventIOMessage(int cpu): Message(cpu, false)
    {
        return_ = true;
    }
    
    virtual ~KEventIOMessage() noexcept = default;

    virtual void on_receive(const kevent64_s& event) = 0;
};



class SocketIOMessage: public KEventIOMessage {
protected:
    fibers::context::iowait_queue_t iowait_queue_;

    bool connection_closed_{false};
    off_t available_{};

public:
    SocketIOMessage(int cpu): KEventIOMessage(cpu) {}
    virtual ~SocketIOMessage() noexcept = default;

    virtual void process() noexcept {}
    virtual void finish();

    virtual std::string describe() {return "SocketIOMessage";}

    void wait_for();

    void reset()
    {
        available_ = 0;
        connection_closed_ = false;
    }

    bool connection_closed() const {
        return connection_closed_;
    }

    virtual void on_receive(const kevent64_s& event) {
        available_ = event.data;
        connection_closed_ = event.flags & EV_EOF;
    }

    off_t available() const {
        return available_;
    }
};

class TimerMessage: public KEventIOMessage {
    uint64_t fd_;
    TimerFn timer_fn_;

    uint64_t fired_times_{};

public:
    TimerMessage(int cpu, uint64_t fd, TimerFn timer_fn):
        KEventIOMessage(cpu),
        fd_(fd),
        timer_fn_(timer_fn)
    {}

    virtual void process() noexcept {}

    virtual void finish();
    virtual void on_receive(const kevent64_s& event);

    virtual std::string describe() {return "TimerMessage";}

};

}}}
