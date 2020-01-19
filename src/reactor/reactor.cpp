
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

#include <memoria/reactor/reactor.hpp>
#include <memoria/reactor/application.hpp>
#include <memoria/core/tools/time.hpp>

#include <memory>
#include <chrono>

namespace memoria {
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

void Reactor::start()
{
    if (own_thread_)
    {
        worker_ = std::thread([this](){
            local_engine_ = this;
            event_loop(app().iopoll_timeout());
        });
    }
    else {
        local_engine_ = this;
    }
}

void Reactor::event_loop (uint64_t iopoll_timeout)
{
    scheduler_ = new Scheduler<Reactor>(shared_from_this());
    running_ = true;
    
    memoria::fibers::context::active()
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

    int io_poll_batch = 32;

    while(running_ || fibers::context::contexts() > fibers::DEFAULT_CONTEXTS) 
    {
        if (++io_poll_cnt_ >= io_poll_batch)
        {
            io_poll_cnt_ = 0;

            bool use_long_timeout{};

            if (this->idle_ticks() >= io_poll_batch)
            {
                auto duration = this->idle_duration();

                if (duration > 10)
                {
                    use_long_timeout = true;
                }
            }

            io_poller_.poll(use_long_timeout ? iopoll_timeout : 0);
            
            while (ring_buffer_.available())
            {
                this->reset_idle_ticks();
                auto msg = ring_buffer_.pop_back();
                if (msg) {
                    process_fn(msg);
                }
            }
        }

		if (event_loop_tasks_.size() > 0) 
		{
			for (auto& task : event_loop_tasks_) {
				task();
			}

			event_loop_tasks_.clear();
		}
        
        smp_->receive_all(cpu_, process_fn);
        
        auto acct0 = scheduler_->activations();

        withTime(yield_stat, []{
            memoria::this_fiber::yield();
        });

        auto acct1 = scheduler_->activations();

        if (acct1 - acct0 <= service_fibers_)
        {
            this->inc_idle_ticks();
        }
        else {
            this->reset_idle_ticks();
        }
    }

    stdout_stream().close();
    stderr_stream().close();
    
    if (app().is_debug()) 
    {
        SBuf buf;
        buf << "Event Loop finished for " << cpu_ << " yields: " << yield_stat << ", new fibers: " << (fiber_stat) << ", finishes: " << finish_stat <<  "\n";
    
        std::cout << buf.str();
    }
    
    thread_pool_.stop_workers();
}

int32_t current_cpu() {
    return engine().cpu();
}

int32_t number_of_cpus() {
    return engine().cpu_num();
}

void resume_ctx(fibers::context* ctx) {
    engine().scheduler()->resume(ctx);
}


}}
