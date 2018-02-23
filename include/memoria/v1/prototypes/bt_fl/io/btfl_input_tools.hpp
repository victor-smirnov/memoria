
// Copyright 2016 Victor Smirnov
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

#include <memoria/v1/core/types.hpp>

#include <memoria/v1/core/packed/tools/packed_dispatcher.hpp>
#include <memoria/v1/core/packed/sseq/packed_rle_searchable_seq.hpp>

#include <memoria/v1/prototypes/bt/layouts/bt_input.hpp>
#include <memoria/v1/prototypes/bt_fl/btfl_tools.hpp>

#include <memory>

namespace memoria {
namespace v1 {
namespace btfl {
namespace io {


template <typename T>
class InputBufferHandler {
    T* ref_;
public:
    using MyType = InputBufferHandler<T>;
    using PtrType = T;

    InputBufferHandler(T* ref): ref_(ref) {}
    InputBufferHandler(): ref_(nullptr) {}
    InputBufferHandler(const MyType& other) = delete;
    InputBufferHandler(MyType&& other): ref_(other.ref_) {
        other.ref_ = nullptr;
    }

    ~InputBufferHandler() {
        if (ref_) free_system(ref_);
    }

    T* get() {
        return ref_;
    }

    const T* get() const {
        return ref_;
    }

    MyType& operator=(const MyType& other) = delete;

    void operator=(MyType&& other) {
        if (ref_) free_system(ref_);
        ref_ = other.ref_;
        other.ref_ = nullptr;
    }

    T* operator->() {return ref_;}
    const T* operator->() const {return ref_;}

    bool is_null() const {
        return ref_ == nullptr;
    }
};


class RunDescr {
    int32_t symbol_;
    int32_t length_;
public:
    RunDescr(): symbol_(), length_() {}

    RunDescr(int32_t symbol, int32_t length = 1): symbol_(symbol), length_(length) {}

    int32_t symbol() const {return symbol_;}
    int32_t length() const {return length_;}

    void set_length(int32_t len) {
        length_ = len;
    }
};


}}}}
