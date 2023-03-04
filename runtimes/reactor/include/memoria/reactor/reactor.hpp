
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

#include <memoria/core/config.hpp>
#include <memoria/core/strings/format.hpp>
#include <memoria/core/memory/ptr_cast.hpp>

#include <memoria/reactor/message/thread_pool_message.hpp>

#include <memoria/reactor/scheduler.hpp>
#include <memoria/reactor/file.hpp>
#include <memoria/reactor/timer.hpp>
#include <memoria/reactor/message_queue.hpp>

#include <memoria/reactor/protected_stack_pool.hpp>
#include <boost/fiber/pooled_fixedsize_stack.hpp>

#ifdef MMA_WINDOWS
#include <memoria/reactor/msvc/msvc_io_poller.hpp>
#include <memoria/reactor/msvc/msvc_smp.hpp>
#elif defined(MMA_MACOSX)
#include <memoria/reactor/macosx/macosx_smp.hpp>
#include <memoria/reactor/macosx/macosx_io_poller.hpp>
#elif defined(MMA_LINUX)
#include <memoria/reactor/linux/linux_smp.hpp>
#include <memoria/reactor/linux/linux_io_poller.hpp>
#endif

#include <memoria/reactor/thread_pool.hpp>
#include <memoria/reactor/ring_buffer.hpp>

#include <memoria/reactor/console_streams.hpp>
#include <memoria/reactor/streambuf.hpp>

#include <thread>
#include <memory>
#include <atomic>
#include <chrono>
#include <list>

namespace memoria {
namespace reactor {

class Reactor;

Reactor& engine();

class Reactor: public std::enable_shared_from_this<Reactor> {

    using TaskQueueT = MessageQueue;

    std::shared_ptr<Smp> smp_ {};
    int cpu_;
    bool own_thread_;
    
    std::thread worker_;
    
    bool running_{false};
    
    boost::intrusive_ptr<Scheduler<Reactor>> scheduler_{};
    
    ThreadPool thread_pool_;
    
    RingBuffer<Message*> ring_buffer_{1024};
    
    IOPoller io_poller_;
    memoria::fibers::protected_stack_pool fiber_stack_pool_{8, 16 * 1024 * 1024};
    //fibers::protected_fixedsize_stack fiber_stack_pool_{};

    uint64_t io_poll_cnt_{};
    uint64_t yield_cnt_{};

    ConsoleOutputStream stdout_{1};
    ConsoleOutputStream stderr_{2};
    ConsoleInputStream stdin_{3};

    BinaryOStreamBuf<char> stdout_streambuf_{&stdout_};
    BinaryOStreamBuf<char> stderr_streambuf_{&stderr_};

    std::basic_ostream<char> cout_{&stdout_streambuf_};
    std::basic_ostream<char> cerr_{&stderr_streambuf_};

    int exit_status_{};

    bool shutdown_requested_{false};

    uint64_t service_fibers_{2};

    std::list<TaskQueueT> tasks_queues_;

public:
    using Clock = std::chrono::system_clock;
    using TimePoint = std::chrono::time_point<Clock>;

private:
    TimePoint idle_start_{};
    uint64_t idle_ticks_;

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

    int exit_status() const {return exit_status_;}

    void start_service_fiber() {
        service_fibers_++;
    }

    void stop_service_fiber()
    {
        BOOST_ASSERT_MSG(service_fibers_ > 0, "No service tibers running");
        service_fibers_--;
    }

    uint64_t idle_ticks() const {
        return idle_ticks_;
    }

    void inc_idle_ticks() {
        if (++idle_ticks_ == 1) {
            idle_start_ = std::chrono::system_clock::now();
        }
    }

    void reset_idle_ticks() {
        idle_ticks_ = 0;
    }


    uint64_t idle_duration() const
    {
        auto now = Clock::now();
        using Ms = std::chrono::milliseconds;
        auto duration = std::chrono::duration_cast<Ms>(now - idle_start_).count();
        return duration;
    }
    
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

    void suspend_fiber(boost::fibers::context* ctx)
    {
        scheduler_->suspend(ctx);
    }

    void resume_fiber(boost::fibers::context* ctx)
    {
        scheduler_->resume(ctx);
    }
    
    template <typename Fn, typename... Args>
    auto run_at(int target_cpu, Fn&& task, Args&&... args)
    {
        if (target_cpu != cpu_) 
        {
            auto ctx = boost::fibers::context::active();
            BOOST_ASSERT_MSG(ctx != nullptr, "Fiber context is null");
        
            auto msg = make_fiber_lambda_message(cpu_, this, ctx, std::forward<Fn>(task), std::forward<Args>(args)...);
            smp_->submit_to(target_cpu, msg.get());
            scheduler_->suspend(ctx);
        
            return std::move(msg->result());
        }
        else {
            return task(std::forward<Args>(args)...);
        }
    }

    template <typename Fn, typename... Args>
    auto run(MessageQueue& queue, Fn&& task, Args&&... args)
    {
        auto ctx = boost::fibers::context::active();
        BOOST_ASSERT_MSG(ctx != nullptr, "Fiber context is null");

        auto msg = make_fiber_lambda_message(cpu_, this, ctx, std::forward<Fn>(task), std::forward<Args>(args)...);

        queue.get()->send(msg.get());
        scheduler_->suspend(ctx);

        return std::move(msg->result());
    }

    template <typename Fn, typename... Args>
    void run_at_v(int target_cpu, Fn&& task, Args&&... args)
    {
        if (target_cpu != cpu_)
        {
            auto ctx = boost::fibers::context::active();
            BOOST_ASSERT_MSG(ctx != nullptr, "Fiber context is null");

            auto msg = make_fiber_lambda_message(cpu_, this, ctx, std::forward<Fn>(task), std::forward<Args>(args)...);
            smp_->submit_to(target_cpu, msg.get());
            scheduler_->suspend(ctx);
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
            //auto ctx = boost::fibers::context::active();
            BOOST_ASSERT_MSG(boost::fibers::context::active() != nullptr, "Fiber context is null");
        
            auto msg = make_one_way_lambda_message(cpu_, std::forward<Fn>(task), std::forward<Args>(args)...);
            smp_->submit_to(target_cpu, msg);
        }
        else {
            task(std::forward<Args>(args)...);
        }
    }

    template <typename Fn, typename... Args>
    void run_async(MessageQueue& queue, Fn&& task, Args&&... args)
    {
        //auto ctx = fibers::context::active();
        BOOST_ASSERT_MSG(boost::fibers::context::active() != nullptr, "Fiber context is null");

        auto msg = make_one_way_lambda_message(cpu_, std::forward<Fn>(task), std::forward<Args>(args)...);
        if (!queue.get()->try_send(msg)) {
            boost::this_fiber::yield();
        }
    }


    
    template <typename Fn, typename... Args>
    auto run_in_thread_pool(Fn&& task, Args&&... args)
    {
        auto ctx = boost::fibers::context::active();
        BOOST_ASSERT_MSG(ctx != nullptr, "Fiber context is null");
        BOOST_ASSERT_MSG(thread_pool_.pool_running(), "Thread pool has been alredy stopped");
        
        auto msg = make_fiber_lambda_message(cpu_, this, ctx, std::forward<Fn>(task), std::forward<Args>(args)...);
        
        while(!thread_pool_.try_run(msg.get())) 
        {
            boost::this_fiber::yield();
        }
        
        scheduler_->suspend(ctx);
        
        thread_pool_.release(msg.get());
        
        return msg->result();
    }

    template <typename Fn, typename RtnFn, typename... Args>
    void run_in_thread_pool_special(Fn&& task, RtnFn&& result_handler, Args&&... args)
    {
        auto ctx = boost::fibers::context::active();
        BOOST_ASSERT_MSG(ctx != nullptr, "Fiber context is null");
        BOOST_ASSERT_MSG(thread_pool_.pool_running(), "Thread pool has been alredy stopped");

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
            boost::this_fiber::yield();
        }

        msg.release();
    }

	void send_message(Message* message) {
		smp_->submit_to(message->cpu(), message);
	}

    void send_message(int cpu, Message* message) {
        smp_->submit_to(cpu, message);
    }

    void send_message(MessageQueue& queue, Message* message) {
        queue.get()->send(message);
    }


    friend class Application;
    friend Reactor& engine();
    friend bool has_engine();
    
    void add_queue_to(const MessageQueue& queue, int cpu) {
        run_at_v(cpu, [=]() -> void {
            engine().register_queue(queue);
        });
    }

    template <typename> friend class FiberMessage;
    template <typename, typename, typename, typename, typename...> friend class ThreadPoolMessage;
    friend class FiberIOMessage;
    
    void stop() {
        running_ = false;
    }
    
    Scheduler<Reactor>* scheduler() {return scheduler_.get();}
    const Scheduler<Reactor>* scheduler() const {return scheduler_.get();}
    
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
    std::basic_ostream<char>& cout(const char* fmt, Args&&... args)
    {
        cout_ << fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...);
        return cout_;
    }

    template <typename... Args>
    std::basic_ostream<char>& println(const char* fmt, Args&&... args)
    {
        memoria::println(cout_, fmt, std::forward<Args>(args)...);
        return cout_;
    }

    template <typename... Args>
    std::basic_ostream<char>& coutln(const char* fmt, Args&&... args)
    {
        cout_ << fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...) << std::flush << std::endl;
        return cout_;
    }

    template <typename... Args>
    std::basic_ostream<char>& cerr(const char* fmt, Args&&... args)
    {
        cerr_ << fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...);
        return cerr_;
    }

    template <typename... Args>
    std::basic_ostream<char>& cerrln(const char* fmt, Args&&... args)
    {
        cerr_ << fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...) << std::flush << std::endl;
        return cerr_;
    }

    bool shutdown_requested() const {return !running_;}


    template <typename Fn, typename... Args>
    boost::fibers::fiber in_fiber(Fn&& fn, Args&&... args)
    {
        boost::fibers::fiber ff(
            boost::fibers::launch::dispatch,
            std::allocator_arg_t(),
            this->fiber_stack_pool_,
            std::forward<Fn>(fn),
            std::forward<Args>(args)...
        );

        return ff;
    }

private:

    void register_queue(const MessageQueue& queue) {
        tasks_queues_.push_back(queue);
    }

    void unregister_queue(const MessageQueue& queue)
    {
        for (auto ii = tasks_queues_.begin(); ii != tasks_queues_.end(); ii++) {
            if (*ii == queue) {
                tasks_queues_.erase(ii);
                break;
            }
        }
    }

    static thread_local Reactor* local_engine_;
    
    void start();    
    void event_loop(uint64_t iopoll_timeout);

    void handle_memory_objects(MemoryObject* obj);
};

bool has_engine();

Reactor& engine();

template <typename Fn, typename... Args>
boost::fibers::fiber in_fiber(Fn&& fn, Args&&... args)
{
    return engine().in_fiber(
        std::forward<Fn>(fn),
        std::forward<Args>(args)...
    );
}


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

}



}}
