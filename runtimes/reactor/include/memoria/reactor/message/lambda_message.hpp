
// Copyright 2017-2025 Victor Smirnov
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

#include <memoria/reactor/message/message.hpp>

#include <boost/smart_ptr/make_local_shared_object.hpp>

namespace memoria {
namespace reactor {

template <typename RtnType, typename Fn, typename... Args>
class LambdaMessage: public Message {

protected:    
    using Base = Message;
    
    using Base::return_;
    using Base::exception_;
    
    Fn fn_;
    std::tuple<Args...> args_;
    
    RtnType result_;
    
public:
    template <typename... CtrArgs>
    LambdaMessage(int cpu, bool one_way, Fn&& fn, CtrArgs&&... args): 
        Base(cpu, one_way),
        fn_(std::move(fn)), 
        args_(std::make_tuple(std::forward<CtrArgs>(args)...))
    {}
    
    virtual void process() noexcept
    {
        try {
            result_ = boost::context::detail::apply(fn_, args_);
        }
        catch(...) {
            exception_ = std::current_exception();
        }
        
        return_ = true;
    }
    
    RtnType result() const 
    {
        if (exception_) {
            std::rethrow_exception(exception_);
        }
        
        return result_;    
    }
    
    virtual void finish() 
    {
        if (one_way_)
        {
            delete this;
        }
    }
    
    virtual std::string describe() {return std::string("LambdaMessage: ") + typeid(Fn).name() + ", return: " + (return_ ? "true" : "false");}
};


template <typename Fn, typename... Args>
class LambdaMessage<void, Fn, Args...>: public Message {
protected:    
    Fn fn_;
    std::tuple<Args...> args_;
    
public:
    template <typename... CtrArgs>
    LambdaMessage(int cpu, bool one_way, Fn&& fn, CtrArgs&&... args): 
        Message(cpu, one_way),
        fn_(std::move(fn)), 
        args_(std::make_tuple(std::forward<CtrArgs>(args)...))
    {
        run_in_fiber_ = true;
    }
    
    virtual void process() noexcept
    {
        try {
            boost::context::detail::apply(fn_, args_);
        }
        catch(...) {
            exception_ = std::current_exception();
        }
        return_ = true;
    }
    
    virtual void finish() 
    {
        if (one_way_) {
            delete this;
        }
    }
    
    void result() const 
    {
        if (exception_) 
        {
            std::rethrow_exception(exception_);
        }
    }
    
    virtual std::string describe() {return std::string("LambdaMessage: ") + typeid(Fn).name() + ", return: " + (return_ ? "true" : "false");}
};

template <typename RtnType, typename Fn, typename... Args>
class OneWayLambdaMessage: public LambdaMessage<RtnType, Fn, Args...> {
    
    using Base = LambdaMessage<RtnType, Fn, Args...>;
    
    using Base::exception_;
    
public:
    
    template <typename... CtrArgs>
    OneWayLambdaMessage(int cpu, Fn&& fn, CtrArgs&&... args): 
        Base(cpu, true, std::forward<Fn>(fn), std::forward<CtrArgs>(args)...)
    {}

    virtual void finish()
    {
    }

    virtual void finalize_memory_object() {
        delete this;
    }
};



template <typename Fn, typename... Args>
auto make_lambda_message(int cpu, Fn&& fn, Args&&... args) 
{
    using RtnType = std::invoke_result_t<std::decay_t< Fn >, std::decay_t< Args > ...>;
    
    using MsgType = LambdaMessage<RtnType, Fn, Args...>;
    
    return std::make_unique<MsgType>(cpu, std::forward<Fn>(fn), std::forward<Args>(args)...);
}


template <typename Fn, typename... Args>
auto make_one_way_lambda_message(int cpu, Fn&& fn, Args&&... args) 
{
    using RtnType = std::invoke_result_t<std::decay_t< Fn >, std::decay_t< Args > ...>;    
    using MsgType = OneWayLambdaMessage<RtnType, Fn, Args...>;
    
    using PoolT = MessagePool<MsgType>;
    static thread_local boost::local_shared_ptr<PoolT> pool = boost::make_local_shared<PoolT>();

    return pool->allocate(cpu, std::forward<Fn>(fn), std::forward<Args>(args)...);
}

    
}}
