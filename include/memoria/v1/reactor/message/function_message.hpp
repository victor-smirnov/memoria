
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

#include <vector>
#include <memory>
#include <tuple>
#include <type_traits>
#include <exception>

namespace memoria {
namespace v1 {
namespace reactor {


class OneWayFunctionMessage: public Message {
    std::function<void()> fn_;
    
public:
    OneWayFunctionMessage(int cpu, const std::function<void()>& fn): 
        Message(cpu, true),
        fn_(fn)
    {}
    
    virtual void process() noexcept 
    {
        try {
            fn_();
        }
        catch(...) {
            exception_ = std::current_exception();
        }
    }
    
    virtual void finish() 
    {
        if (exception_)
        {
            auto tmp = exception_;
            delete this;
            std::rethrow_exception(tmp);
        }
        else {
            delete this;
        }
    }
    
    virtual std::string describe() {return "OneWayFunctionMessage";}
};



    
}}}
