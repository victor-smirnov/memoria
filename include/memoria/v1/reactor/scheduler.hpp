
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

#include "../fiber/all.hpp"

#include <thread>
#include <memory>
#include <iostream>

namespace memoria {
namespace v1 {
namespace reactor {
    
    
class FiberProperties : public fibers::fiber_properties {
public:
    FiberProperties( fibers::context * ctx):
        fibers::fiber_properties( ctx)
    {}
};     

template <typename Reactor>
class Scheduler: public fibers::algo::algorithm_with_properties<FiberProperties> {
    std::shared_ptr<Reactor> reactor_;
    
    using ReadyQueue = fibers::scheduler::ready_queue_t;
    using WaitQueue = fibers::context::wait_queue_t;

    ReadyQueue ready_queue_ {};
    WaitQueue wait_queue_ {};
    
    uint64_t activations_{};

public:
    Scheduler(std::shared_ptr<Reactor> reactor): reactor_(reactor) {}

    uint64_t activations() const {return activations_;}
    
    virtual void awakened( fibers::context* ctx, FiberProperties& properties) noexcept 
    {
        BOOST_ASSERT( nullptr != ctx);

        BOOST_ASSERT( ! ctx->ready_is_linked() );
        ctx->ready_link( ready_queue_);
    }

    virtual fibers::context * pick_next() noexcept 
    {
        fibers::context* victim{ nullptr };
        
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
    
    void suspend(fibers::context* ctx)
    {
        BOOST_ASSERT( ! ctx->wait_is_linked() );
        ctx->wait_link( wait_queue_);
        // suspend this fiber
        ctx->suspend();
        BOOST_ASSERT( ! ctx->wait_is_linked() );
    }
    
    void resume(fibers::context* ctx)
    {
        BOOST_ASSERT( ctx->wait_is_linked() );
        
        ctx->wait_unlink();
        
        BOOST_ASSERT( !ctx->wait_is_linked() );
        
        ctx->get_scheduler()->set_ready(ctx);   
    }
};
    
}}}
