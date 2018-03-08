
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

#include <memoria/v1/core/config.hpp>
#include <memoria/v1/core/strings/format.hpp>
#include <memoria/v1/core/tools/ptr_cast.hpp>

#include <memoria/v1/reactor/message/thread_pool_message.hpp>

#include "scheduler.hpp"
#include "file.hpp"
#include "timer.hpp"

#include <memoria/v1/reactor/smart_ptr.hpp>

#include <memoria/v1/fiber/protected_stack_pool.hpp>
#include <memoria/v1/fiber/pooled_fixedsize_stack.hpp>

#ifdef MMA1_WINDOWS
#include <memoria/v1/reactor/msvc/msvc_io_poller.hpp>
#include <memoria/v1/reactor/msvc/msvc_smp.hpp>
#elif defined(MMA1_MACOSX)
#include <memoria/v1/reactor/macosx/macosx_smp.hpp>
#include <memoria/v1/reactor/macosx/macosx_io_poller.hpp>
#elif defined(MMA1_LINUX)
#include <memoria/v1/reactor/linux/linux_smp.hpp>
#include <memoria/v1/reactor/linux/linux_io_poller.hpp>
#endif

#include <memoria/v1/reactor/thread_pool.hpp>
#include <memoria/v1/reactor/ring_buffer.hpp>

#include <memoria/v1/reactor/console_streams.hpp>
#include <memoria/v1/reactor/streambuf.hpp>

#include <thread>
#include <memory>
#include <atomic>
#include <chrono>

namespace memoria {
namespace v1 {
namespace reactor {
    

class Reactor: public std::enable_shared_from_this<Reactor> {
    std::shared_ptr<Smp> smp_ {};
    int cpu_;
    bool own_thread_;
    
    std::thread worker_;
    
    bool running_{false};
    
    Scheduler<Reactor>* scheduler_{};
    
    ThreadPool thread_pool_;
    
    RingBuffer<Message*> ring_buffer_{1024};
    
    IOPoller io_poller_;
    
    fibers::protected_stack_pool fiber_stack_pool_{4};
    //fibers::protected_fixedsize_stack fiber_stack_pool_{};
    
    int32_t io_poll_cnt_{};
    int32_t yield_cnt_{};

    ConsoleOutputStream stdout_{1};
    ConsoleOutputStream stderr_{2};
    ConsoleInputStream stdin_{3};

    BinaryOStreamBuf<char> stdout_streambuf_{&stdout_};
    BinaryOStreamBuf<char> stderr_streambuf_{&stderr_};

    std::basic_ostream<char> cout_{&stdout_streambuf_};
    std::basic_ostream<char> cerr_{&stderr_streambuf_};

public:
	using EventLoopTask = std::function<void(void)>;
private:
	std::vector<EventLoopTask> event_loop_tasks_;

public:
    
    Reactor(std::shared_ptr<Smp> smp, int cpu, bool own_thread):
        smp_(smp), cpu_(cpu), own_thread_(own_thread), thread_pool_(1, 1000, smp_),
        io_poller_(cpu, ring_buffer_)
    {
        std::atomic_thread_fence(std::memory_order_seq_cst);
    }
    
    Reactor(const Reactor&) = delete;
    Reactor(Reactor&&) = delete;
    
    Reactor* operator=(const Reactor&) = delete;
    Reactor* operator=(Reactor&&) = delete;
    
    int cpu() const {return cpu_;}
    int cpu_num() const {return smp_->cpu_num();}
    bool own_thread() const {return own_thread_;}
    
    IOPoller& io_poller() {return io_poller_;}
    const IOPoller& io_poller() const {return io_poller_;}

    void sleep_for(const std::chrono::milliseconds& time) {
        io_poller_.sleep_for(time);
    }
    
    void join() 
    {
        if (own_thread())
        {
            return worker_.join();
        }
    }
    
    template <typename Fn, typename... Args>
    auto run_at(int target_cpu, Fn&& task, Args&&... args) 
    {
        if (target_cpu != cpu_) 
        {
            auto ctx = fibers::context::active();
            BOOST_ASSERT_MSG(ctx != nullptr, "Fiber context is null");
        
            auto msg = make_fiber_lambda_message(cpu_, this, ctx, std::forward<Fn>(task), std::forward<Args>(args)...);
            smp_->submit_to(target_cpu, msg.get());
            scheduler_->suspend(ctx);
        
            return msg->result();
        }
        else {
            return task(std::forward<Args>(args)...);
        }
    }
    
    template <typename Fn, typename... Args>
    void run_at_async(int target_cpu, Fn&& task, Args&&... args) 
    {
        if (target_cpu != cpu_) 
        {
            //auto ctx = fibers::context::active();
            BOOST_ASSERT_MSG(fibers::context::active() != nullptr, "Fiber context is null");
        
            auto msg = make_one_way_lambda_message(cpu_, std::forward<Fn>(task), std::forward<Args>(args)...);
            smp_->submit_to(target_cpu, msg);
        }
        else {
            task(std::forward<Args>(args)...);
        }
    }
    
    template <typename Fn, typename... Args>
    auto run_in_thread_pool(Fn&& task, Args&&... args)
    {
        auto ctx = fibers::context::active();
        BOOST_ASSERT_MSG(ctx != nullptr, "Fiber context is null");
        
        auto msg = make_fiber_lambda_message(cpu_, this, ctx, std::forward<Fn>(task), std::forward<Args>(args)...);
        
        while(!thread_pool_.try_run(msg.get())) 
        {
            memoria::v1::this_fiber::yield();
        }
        
        scheduler_->suspend(ctx);
        
        thread_pool_.release(msg.get());
        
        return msg->result();
    }

    template <typename Fn, typename RtnFn, typename... Args>
    void run_in_thread_pool_special(Fn&& task, RtnFn&& result_handler, Args&&... args)
    {
        auto ctx = fibers::context::active();
        BOOST_ASSERT_MSG(ctx != nullptr, "Fiber context is null");

        auto msg = make_thread_pool_lambda_message(
                    cpu_,
                    this,
                    ctx,
                    std::forward<Fn>(task),
                    std::forward<RtnFn>(result_handler),
                    std::forward<Args>(args)...
        );

        while(!thread_pool_.try_run(msg.get()))
        {
            memoria::v1::this_fiber::yield();
        }

        msg.release();
    }

	void send_message(Message* message) {
		smp_->submit_to(message->cpu(), message);
	}
    
    friend class Application;
    friend Reactor& engine();
    friend bool has_engine();
    
    template <typename> friend class FiberMessage;
    template <typename, typename, typename, typename, typename...> friend class ThreadPoolMessage;
    friend class FiberIOMessage;
    
    void stop() {
        running_ = false;
    }
    
    Scheduler<Reactor>* scheduler() {return scheduler_;}
    const Scheduler<Reactor>* scheduler() const {return scheduler_;}
    
    void drain_pending_io_events(const Message* message)
    {
        using PMessage = Message*;
        ring_buffer_.for_each([=](PMessage& pending_event){
            if (pending_event == message){
                pending_event = nullptr;
            }
        });
    }

	void run_in_event_loop(EventLoopTask task) {
		event_loop_tasks_.push_back(task);
	}

    std::shared_ptr<Reactor> self() {return shared_from_this();}

    ConsoleOutputStream& stdout_stream() {return stdout_;}
    ConsoleOutputStream& stderr_stream() {return stderr_;}
    ConsoleInputStream& stdin_stream() {return stdin_;}

    std::basic_ostream<char>& cout() {return cout_;}
    std::basic_ostream<char>& cerr() {return cerr_;}

    template <typename... Args>
    std::basic_ostream<char>& cout(const char16_t* fmt, Args&&... args)
    {
        cout_ << fmt::format(fmt, std::forward<Args>(args)...);
        return cout_;
    }

    template <typename... Args>
    std::basic_ostream<char>& coutln(const char16_t* fmt, Args&&... args)
    {
        cout_ << fmt::format(fmt, std::forward<Args>(args)...) << std::endl;
        return cout_;
    }

    template <typename... Args>
    std::basic_ostream<char>& cerr(const char16_t* fmt, Args&&... args)
    {
        cerr_ << fmt::format(fmt, std::forward<Args>(args)...);
        return cerr_;
    }

    template <typename... Args>
    std::basic_ostream<char>& cerrln(const char16_t* fmt, Args&&... args)
    {
        cerr_ << fmt::format(fmt, std::forward<Args>(args)...) << std::endl;
        return cerr_;
    }

    void println() {
        cout_ << std::endl;
    }



private:

    static thread_local Reactor* local_engine_;
    
    void start()
    {
        if (own_thread_) 
        {
            worker_ = std::thread([this](){
                local_engine_ = this;
                event_loop();
            });
        }
        else {
            local_engine_ = this;
        }
    }
    
    void event_loop();
    
    
    void shutdown() 
    {
        running_ = false;
    }
};

bool has_engine();

Reactor& engine();
    
template <typename Fn, typename... Args> 
auto engine_or_local(Fn&& fn, Args&&... args)
{
    if (has_engine()) {
        return engine().run_in_thread_pool(std::forward<Fn>(fn), std::forward<Args>(args)...);
    }
    else {
        return fn(std::forward<Args>(args)...);
    }
}


namespace detail {

template <typename Fn>
void run_at_engine(int32_t cpu, Fn&& fn) {
    engine().run_at(cpu, std::forward<Fn>(fn));
}


template <typename T>
int32_t engine_current_cpu() {
    return engine().cpu();
}

template <typename T>
int32_t engine_cpu_num() {
    return engine().cpu_num();
}


template <typename Fn>
auto shared_count::run_at(int32_t cpu, Fn&& fn) {
    return engine().run_at(cpu, std::forward<Fn>(fn));
}

}



}}}
