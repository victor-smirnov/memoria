
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
#include <memoria/core/memory/ptr_cast.hpp>

#include <memoria/core/tools/result.hpp>
#include <memoria/core/datatypes/traits.hpp>

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

    RelativePtr(const RelativePtr& other) noexcept {
        offset_(to_u8(other.get()) - my_addr());
    }

    RelativePtr(RelativePtr&& other) noexcept {
        offset_(to_u8(other.get()) - my_addr());
    }

    ~RelativePtr() noexcept = default;

    RelativePtr& operator=(const RelativePtr& other) {
        offset_ = to_u8(other.get()) - my_addr();
        return *this;
    }

    RelativePtr& operator=(RelativePtr&& other) {
        offset_ = to_u8(other.get()) - my_addr();
        return *this;
    }

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

private:

    uint8_t* to_u8(T* ptr) const noexcept {
        return reinterpret_cast<uint8_t*>(ptr);
    }

    const uint8_t* addr() const noexcept {
        return my_addr() + offset_;
    }

    uint8_t* my_addr() const noexcept {
        return const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(this));
    }
};



template <typename T>
class alignas(8) EmbeddingRelativePtr {
    uint8_t buffer_[8];
public:
    static constexpr size_t BUFFER_SIZE = 8;

    template <typename DV>
    static constexpr bool fits_in() {
        return std::is_trivially_copyable_v<DV> && sizeof(DV) < BUFFER_SIZE;
    }

    template <typename DT>
    static constexpr bool dt_fits_in() {
        using DV = DTTViewType<DT>;
        return DataTypeTraits<DT>::isFixedSize && fits_in<DV>() && TypeHashV<DT> < 128;
    }


    EmbeddingRelativePtr() noexcept
    {
        *as_u64() = 0;
    }

    EmbeddingRelativePtr(T* ptr) noexcept
    {
        set_offset(to_u8(ptr) - my_addr());
    }


    EmbeddingRelativePtr(uint8_t tag, const uint8_t* buf)
    {
        for (size_t c = 0; c < BUFFER_SIZE; c++) {
            buffer_[c] = buf[c];
        }

        uint8_t tagv = (tag << 1) | 0x01;
        buffer_[BUFFER_SIZE - 1] = tagv;
    }

    EmbeddingRelativePtr(const EmbeddingRelativePtr& other) noexcept
    {
        if (other.is_pointer()) {
            set_offset(to_u8(other.get()) - my_addr());
        }
        else {
            *as_u64() = *other.as_u64();
        }
    }

    EmbeddingRelativePtr(EmbeddingRelativePtr&& other) noexcept
    {
        if (other.is_pointer()) {
            set_offset(to_u8(other.get()) - my_addr());
        }
        else {
            *as_u64() = *other.as_u64();
        }
    }

    ~EmbeddingRelativePtr() noexcept = default;


    EmbeddingRelativePtr& operator=(const EmbeddingRelativePtr& other) noexcept {
        if (other.is_pointer()) {
            set_offset(to_u8(other.get()) - my_addr());
        }
        else {
            *as_u64() = *other.as_u64();
        }
        return *this;
    }

    EmbeddingRelativePtr& operator=(EmbeddingRelativePtr&& other) noexcept
    {
        if (other.is_pointer()) {
            set_offset(to_u8(other.get()) - my_addr());
        }
        else {
            *as_u64() = *other.as_u64();
        }

        return this;
    }

    const uint8_t* buffer() const noexcept {
        return buffer_;
    }

    void reset() noexcept {
        set_offset(0);
    }

    bool is_null() const noexcept {
        return *as_u64() == 0;
    }

    bool is_not_null() const noexcept {
        return *as_u64() != 0;
    }

    bool is_pointer() const noexcept
    {
        auto vv = buffer_[BUFFER_SIZE - 1];
        auto vv1 = vv & 0x01;
        return !vv1;
    }

    T* get() noexcept
    {
        if (MMA_LIKELY(is_pointer())) {
            return ptr_cast<T>(my_addr() + offset());
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Taking pointer form value-storing EmbeddingRelativePtr<T>").do_throw();
        }
    }

    T* get() const noexcept
    {
        if (MMA_LIKELY(is_pointer())) {
            return ptr_cast<T>(my_addr() + offset());
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Taking pointer form value-storing EmbeddingRelativePtr<T>").do_throw();
        }
    }



    int64_t offset() const noexcept
    {
        uint64_t val = *ptr_cast<uint64_t>(buffer_);
        uint64_t tag = val >> 56;
        val <<= 8;
        return static_cast<int64_t>(val | tag);
    }

    T* operator=(T* ptr) noexcept
    {
        if (MMA_LIKELY((bool)ptr)) {
            set_offset(to_u8(ptr) - my_addr());
        }
        else {
            set_offset(0);
        }
        return ptr;
    }

    T* operator->() noexcept {
        return get();
    }

    T* operator->() const noexcept {
        return get();
    }

    uint8_t get_tag() const noexcept {
        return buffer_[BUFFER_SIZE - 1] >> 1;
    }


    template <typename VT>
    std::enable_if_t <sizeof(VT) < BUFFER_SIZE, void>
            embed(uint8_t tag, VT data) noexcept
    {
        *ptr_cast<uint64_t>(buffer_) = uint64_t{};
        buffer_[BUFFER_SIZE - 1] = (tag << 1) | 0x01;
        *ptr_cast<VT>(buffer_) = data;
    }


    void copy_from(const EmbeddingRelativePtr& other) {
        for (size_t c = 0; c < BUFFER_SIZE; c++) {
            buffer_[c] = other.buffer_[c];
        }
    }

private:

    void set_offset(int64_t value)
    {
        uint64_t val  = static_cast<uint64_t>(value);
        uint64_t lowb = val << 56;
        uint64_t tgt  = (val >> 8) | lowb;

        *as_u64() = tgt;
    }

    uint64_t* as_u64() noexcept {
        return ptr_cast<uint64_t>(buffer_);
    }

    const uint64_t* as_u64() const noexcept {
        return ptr_cast<const uint64_t>(buffer_);
    }

    uint8_t* to_u8(T* ptr) const noexcept {
        return reinterpret_cast<uint8_t*>(ptr);
    }

    const uint8_t* addr() const noexcept {
        return my_addr() + offset();
    }

    uint8_t* my_addr() const noexcept {
        return const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(this));
    }
};

using ERelativePtr = EmbeddingRelativePtr<void>;

}}
