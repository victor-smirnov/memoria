// Copyright (c) 2011, François Saint-Jacques
// Copyright (c) 2017, Victor Smirnov
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of the disruptor-- nor the
//       names of its contributors may be used to endorse or promote products
//       derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL FRANÇOIS SAINT-JACQUES BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include "ring_buffer.h"
#include "sequence.h"
#include "sequence_barrier.h"
#include "utils.h"

#include <stdexcept>
#include <functional>

namespace memoria {
namespace disruptor {


template <typename T, typename SequenceBarrier>
class BatchEventProcessor
{
    std::atomic<bool> running_;
    Sequence sequence_; // consumer idx

    RingBuffer<T>*    ring_buffer_;
    std::unique_ptr<SequenceBarrier>  sequence_barrier_;

    int64_t next_sequence_ {0};
    int64_t avalaible_sequence_ {kInitialCursorValue};

public:
    BatchEventProcessor ( RingBuffer<T>* ring_buffer, std::unique_ptr<SequenceBarrier> sequence_barrier ) :
        ring_buffer_ ( ring_buffer ),
        sequence_barrier_ ( std::move(sequence_barrier) )
    {}

    T& get_event ( int64_t idx )
    {
        return ring_buffer_->operator[] ( next_sequence_ + idx );
    }

    const T& get_event ( int64_t idx ) const
    {
        return ring_buffer_->operator[] ( next_sequence_ + idx );
    }

    const T& get_current_event () const {return get_event(next_sequence_);}
    T& get_current_event () {return get_event(next_sequence_);}


    int64_t has_events()
    {
        if (!is_batch())
        {
            return begin_batch();
        }

        return avalaible_sequence_ - next_sequence_ + 1; // +1 ?
    }


    int64_t begin_batch()
    {
        next_sequence_ = sequence_.sequence() + 1L;
        avalaible_sequence_ = sequence_barrier_->WaitFor ( next_sequence_ );

        if ( next_sequence_ <= avalaible_sequence_ )
        {
            return avalaible_sequence_ - next_sequence_ + 1; // +1 ?
        }
        else {
            return 0;
        }
    }

    void commit(int64_t len)
    {
        next_sequence_ += len;

        if (!is_batch()) {
            finish_batch();
        }
    }

    void finish_batch()
    {
        sequence_.set_sequence ( next_sequence_ - 1L );
    }

    int64_t next_sequence() const {return next_sequence_;}
    int64_t avalaible_sequence() const {return avalaible_sequence_;}

    bool is_batch() const {
        return next_sequence_ <= avalaible_sequence_;
    }

    SequenceBarrier* sequence_barrier() {return sequence_barrier_;}
    const SequenceBarrier* sequence_barrier() const {return sequence_barrier_;}

    Sequence& consumer_sequence() {return sequence_;}
    const Sequence& consumer_sequence() const {return sequence_;}

private:
    MMA_DISALLOW_COPY_MOVE_AND_ASSIGN ( BatchEventProcessor );
};


}}


