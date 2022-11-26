
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

private:
    RelativePtr& operator=(const RelativePtr&) = delete;
    RelativePtr& operator=(RelativePtr&&) = delete;


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
        return DataTypeTraits<DT>::isFixedSize && fits_in<DV>();
    }

    EmbeddingRelativePtr() noexcept
    {
        *as_u64() = 0;
    }

    EmbeddingRelativePtr(T* ptr) noexcept
    {
        set_offset(to_u8(ptr) - my_addr());
    }

    template <typename DV>
    EmbeddingRelativePtr(uint8_t tag, DV dv) {
        embed(tag, dv);
    }


    ~EmbeddingRelativePtr() noexcept = default;
private:
    EmbeddingRelativePtr(const EmbeddingRelativePtr&) = default;
    EmbeddingRelativePtr(EmbeddingRelativePtr&& ptr) noexcept = default;

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

public:
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

//    // alignof = 1
//    void* allocate_in(uint8_t tag) noexcept
//    {
//        buffer_[BUFFER_SIZE - 1] = (tag << 1) | 0x01;
//        return buffer_;
//    }

    template <typename VT>
    std::enable_if_t <sizeof(VT) < BUFFER_SIZE, void>
            embed(uint8_t tag, VT data) noexcept
    {
        *ptr_cast<uint64_t>(buffer_) = uint64_t{};
        buffer_[BUFFER_SIZE - 1] = (tag << 1) | 0x01;
        *ptr_cast<VT>(buffer_) = data;
    }

//    template <typename VT>
//    std::enable_if_t <sizeof(VT) == BUFFER_SIZE, void>
//            embed(uint8_t tag, VT data) noexcept
//    {
//        const uint8_t* view = reinterpret_cast<const VT*>(&data);

//        for (size_t c = 0; c < 8; c++) {
//            buffer_[c] = view[c];
//        }

//        buffer_[7] = (tag << 1) | 0x01;
//    }

//    template <typename VT>
//    std::enable_if_t <sizeof(VT) < BUFFER_SIZE, void>
//            extract(uint8_t& tag, VT& data) const noexcept
//    {
//        tag = buffer_[7] >> 1;
//        data = *ptr_cast<const VT>(buffer_);
//    }

//    template <typename VT>
//    std::enable_if_t <sizeof(VT) == BUFFER_SIZE, void>
//            extract(uint8_t& tag, VT& data) const noexcept
//    {
//        uint64_t val = *as_u64();
//        tag = val >> 57;
//        val = val & 0xFF00000000000000;
//        data = *ptr_cast<const VT>(&val);
//    }

    void copy_from(const EmbeddingRelativePtr& other) {
        for (size_t c = 0; c < 8; c++) {
            buffer_[c] = other.buffer_[c];
        }
    }

private:
    EmbeddingRelativePtr& operator=(const EmbeddingRelativePtr&) = delete;
    EmbeddingRelativePtr& operator=(EmbeddingRelativePtr&&) = delete;

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
