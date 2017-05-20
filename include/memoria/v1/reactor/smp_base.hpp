
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

#include "../fiber/context.hpp"

#include <memoria/v1/core/tools/perror.hpp>

#include "mpsc_queue.hpp"
#include "message.hpp"

#include <vector>
#include <memory>
#include <tuple>
#include <type_traits>


namespace memoria {
namespace v1 {
namespace reactor {

using WorkerMessageQueue = MPSCQueue<Message*, 1024>;
using WorkerMessageQueuePtr = std::unique_ptr<WorkerMessageQueue>;

template <typename MyType>
class SmpBase: public std::enable_shared_from_this<MyType> {
    
    using Base = std::enable_shared_from_this<MyType>;
    
    using Base::shared_from_this;
    
    int cpu_num_;
    
    std::vector<WorkerMessageQueuePtr> inboxes_;
    
public:
    SmpBase(int cpu_num): 
        cpu_num_(cpu_num) 
    {
        if (cpu_num > 0) 
        {
            for (int c = 0; c < cpu_num; c++)
            {
                inboxes_.push_back(std::make_unique<WorkerMessageQueue>());
            }
        }
        else {
            tools::rise_error(SBuf() << "Number of threads (--threads) must be greather than zero: " << cpu_num);
        }
    }
    
    bool submit_to(int cpu, Message* msg) 
    {
        BOOST_ASSERT_MSG(cpu >= 0 && cpu < cpu_num_, "Invalid cpu number");
        return inboxes_[cpu]->send(msg);
    }
    
    template <typename Fn>
    size_t receive(int cpu, size_t max_batch_size, Fn&& consumer) 
    {
        BOOST_ASSERT_MSG(cpu >= 0 && cpu < cpu_num_, "Invalid cpu number");
        return inboxes_[cpu]->get(max_batch_size, std::forward<Fn>(consumer));
    }
    
    template <typename Fn>
    size_t receive_all(int cpu, Fn&& consumer) 
    {
        BOOST_ASSERT_MSG(cpu >= 0 && cpu < cpu_num_, "Invalid cpu number");
        return inboxes_[cpu]->get_all(std::forward<Fn>(consumer));
    }
    
    int cpu_num() const {return cpu_num_;}

    friend class Application;
    friend class Reactor;
    
protected:
    
    std::shared_ptr<MyType> self() {
        return shared_from_this();
    }
};
    
}}}
