// Copyright (c) 2011-2015, Francois Saint-Jacques
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
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL FRANCOIS SAINT-JACQUES BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
// THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include "wait_strategy.h"
#include "sequence.h"

#include <memory>
#include <vector>
#include <iostream>

namespace memoria {
namespace v1 {
namespace disruptor {




template <typename W, template <typename> class Container = StdVector>
class SequenceBarrier
{
public:
    SequenceBarrier ( const Sequence& cursor, const Container<Sequence*>& dependents)
        : cursor_ ( cursor ), dependents_ ( dependents ), alerted_ ( false )
    {}

    int64_t WaitFor ( const int64_t& sequence )
    {
        return wait_strategy_.WaitFor ( sequence, cursor_, dependents_, alerted_ );
    }

    template <class R, class P>
    int64_t WaitFor ( const int64_t& sequence, const std::chrono::duration<R, P>& timeout )
    {
        return wait_strategy_.WaitFor ( sequence, cursor_, dependents_, alerted_, timeout );
    }

    int64_t get_sequence() const
    {
        return cursor_.sequence();
    }

    bool alerted() const
    {
        return alerted_.load ( std::memory_order::memory_order_acquire );
    }

    void set_alerted ( bool alert )
    {
        alerted_.store ( alert, std::memory_order::memory_order_release );
    }

private:
    W wait_strategy_;
    const Sequence& cursor_; // producer
    Container<Sequence*> dependents_;
    std::atomic<bool> alerted_;
};

}}}
