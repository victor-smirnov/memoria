
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

#include "fiber_lambda_message.hpp"

#include <string>

namespace memoria {
namespace reactor {

namespace detail {

template <bool IsNoexcept>
struct NoexceptHelper {
    template <typename RtnFn, typename RtnValue>
    static void process(RtnFn& fn, RtnValue& result, std::exception_ptr& ex_ptr, FiberContext* fiber_context)
    {
        fn(result, fiber_context);
    }
};

template <>
struct NoexceptHelper<false> {
    template <typename RtnFn, typename RtnValue>
    static void process(RtnFn& fn, RtnValue& result, std::exception_ptr& ex_ptr, FiberContext* fiber_context)
    {
        fn(result, ex_ptr, fiber_context);
    }
};

template <bool IsNoexcept>
struct VoidNoexceptHelper {
    template <typename RtnFn>
    static void process(RtnFn& fn, std::exception_ptr& ex_ptr, FiberContext* fiber_context)
    {
        fn(fiber_context);
    }
};

template <>
struct VoidNoexceptHelper<false> {
    template <typename RtnFn>
    static void process(RtnFn& fn, std::exception_ptr& ex_ptr, FiberContext* fiber_context)
    {
        fn(ex_ptr, fiber_context);
    }
};


}


template <typename RtnFn, typename Reactor, typename RtnType, typename Fn, typename... Args>
class ThreadPoolMessage: public FiberLambdaMessage<Reactor, RtnType, Fn, Args...> {
    using Base = FiberLambdaMessage<Reactor, RtnType, Fn, Args...>;
    RtnFn rtn_fn_;

    using Base::fiber_context_;
    using Base::reactor_;
    using Base::result_;
    using Base::exception_;
    using Base::fn_;

public:
    template <typename... CtrArgs>
    ThreadPoolMessage(int cpu, Reactor* reactor, FiberContext* fiber_context, Fn&& fn, RtnFn&& rtn_fn, CtrArgs&&... args):
        Base(cpu, reactor, fiber_context, std::move(fn), std::forward<CtrArgs>(args)...),
        rtn_fn_(std::move(rtn_fn))
    {}

    virtual void finish() noexcept
    {
        BOOST_ASSERT_MSG(fiber_context_ != nullptr, "FiberContext is not set for a Message object");

        detail::NoexceptHelper<noexcept(fn_(std::declval<Args>()...))>::process(rtn_fn_, result_, exception_, fiber_context_);

        reactor_->thread_pool_.release(this);

        delete this;
    }
};


template <typename Reactor, typename Fn, typename RtnFn, typename... Args>
class ThreadPoolMessage<RtnFn, Reactor, void, Fn, Args...>: public FiberLambdaMessage<Reactor, void, Fn, Args...> {
    using Base = FiberLambdaMessage<Reactor, void, Fn, Args...>;
    RtnFn rtn_fn_;

    using Base::fiber_context_;
    using Base::reactor_;
    using Base::exception_;
    using Base::fn_;

public:
    template <typename... CtrArgs>
    ThreadPoolMessage(int cpu, Reactor* reactor, FiberContext* fiber_context, Fn&& fn, RtnFn&& rtn_fn, CtrArgs&&... args):
        Base(cpu, reactor, fiber_context, std::move(fn), std::forward<CtrArgs>(args)...),
        rtn_fn_(std::move(rtn_fn))
    {

    }

    virtual void finish() noexcept
    {
        BOOST_ASSERT_MSG(fiber_context_ != nullptr, "FiberContext is not set for a Message object");

        detail::VoidNoexceptHelper<noexcept(fn_(std::declval<Args>()...))>::process(rtn_fn_, exception_, fiber_context_);

        reactor_->thread_pool_.release(this);

        delete this;
    }
};



template <typename Reactor, typename Fn, typename RtnFn, typename... Args>
auto make_thread_pool_lambda_message(int cpu, Reactor* reactor, FiberContext* context, Fn&& fn, RtnFn&& rtn_fn, Args&&... args)
{
    using RtnType = std::invoke_result_t<std::decay_t< Fn >, std::decay_t< Args > ...>;
    
    using MsgType = ThreadPoolMessage<RtnFn, Reactor, RtnType, Fn, Args...>;
    
    return std::make_unique<MsgType>(cpu, reactor, context, std::forward<Fn>(fn), std::forward<RtnFn>(rtn_fn), std::forward<Args>(args)...);
}


    
}}
