
// Copyright 2017-2023 Victor Smirnov
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

#include <memoria/disruptor/sequencer.h>
#include <memoria/disruptor/ring_buffer.h>
#include <memoria/disruptor/sequence.h>
#include <memoria/disruptor/event_processor.h>

#include <atomic_queue/atomic_queue.h>

#include <functional>
#include <thread>
#include <vector>
#include <memory>
#include <iostream>
#include <algorithm>

namespace memoria {
namespace reactor {
    
template <typename T, size_t BufferSize = 1024>
class MPMCQueue {
#ifdef NDEBUG
    using W = atomic_queue::AtomicQueue<T, BufferSize>;
#else
    using W = atomic_queue::AtomicQueue2<T, BufferSize>;
#endif

    W queue_;

public:
    MPMCQueue(){}
    
    bool send(const T& value) {
        queue_.push(value);
        return true;
    }

    bool try_send(const T& value) {
        return queue_.try_push(value);
    }
    
    bool get(T& value) {
        return queue_.try_pop(value);
    }
    
    
    template <typename Fn>
    size_t get(size_t requested_size, Fn&& fn) 
    {
        for (size_t c = 0; c < requested_size; c++) {
            T elem{};
            if (!queue_.try_pop(elem)) {
                return c;
            }
            else {
                fn(elem);
            }
        }
        return requested_size;
    }
    
    template <typename Fn>
    size_t get_all(Fn&& fn)
    {
        size_t available_size = queue_.was_size();
        if (available_size == 0) {
            return 0;
        }
        
        for (size_t c = 0; c < available_size; c++)
        {
            T elem{};
            if (!queue_.try_pop(elem)) {
                return c;
            }
            else {
                fn(elem);
                //std::cout << "Recieved" << std::endl;
            }
        }

        return available_size;
    }
    
};




    
}}
