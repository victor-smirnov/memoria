
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
#else
#include "linux/smp.hpp"
#endif

#include "message/message.hpp"

#include "../core/tools/ptr_cast.hpp"

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
namespace v1 {
namespace reactor {

    
namespace detail {
    
    class Worker: public boost::intrusive::list_base_hook<>  {
        std::thread thread_;
        
        std::mutex mutex_;
        std::condition_variable cv_;
        
        Message* task_{};
        
        
        bool stop_{false};
        bool finished_{false};
        bool ready_{false};
        
        
        std::shared_ptr<Smp> smp_;

    public:
        Worker(std::shared_ptr<Smp>& smp): smp_(smp)
        {
            thread_ = std::thread([this]
            {   
                std::unique_lock<std::mutex> lk(mutex_);
                
                while(true) 
                {
                    ready_ = false;
                    cv_.wait(lk, [this]{
                        return ready_;
                    });
                    
                    if (!stop_)
                    {
                        BOOST_ASSERT(task_);
                        
                        task_->process();
                        smp_->submit_to(task_->cpu(), task_);
                    }
                    else {
                        break;
                    }
                }
                
                finished_ = true;
            });
        }
        
        void stop() 
        {
            if (thread_.joinable()) 
            {
                {
                    std::unique_lock<std::mutex> lk(mutex_);
                    stop_ = true;
                    ready_ = true;
                    task_ = nullptr;
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
            BOOST_ASSERT_MSG(!stop_, "ThreadPool has been already stopped");
            
            {
                std::unique_lock<std::mutex> lk(mutex_);
                task_  = task;
                ready_ = true;
            }

            cv_.notify_all();
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
    
public:
    ThreadPool(size_t min_size, size_t max_size, std::shared_ptr<Smp>& smp);
    
    ~ThreadPool();
    
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
    
}}}
