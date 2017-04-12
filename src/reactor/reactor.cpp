
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

#include <memory>

namespace memoria {
namespace v1 {
namespace reactor {

thread_local Reactor* Reactor::local_engine_ = nullptr;    
    
    
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

    auto process_fn = [this](Message* msg) {
        if (msg->is_return())
        {
            msg->finish();
        }
        else {
            fibers::fiber ff([this, msg](){
                msg->process();
                if (!msg->is_one_way()) 
                {
                    smp_->submit_to(msg->cpu(), msg);
                }
                else {
                    try {
                        msg->finish();
                    }
                    catch (...) {
                        std::terminate();
                    }
                }
            });
            
            ff.detach();
        }
    };
        
    while(running_ || fibers::context::contexts() > fibers::DEFAULT_CONTEXTS) 
    {
        io_poller_.poll();
        
        while (ring_buffer_.available())
        {
            process_fn(ring_buffer_.pop_back());
        }
        
        smp_->receive_all(cpu_, process_fn);
        
        memoria::v1::this_fiber::yield();
    }
    
    std::cout << "Event Loop finished for " << cpu_ << std::endl;
    
    thread_pool_.stop_workers();
}

    
}}}
