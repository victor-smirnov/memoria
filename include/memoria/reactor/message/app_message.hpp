
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

#include "lambda_message.hpp"

#include <vector>
#include <memory>
#include <tuple>
#include <type_traits>
#include <exception>

namespace memoria {
namespace reactor {


template <typename RtnType, typename Fn, typename... Args>
class AppLambdaMessage: public LambdaMessage<RtnType, Fn, Args...> {
    
    using Base = LambdaMessage<RtnType, Fn, Args...>;
    
public:
    
    template <typename... CtrArgs>
    AppLambdaMessage(int cpu, Fn&& fn, CtrArgs&&... args): 
        Base(cpu, true, std::forward<Fn>(fn), std::forward<CtrArgs>(args)...)
    {}
    
    virtual void finish() 
    {
        // Don't delete this OW message
    }
};

template <typename Fn, typename... Args>
auto make_special_one_way_lambda_message(int cpu, Fn&& fn, Args&&... args) 
{
    using RtnType = std::result_of_t<std::decay_t< Fn >( std::decay_t< Args > ... )>;
    
    using MsgType = AppLambdaMessage<RtnType, Fn, Args...>;
    
    return std::make_unique<MsgType>(cpu, std::forward<Fn>(fn), std::forward<Args>(args)...);
}
    
}}
