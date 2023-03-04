
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

#include "message.hpp"

#include <tuple>
#include <exception>
#include <string>

namespace memoria {
namespace reactor {

template <typename Reactor>
class FiberMessage: public Message {
protected:
    
    Reactor* reactor_;
    FiberContext* fiber_context_;
    
    
public:
    FiberMessage(int cpu, Reactor* reactor, FiberContext* fiber_context): 
        Message(cpu, false), 
        reactor_(reactor),
        fiber_context_(fiber_context)
    {
        run_in_fiber_ = true;
    }
    
    virtual ~FiberMessage() noexcept {}
    
    FiberContext* fiber_context() {return fiber_context_;}
    
    virtual void process() noexcept = 0;
    virtual void finish() 
    {
        BOOST_ASSERT_MSG(fiber_context_ != nullptr, "FiberContext is not set for a Message object");
        reactor_->scheduler()->resume(fiber_context_);
    }
    
    Reactor* reactor() {return reactor_;}
    const Reactor* reactor() const {return reactor_;}
};



    
}}
