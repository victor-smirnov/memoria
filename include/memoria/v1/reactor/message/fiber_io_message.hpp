
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
    fibers::context::iowait_queue_t iowait_queue_;
#if __APPLE__
    off_t available_{};
    bool eof_{};
#endif
    
public:
    FiberIOMessage(int cpu): 
        Message(cpu, false)
    {
        return_ = true;
    }
    
    virtual ~FiberIOMessage() {}
        
    virtual void process() noexcept {}
    
    virtual void finish();
    
    virtual std::string describe();
   


#if __APPLE__
    void configure(off_t value, bool eof) {
        available_ = value;
        eof_ = eof;
    }
    
    off_t available() const {
        return available_;
    }
    
    bool is_eof() const {
        return eof_;
    }
#endif
    
    void wait_for();
};


    
}}}
