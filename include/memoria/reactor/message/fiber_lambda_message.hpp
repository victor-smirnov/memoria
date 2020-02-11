
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

#include "fiber_message.hpp"

#include <memoria/core/tools/optional.hpp>

#include <string>

namespace memoria {
namespace reactor {

template <typename Reactor, typename RtnType, typename Fn, typename... Args>
class FiberLambdaMessage: public FiberMessage<Reactor> {
    
    using Base = FiberMessage<Reactor>;
protected:
    using Base::return_;
    using Base::exception_;
    
    Fn fn_;
    std::tuple<Args...> args_;
    Optional<RtnType> result_{};

public:
    template <typename... CtrArgs>
    FiberLambdaMessage(int cpu, Reactor* reactor, FiberContext* fiber_context, Fn&& fn, CtrArgs&&... args): 
        Base(cpu, reactor, fiber_context),
        fn_(std::move(fn)), 
        args_(std::make_tuple(std::forward<CtrArgs>(args)...))
    {}
    
    virtual void process() noexcept
    {
        try {
            result_ = memoria::context::detail::apply(fn_, args_);
        }
        catch(...) {
            exception_ = std::current_exception();
        }
        
        return_ = true;
    }
    
    RtnType&& result()
    {
        if (exception_) {
            std::rethrow_exception(exception_);
        }
        
        return std::move(result_.get());
    }
    
    virtual std::string describe() {return std::string("FiberLambdaMessage: ") + typeid(Fn).name() + ", return: " + (return_ ? "true" : "false");}
};

template <typename Reactor, typename Fn, typename... Args>
class FiberLambdaMessage<Reactor, void, Fn, Args...>: public FiberMessage<Reactor> {
    using Base = FiberMessage<Reactor>;
protected:
    using Base::return_;
    using Base::exception_;
    
    Fn fn_;
    std::tuple<Args...> args_;
    
public:
    template <typename... CtrArgs>
    FiberLambdaMessage(int cpu, Reactor* reactor, FiberContext* fiber_context, Fn&& fn, CtrArgs&&... args): 
        Base(cpu, reactor, fiber_context),
        fn_(std::move(fn)), 
        args_(std::make_tuple(std::forward<CtrArgs>(args)...))
    {}
    
    virtual void process() noexcept
    {
        try {
            memoria::context::detail::apply(fn_, args_);
        }
        catch(...) {
            exception_ = std::current_exception();
        }
        return_ = true;
    }
    
    void result() const {
        if (exception_) {
            std::rethrow_exception(exception_);
        }
    }
    
    virtual std::string describe() {return std::string("FiberLambdaMessage: ") + typeid(Fn).name() + ", return: " + (return_ ? "true" : "false");}
};


template <typename Reactor, typename Fn, typename... Args>
auto make_fiber_lambda_message(int cpu, Reactor* reactor, FiberContext* context, Fn&& fn, Args&&... args) 
{
    using RtnType = std::result_of_t<std::decay_t< Fn >( std::decay_t< Args > ... )>;
    
    using MsgType = FiberLambdaMessage<Reactor, RtnType, Fn, Args...>;
    
    return std::make_unique<MsgType>(cpu, reactor, context, std::forward<Fn>(fn), std::forward<Args>(args)...);
}


    
}}
