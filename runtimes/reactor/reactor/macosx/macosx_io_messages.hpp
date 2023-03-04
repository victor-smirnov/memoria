
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

#include <memoria/fiber/context.hpp>
#include <memoria/reactor/message/message.hpp>

#include <memoria/reactor/timer.hpp>

#include <tuple>
#include <exception>
#include <string>

#include <sys/event.h>

namespace memoria {
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

    fibers::context* context_{};

    bool connection_closed_{false};
    off_t available_{};
    bool error_{};
    int32_t error_code_{};

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
        error_ = false;
        error_code_ = 0;
    }

    bool connection_closed() const {
        return connection_closed_;
    }

    virtual void on_receive(const kevent64_s& event)
    {
        available_ = event.data;
        connection_closed_ = event.flags & EV_EOF;
        error_ = event.flags & EV_ERROR;
        error_code_ = event.data;
    }

    off_t available() const {
        return available_;
    }
};

class TimerImpl;

class TimerMessage: public KEventIOMessage {

    TimerImpl* timer_;

    uint64_t fired_times_;

public:
    TimerMessage(int cpu, TimerImpl* timer):
        KEventIOMessage(cpu),
        timer_(timer)
    {}

    virtual void process() noexcept {}

    virtual void finish();
    virtual void on_receive(const kevent64_s& event);

    virtual std::string describe() {return "TimerMessage";}

};

}}
