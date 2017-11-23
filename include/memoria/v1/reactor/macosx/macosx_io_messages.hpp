
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

#include <sys/event.h>

namespace memoria {
namespace v1 {
namespace reactor {
 


class KEventIOMessage: public FiberIOMessage {
protected:

    bool connection_closed_{false};
    off_t available_{};

public:
    KEventIOMessage(int cpu): FiberIOMessage(cpu)
    {}
    
    virtual ~KEventIOMessage() {}

    void reset()
    {
        available_ = 0;
        connection_closed_ = false;
    }

    virtual void wait_for() {
        reset();
        FiberIOMessage::wait_for();
    }

    bool connection_closed() const {
        return connection_closed_;
    }

    void configure(bool eof, off_t value) {
        available_ = value;
        connection_closed_ = eof;
    }

    off_t available() const {
        return available_;
    }
};

    
}}}
