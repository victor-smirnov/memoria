
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

#include <boost/fiber/all.hpp>

#include <thread>
#include <memory>
#include <iostream>

namespace memoria {
namespace reactor {
    
    
class FiberProperties : public boost::fibers::fiber_properties {
public:
    FiberProperties( boost::fibers::context * ctx):
        boost::fibers::fiber_properties( ctx)
    {}
};     

template <typename Reactor>
class Scheduler: public boost::fibers::algo::algorithm_with_properties<FiberProperties> {
    std::shared_ptr<Reactor> reactor_;
    
    using ReadyQueue = boost::fibers::scheduler::ready_queue_type;
    ReadyQueue ready_queue_ {};
    
    uint64_t activations_{};
public:
    Scheduler(std::shared_ptr<Reactor> reactor): reactor_(reactor) {}

    uint64_t activations() const {return activations_;}
    
    virtual void awakened( boost::fibers::context* ctx, FiberProperties& properties) noexcept
    {
        BOOST_ASSERT( nullptr != ctx);

        BOOST_ASSERT( ! ctx->ready_is_linked() );
        ctx->ready_link( ready_queue_);
    }

    virtual boost::fibers::context * pick_next() noexcept
    {
        boost::fibers::context* victim{ nullptr };
        
        if ( ! ready_queue_.empty() )
        {
            victim = & ready_queue_.front();
            ready_queue_.pop_front();
            BOOST_ASSERT( nullptr != victim);
            BOOST_ASSERT( ! victim->ready_is_linked() );

            ++activations_;
        }
        
        return victim;
    }

    virtual bool has_ready_fibers() const noexcept 
    {
        return ! ready_queue_.empty();
    }

    virtual void suspend_until( std::chrono::steady_clock::time_point const&) noexcept 
    {
        // Go to event loop
    }

    virtual void notify() noexcept 
    {}
    
    void suspend(boost::fibers::context* ctx)
    {
        ctx->suspend();
    }
    
    void resume(boost::fibers::context* ctx)
    {        
        ctx->get_scheduler()->schedule(ctx);
    }
};
    
}}
