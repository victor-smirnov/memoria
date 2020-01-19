
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

#include <boost/assert.hpp>

#include <vector>
#include <memory>
#include <type_traits>
#include <stdint.h>


namespace memoria {
namespace reactor {

template <typename T>
class RingBuffer {
    std::vector<T> buffer_;
    
    int64_t tail_{};
    int64_t head_{};
    
public:
    RingBuffer(size_t size): buffer_{size}
    {}
    
    size_t capacity() const {
        return buffer_.size() - available();
    }
    
    int capacity_i() const {
        return (int)(buffer_.size() - available());
    }
    
    size_t available() const {
        return head_ - tail_;
    }
    
    size_t ring_size() const {
        return buffer_.size();
    }
    
    const T& pop_back() 
    {
        BOOST_ASSERT(head_ > tail_);
        return buffer_[idx(tail_++)];
    }
    
    void push_front(const T& value)
    {
        BOOST_ASSERT(available() < ring_size());
        buffer_[idx(head_++)] = value;
    }

    template <typename Fn>
    void for_each(Fn&& fn)
    {
        for (auto c = tail_; c < head_; c++) {
            fn(buffer_[idx(c)]);
        }
    }
    
private:
    size_t idx(size_t i) const 
    {
        return i & (buffer_.size() - 1);
    }
};
    
    
}}
