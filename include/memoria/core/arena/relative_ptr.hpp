
// Copyright 2022 Victor Smirnov
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

#include <memoria/core/types.hpp>

namespace memoria {
namespace arena {

template <typename T>
class RelativePtr {
    int64_t offset_;
public:
    RelativePtr() noexcept :
        offset_(0)
    {}

    RelativePtr(T* ptr) noexcept :
        offset_(to_u8(ptr) - my_addr())
    {}

    ~RelativePtr() noexcept = default;

private:
    RelativePtr(const RelativePtr&) = default;
    RelativePtr(RelativePtr&& ptr) noexcept = default;

public:

    void reset() noexcept {
        offset_ = 0;
    }

    bool is_null() const noexcept {
        return offset_ == 0;
    }

    bool is_not_null() const noexcept {
        return offset_ != 0;
    }

    T* get() noexcept {
        return ptr_cast<T>(my_addr() + offset_);
    }

    T* get() const noexcept {
        return ptr_cast<T>(my_addr() + offset_);
    }

    int64_t offset() const noexcept {
        return offset_;
    }

    T* operator=(T* ptr) noexcept
    {
        if (MMA_LIKELY((bool)ptr)) {
            offset_ = to_u8(ptr) - my_addr();
        }
        else {
            offset_ = 0;
        }
        return ptr;
    }

    T* operator->() noexcept {
        return get();
    }

    T* operator->() const noexcept {
        return get();
    }

//    T& operator*() noexcept {
//        return *get();
//    }

//    const T& operator*() const noexcept {
//        return *get();
//    }

private:
    RelativePtr& operator=(const RelativePtr&) = delete;
    RelativePtr& operator=(RelativePtr&&) = delete;


    uint8_t* to_u8(T* ptr) const noexcept {
        return reinterpret_cast<uint8_t*>(ptr);
    }


//    uint8_t* addr() noexcept {
//        return my_addr() + offset_;
//    }

    const uint8_t* addr() const noexcept {
        return my_addr() + offset_;
    }

//    uint8_t* my_addr() noexcept {
//        return reinterpret_cast<uint8_t*>(this);
//    }

    uint8_t* my_addr() const noexcept {
        return const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(this));
    }
};



}}
