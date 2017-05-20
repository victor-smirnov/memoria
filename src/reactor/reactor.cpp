
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

#include <memoria/v1/reactor/reactor.hpp>
#include <memoria/v1/core/tools/time.hpp>

#include <memory>
#include <chrono>

namespace memoria {
namespace v1 {
namespace reactor {

thread_local Reactor* Reactor::local_engine_ = nullptr;    

bool has_engine() {
    return Reactor::local_engine_ != nullptr;
}


Reactor& engine() 
{
	if (Reactor::local_engine_ == nullptr) {
		std::cout << "No reactor" << std::endl;
	}

    BOOST_ASSERT_MSG(Reactor::local_engine_ != nullptr, "Reactor hasn't been initialized");
    
    return *Reactor::local_engine_;
}

void Reactor::event_loop ()
{
    scheduler_ = new Scheduler<Reactor>(shared_from_this());
    running_ = true;
    
    memoria::v1::fibers::context::active()
        ->get_scheduler()
        ->set_algo(std::unique_ptr< Scheduler<Reactor> >(scheduler_));

    
    
    CallDuration fiber_stat;
    CallDuration yield_stat;
    CallDuration finish_stat;
        
    auto process_fn = [&](Message* msg) {
        if (msg->is_return())
        {
            withTime(finish_stat, [&]{
                msg->finish();
            });
        }
        else {
            withTime(fiber_stat, [&]{
                fibers::fiber ff(fibers::launch::dispatch, std::allocator_arg_t(), this->fiber_stack_pool_, [&, msg](){
                    msg->process();
                    if (!msg->is_one_way())
                    {
                        smp_->submit_to(msg->cpu(), msg);
                    }
                    else {
                        try {
                            withTime(finish_stat, [msg]{
                                msg->finish();
                            });
                        }
                        catch (...) {
                            std::terminate();
                        }
                    }
                });
                
                ff.detach();
            });
        }
    };

    
    while(running_ || fibers::context::contexts() > fibers::DEFAULT_CONTEXTS) 
    {
        if (++io_poll_cnt_ == 32) 
        {
            io_poll_cnt_ = 0;
            io_poller_.poll();
            
            while (ring_buffer_.available())
            {
                process_fn(ring_buffer_.pop_back());
            }
        }
        
        smp_->receive_all(cpu_, process_fn);
        
        withTime(yield_stat, []{
            memoria::v1::this_fiber::yield();
        });
    }
    
    SBuf buf;
    buf << "Event Loop finished for " << cpu_ << " yields: " << yield_stat << ", new fibers: " << (fiber_stat) << ", finishes: " << finish_stat << "\n";
    
    std::cout << buf.str();
    
    thread_pool_.stop_workers();
}

    
}}}
