
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

#include <memoria/v1/reactor/thread_pool.hpp>

#include <memory>

namespace memoria {
namespace v1 {
namespace reactor {
    
ThreadPool::ThreadPool(size_t min_size, size_t max_size, std::shared_ptr<Smp>& smp): 
    min_size_(min_size), 
    max_size_(max_size),
    smp_(smp)
{
    for (size_t c = 0; c < min_size; c++) 
    {
        free_workers_.push_back(*new WorkerT(smp_));
    }
}    

ThreadPool::~ThreadPool() 
{
    free_workers_.erase_and_dispose(
        free_workers_.begin(),
        free_workers_.end(),
        std::default_delete<WorkerT>()
    );
}



void ThreadPool::stop_workers()
{
    for (auto ii = free_workers_.begin(); ii != free_workers_.end(); ii++) {
        (*ii).stop();
    }
}

bool ThreadPool::wait_for_termination() 
{
    for (auto ii = free_workers_.begin(); ii != free_workers_.end(); ii++) 
    {
        if (!(*ii).is_finished()) 
        {
            return false;
        }
    }
    
    return true;
}


void ThreadPool::release(Message* task) 
{
    BOOST_ASSERT_MSG(task->data(), "ThreadPooolST: Provided message has no worker associated with");
    
    WorkerT* worker = tools::ptr_cast<WorkerT>(task->data());
    
    auto ii = busy_workers_.iterator_to(*worker);
    
    busy_workers_.erase(ii);

    if (free_workers_.size() < min_size_) 
    {
        free_workers_.push_back(*worker);
    }
    else {
        worker->stop();
        delete worker;
    }
}



ThreadPool::WorkerT* ThreadPool::acquire_worker() 
{
    WorkerT* worker {};
    if (free_workers_.size() > 0) 
    {
        worker = &free_workers_.back();
        free_workers_.pop_back();
    }
    else {
        worker = new WorkerT(smp_);
    }
        
    busy_workers_.push_back(*worker);
    
    return worker;
}
    
}}}
