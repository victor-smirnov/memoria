
// Copyright 2015 Victor Smirnov
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

#include <memoria/v1/api/common/ctr_api.hpp>

#include <memoria/v1/core/bignum/int64_codec.hpp>
#include <memoria/v1/core/types.hpp>

#include <memoria/v1/core/iovector/io_vector.hpp>

namespace memoria {
namespace v1 {

template <typename V> struct IsVLen: HasValue<bool, false> {};   

template <typename V, Granularity G> 
struct IsVLen<VLen<G, V>>: HasValue<bool, true> {};



template <typename SizeT>
class EndIteratorFn {
    SizeT size_;
public:
    EndIteratorFn(SizeT size): size_(size) {}
    
    SizeT size() const {return size_;}
};


template <typename Fn>
class IteratorFn {
    Fn& fn_;
    int64_t cnt_{};
public:
    IteratorFn(Fn& fn): fn_(fn) {}
    
    auto operator*() const {
        return fn_();
    }
    
    void operator++(int) {cnt_++;}
    
    bool operator!=(const EndIteratorFn<int64_t>& end) {
        return cnt_ < end.size();
    }
};






}}
