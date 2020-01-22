
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

#ifdef _WIN32
#include "msvc/msvc_smp.hpp"
#elif __APPLE__
#include "macosx/macosx_smp.hpp"
#elif __linux__
#include "linux/linux_smp.hpp"
#else
#error "This platform is not supported"
#endif

#include "message/message.hpp"

#include "../core/memory/ptr_cast.hpp"

#include <thread>
#include <memory>
#include <atomic>
#include <mutex>
#include <vector>
#include <functional>
#include <iostream>
#include <exception>
#include <condition_variable>

#include <boost/intrusive/list.hpp>

#include <stdint.h>

namespace memoria {
namespace reactor {

    
namespace detail {
    
    class Worker: public boost::intrusive::list_base_hook<>  {
        std::thread thread_;
        
        std::mutex mutex_;
        std::condition_variable cv_;
        
        std::shared_ptr<Smp> smp_;

        Message* task_;
        bool ready_;
        bool stop_;
        bool finished_;
        
    public:
        Worker(std::shared_ptr<Smp>& smp)
        {
            std::unique_lock<std::mutex> lk(mutex_);

            smp_   = smp;
            task_  = nullptr;
            ready_ = false;
            stop_  = false;
            finished_ = false;

            thread_ = std::thread([this]
            {   
                std::unique_lock<std::mutex> lk(mutex_);
                
                while(true) 
                {
                    cv_.wait(lk, [this]{
                        return ready_;
                    });

                    ready_ = false;

                    if (!stop_)
                    {
                        BOOST_ASSERT(task_);
                        
                        task_->process();
                        smp_->submit_to(task_->cpu(), task_);
                        task_ = nullptr;
                    }
                    else {
                        break;
                    }
                }
                
                finished_ = true;
            });
        }
        
        ~Worker() {}
        
        void stop() 
        {
            if (thread_.joinable())
            {
                {
                    std::unique_lock<std::mutex> lk(mutex_);

                    ready_ = true;
                    stop_  = true;
                    task_  = nullptr;
                }
        
                cv_.notify_all();
            
                thread_.join();
            }
        }
        
        bool is_finished()
        {
            std::unique_lock<std::mutex> lk(mutex_);
            return finished_;
        }
        
        void run(Message* task)
        {
            BOOST_ASSERT_MSG(task != nullptr, "Task argument must not be null for ThreadPool::run()");
            //BOOST_ASSERT_MSG(!stop_, "ThreadPool has been already stopped");
            
            if (!stop_)
            {
                {
                    std::unique_lock<std::mutex> lk(mutex_);
                    task_  = task;
                    ready_ = true;
                }

                cv_.notify_all();
            }
            else {
                std::cerr << "Thread pool has been already stopped" << std::endl;
            }
        }
    };
}



class ThreadPool {
    
    using WorkerT       = detail::Worker;
    using WorkerList    = boost::intrusive::list<WorkerT>;
    
    size_t min_size_;
    size_t max_size_;
    
    WorkerList free_workers_;
    WorkerList busy_workers_;
    
    std::shared_ptr<Smp> smp_;

    bool pool_stopped_{false};
    
public:
    ThreadPool(size_t min_size, size_t max_size, std::shared_ptr<Smp>& smp);
    
    ~ThreadPool();

    bool pool_running() const {
        return !pool_stopped_;
    }
    
    bool try_run(Message* task)
    {
        if (busy_workers_.size() < max_size_)
        {
            WorkerT* worker = acquire_worker();
            
            task->set_data(worker);
            worker->run(task);
            
            return true;
        }
        
        return false;
    }
    
    void stop_workers();
    
    bool wait_for_termination();
    
    void release(Message* task);
    
    friend class detail::Worker;

private:
    
    WorkerT* acquire_worker();
};    
    
}}
