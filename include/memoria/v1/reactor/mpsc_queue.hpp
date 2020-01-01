
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

//#include "../disruptor/sequencer.h"
//#include "../disruptor/ring_buffer.h"
//#include "../disruptor/sequence.h"
//#include "../disruptor/event_processor.h"

#include <disruptor-memoria/sequencer.h>
#include <disruptor-memoria/ring_buffer.h>
#include <disruptor-memoria/sequence.h>
#include <disruptor-memoria/event_processor.h>

#include <functional>
#include <thread>
#include <vector>
#include <memory>
#include <iostream>
#include <algorithm>

namespace memoria {
namespace v1 {
namespace reactor {
    
template <typename T, size_t BufferSize = 1024>
class MPSCQueue {
    
    using W = disruptor_memoria::DontWaitStrategy;
    using SequenceBarrierT = disruptor_memoria::SequenceBarrier<W, disruptor_memoria::StdVector>;
    
    disruptor_memoria::Sequencer<T, BufferSize, disruptor_memoria::MultiThreadedStrategy<BufferSize>, W, disruptor_memoria::StdVector> sequencer_;
    disruptor_memoria::BatchEventProcessor<T, SequenceBarrierT> event_processor_;
    
public:
    MPSCQueue():
        sequencer_{std::array<T, BufferSize>()},
        event_processor_(&sequencer_.ring_buffer(), sequencer_.NewBarrier())
    {
        sequencer_.set_gating_sequences(
            disruptor_memoria::StdVector<disruptor_memoria::Sequence*>({ &event_processor_.consumer_sequence() })
        );
    }
    
    bool send(const T& value)
    {
        auto idx = sequencer_.Claim(1);
        sequencer_[idx] = value;
        sequencer_.Publish(idx, 1);
        
        return true;
    }
    
    bool get(T& value)
    {
        auto size = event_processor_.has_events();
        if (size == 0) {
            return false;
        }
        
        value = event_processor_.get_current_event();
        event_processor_.commit(1);
        
        return true;
    }
    
    
    template <typename Fn>
    size_t get(size_t requested_size, Fn&& fn) 
    {
        size_t available_size = event_processor_.has_events();
        if (available_size == 0) {
            return 0;
        }
        
        size_t min_size = std::min(available_size, requested_size);
        
        for (size_t c = 0; c < min_size; c++)
        {
            fn(event_processor_.get_event(c));
        }
        
        event_processor_.commit(min_size);
        
        return min_size;
    }
    
    template <typename Fn>
    size_t get_all(Fn&& fn) 
    {
        size_t available_size = event_processor_.has_events();
        if (available_size == 0) {
            return 0;
        }
        
        for (size_t c = 0; c < available_size; c++) 
        {
            fn(event_processor_.get_event(c));
        }
        
        event_processor_.commit(available_size);
        
        return available_size;
    }
    
};    
    
}}}
