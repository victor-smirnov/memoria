
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


#include <memoria/v1/reactor/message/message.hpp>
#include <memoria/v1/reactor/timer.hpp>

#include <tuple>
#include <exception>
#include <string>

#include <linux/aio_abi.h>
#include <sys/epoll.h>



namespace memoria {
namespace v1 {
namespace reactor {
 

class EPollIOMessage: public Message {
protected:
    uint32_t flags_;

public:
    EPollIOMessage(int cpu):
        Message(cpu, false)
    {
        return_ = true;
    }

    virtual ~EPollIOMessage() noexcept {}

    virtual void on_receive(const epoll_event& event)
    {
        this->flags_ = event.events;
    }

    uint32_t flags() const {return flags_;}

    void reset(){
        flags_ = 0;
    }
};




class SocketIOMessage: public EPollIOMessage {
protected:

    fibers::context::iowait_queue_t iowait_queue_;

    uint32_t flags_;

    bool connection_closed_{false};

    const char* id_;
    
public:
    SocketIOMessage(int cpu, const char* id = "::default"):
        EPollIOMessage(cpu), id_(id)
    {}
    
    virtual ~SocketIOMessage() noexcept {}

    void reset()
    {
        EPollIOMessage::reset();
        connection_closed_ = false;
    }

    virtual void wait_for();

    bool connection_closed() const {
        return connection_closed_;
    }

    virtual void process() noexcept {}

    virtual void finish();

    virtual std::string describe() {return std::string("SocketIOMessage") + id_;}
};




class FileIOMessage: public Message {
public:
    FileIOMessage(int cpu): Message(cpu, false) {}
    virtual void report(io_event* status) = 0;
};


class TimerImpl;

class TimerMessage: public EPollIOMessage {
    int fd_;
    TimerImpl* timer_;

    uint64_t fired_times_{};

public:
    TimerMessage(int cpu, int fd, TimerImpl* timer):
        EPollIOMessage(cpu),
        fd_(fd),
        timer_(timer)
    {}

    virtual void process() noexcept {}

    virtual void finish();
    virtual void on_receive(const epoll_event& event);

    virtual std::string describe() {return "TimerMessage";}

};
    
}}}
