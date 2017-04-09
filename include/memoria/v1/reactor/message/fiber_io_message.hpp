
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
#include "message.hpp"

#include <tuple>
#include <exception>
#include <string>

namespace memoria {
namespace v1 {
namespace reactor {
 
class FiberIOMessage: public Message {
protected:
    
    size_t count_;
    FiberContext* fiber_context_;
    
    
public:
    FiberIOMessage(int cpu, size_t count = 1, FiberContext* fiber_context = fibers::context::active()): 
        Message(cpu, false), 
        count_(count),
        fiber_context_(fiber_context)
    {
        return_ = true;
    }
    
    virtual ~FiberIOMessage() {}
    
    FiberContext* fiber_context() {return fiber_context_;}
    const FiberContext* fiber_context() const {return fiber_context_;}
    
    virtual void process() noexcept {}
    
    virtual void finish();
    
    virtual std::string describe();
   
    void wait_for();
    
    size_t count() const {return count_;}

	void set_count(size_t count) { count_ = count; }
};




    
}}}
