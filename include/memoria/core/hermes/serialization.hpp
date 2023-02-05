
// Copyright 2023 Victor Smirnov
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

#include <memoria/core/hermes/traits.hpp>

#include <memoria/core/memory/memory.hpp>

#include <memoria/core/tools/arena_buffer.hpp>
#include <memoria/core/tools/iostreams.hpp>
#include <memoria/core/tools/result.hpp>
#include <memoria/core/tools/bitmap.hpp>

#include <memoria/core/flat_map/flat_hash_map.hpp>
#include <memoria/core/arena/arena.hpp>
#include <memoria/core/hermes/common.hpp>

#include <memoria/core/tools/dump.hpp>

#include <variant>
#include <functional>
#include <stack>

namespace memoria {

class TypeReflection;

namespace hermes {

class DeepCopyFnObj;
class DeepCopyState;

class DeepCopyState {
    arena::ArenaAllocator& arena_;
    LWMemHolder* mem_holder_;

    ska::flat_hash_map<const void*, arena::AddrResolver<void>> addr_map_;

public:
    DeepCopyState(arena::ArenaAllocator& arena, LWMemHolder* mem_holder):
        arena_(arena), mem_holder_(mem_holder)
    {}

    DeepCopyState(arena::ArenaAllocator& arena):
        arena_(arena), mem_holder_(nullptr)
    {}

    void set_mem_holder(LWMemHolder* mem_holder) {
        mem_holder_ = mem_holder;
    }

    arena::ArenaAllocator& arena() {return arena_;}
    LWMemHolder* mem_holder() {return mem_holder_;}

    template <typename T>
    T* resolve(arena::ArenaAllocator& arena, const T* src) noexcept
    {
        auto ii = addr_map_.find(src);
        if (ii != addr_map_.end()) {
            return reinterpret_cast<T*>(ii->second.get(arena));
        }
        return nullptr;
    }

    template <typename T>
    void map(arena::ArenaAllocator& arena, const T* src, T* dst) {
        addr_map_[src] = arena.get_resolver_for(static_cast<void*>(dst));
    }
};


class SimpleBitmap {
    using UnitT = uint64_t;
    static constexpr size_t UNIT_BITS = sizeof(UnitT) * 8;

    size_t units_;
    UniquePtr<UnitT> buffer_;
    size_t size_;
    size_t capacity_;
public:
    SimpleBitmap():
        units_(0),
        buffer_(nullptr, ::free),
        size_(0), capacity_(0)
    {}

    SimpleBitmap(size_t size):
        units_(div_up(size, UNIT_BITS)),
        buffer_(allocate_system_zeroed<UnitT>(units_)),
        size_(size),
        capacity_(size)
    {}

    void resize(size_t size)
    {
        if (size <= capacity_)
        {
            size_ = size;

            if (units_) {
                std::memset(buffer_.get(), 0, units_ * sizeof(UnitT));
            }
        }
        else {
            units_  = div_up(size, UNIT_BITS);
            buffer_ = allocate_system_zeroed<UnitT>(units_);
            size_ = capacity_ = size;
        }
    }

    size_t size() const {
        return size_;
    }

    bool test_bit(size_t idx) const
    {
        if (idx < size_) {
            return TestBit(buffer_.get(), idx);
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Range check in SimpleBitmap::test_bit(): {} {}", idx, size_).do_throw();
        }
    }

    void set_bit(size_t idx)
    {
        if (idx < size_) {
            SetBit(buffer_.get(), idx);
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Range check in SimpleBitmap::set_bit(): {} {}", idx, size_).do_throw();
        }
    }

    void clear_bit(size_t idx)
    {
        if (idx < size_) {
            ClearBit(buffer_.get(), idx);
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Range check in SimpleBitmap::clear_bit(): {} {}", idx, size_).do_throw();
        }
    }

    bool check_and_set(size_t start, size_t end)
    {
        if (start <= end && end <= size_)
        {
            UnitT* buf = buffer_.get();
            for (size_t c = start; c < end; c += UNIT_BITS)
            {
                size_t remainder = end - c;
                size_t bits_num = remainder >= UNIT_BITS ? UNIT_BITS : remainder;
                UnitT bits = GetBits(buf, c, bits_num);
                if (!bits) {
                    SetBits(buf, c, 0xFFFFFFFFFFFFFFFF, bits_num);
                }
                else {
                    return false;
                }
            }

            return true;
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR(
                "Range check error in SimpleBitmap::check_and_set(): {}, {}, {}",
                start, end, size_
            ).do_throw();
        }
    }
};

class CheckStructureState {
    arena::ArenaAllocator& arena_;
    LWMemHolder* mem_holder_;

    ska::flat_hash_set<void*> addr_map_;

    size_t depth_max_{256};
    size_t depth_{};

    const void* segment_start_;
    const void* segment_end_;

    SimpleBitmap allocation_bitmap_;
    SimpleBitmap cycle_bitmap_;

public:
    CheckStructureState(arena::ArenaAllocator& arena, LWMemHolder* mem_holder):
        arena_(arena), mem_holder_(mem_holder)
    {
        configure_bounds();
    }

    CheckStructureState(arena::ArenaAllocator& arena):
        arena_(arena), mem_holder_(nullptr)
    {
        configure_bounds();
    }

    void set_mem_holder(LWMemHolder* mem_holder) {
        mem_holder_ = mem_holder;
    }

    arena::ArenaAllocator& arena() {return arena_;}
    LWMemHolder* mem_holder() {return mem_holder_;}

    void enter()
    {
        if (depth_ < depth_max_) {
            depth_++;
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR(
                "Internal error, SerializationState depth maximum of {} exceeded", depth_max_
            ).do_throw();
        }
    }

    void exit()
    {
        if (depth_) {
            depth_--;
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Internal error, SerializationState depth is zero").do_throw();
        }
    }

    template <typename... Args>
    void check_bounds(const void* ptr, const char* src, const char* msg, Args&&... args) const {
        if (ptr < segment_start_ || ptr >= segment_end_) {
            make_generic_error_with_source(src, msg, std::forward<Args>(args)...).do_throw();
        }
    }


    void check_bounds(const void* ptr, const char* src) const {
        check_bounds(ptr, src, "Pointer bounds check: {} {} {}", ptr, segment_start_, segment_end_);
    }

    void check_ptr(const arena::EmbeddingRelativePtr<void>& ptr, const char* src);
    void check_ptr(const arena::RelativePtr<void>& ptr, const char* src);
    void check_ptr(const void* ptr, const char* src);

    size_t offset(const void* ptr) const
    {
        return reinterpret_cast<const uint8_t*>(ptr) -
                reinterpret_cast<const uint8_t*>(segment_start_);
    }

    template <typename T>
    void check_alignment(const void* ptr, const char* src){
        check_alignment(ptr, alignof(T), src);
    }

    void check_alignment(const void* ptr, size_t alignment, const char* src)
    {
        size_t mem = reinterpret_cast<size_t>(ptr);
        size_t alc_bits = Log2U(alignment) - 1;
        size_t mask = make_bitmask<size_t>(alc_bits);

        if (mem & mask)
        {
            MEMORIA_MAKE_GENERIC_ERROR(
                "Invalid pointer aligment of {}, expected {} at {}",
                mem & mask, alignment, src
            ).do_throw();
        }
    }

    void mark_as_processed(const void* ptr) {
        cycle_bitmap_.set_bit(offset(ptr));
    }

    void check_unique_and_mark_as_processed(const void* ptr, const char* src)
    {
        size_t idx = offset(ptr);
        if (!cycle_bitmap_.test_bit(idx)) {
            cycle_bitmap_.set_bit(offset(ptr));
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR(
                "Requested object at offset {} has been already processed at {}",
                idx, src
            ).do_throw();
        }
    }


    void check_and_set_tagged(const void* ptr, size_t size, const char* src);
    void check_and_set(const void* ptr, size_t size, const char* src);

private:
    void configure_bounds()
    {
        if (arena_.chunks() == 1)
        {
            auto& chunk = arena_.head();
            segment_start_ = chunk.memory.get();
            segment_end_   = chunk.memory.get() + chunk.size;

            allocation_bitmap_.resize(chunk.size);
            cycle_bitmap_.resize(chunk.size);
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Arena is empty or multi-chunk. Chunks: {}", arena_.chunks());
        }
    }
};


class CheckStructureScope {
    CheckStructureState& state_;
public:
    CheckStructureScope(CheckStructureState& state): state_(state) {
        state_.enter();
    }

    ~CheckStructureScope() {
        state_.exit();
    }
};


class SerializationState {
    PoolSharedPtr<ArenaBuffer<uint8_t>> buffer_;
    size_t prefix_size_{};
    IBinaryOutputStream& stream_;
    ska::flat_hash_map<void*, size_t> mapping_;

    size_t depth_max_{256};
    size_t depth_{};

    LWMemHolder* mem_holder_;

public:
    static constexpr size_t BUFFER_SIZE = 64*1024;

    SerializationState(IBinaryOutputStream& stream, LWMemHolder* mem_holder):
        buffer_(TL_get_reusable_shared_instance<ArenaBuffer<uint8_t>>()),
        stream_(stream),
        mem_holder_(mem_holder)
    {
        reserve_header();
    }

    LWMemHolder* mem_holder() {
        return mem_holder_;
    }

    size_t capacity() const noexcept
    {
        size_t size = buffer_->size();
        return BUFFER_SIZE >= size ? BUFFER_SIZE - size : 0;
    }

    void reserve(size_t n) {
        for (size_t c = 0; c < n; c++) {
            buffer_->emplace_back(0);
        }
    }

    void write_u32(uint32_t value) {
        for (size_t c = 0; c < 4; c++, value >>= 8) {
            buffer_->emplace_back(value & 0xFFu);
        }
    }

    void write_u32(size_t at, uint32_t value) {
        for (size_t c = at; c < at + 4; c++, value >>= 8) {
            buffer_->operator[](c) = value & 0xFFu;
        }
    }

    void write_u64(uint64_t value) {
        for (size_t c = 0; c < 8; c++, value >>= 8) {
            buffer_->emplace_back(value & 0xFFull);
        }
    }

    void write_u64(size_t at, uint64_t value) {
        for (size_t c = at; c < at + 8; c++, value >>= 8) {
            buffer_->operator[](c) = value & 0xFFull;
        }
    }

    void write_u64_vl(uint64_t value)
    {
        size_t len = 8;
        for (size_t c = 0; c < 8; c++) {
            uint64_t mask = 0xFFull << (7 - c) * 8;
            bool set = value & mask;
            if (!set) {
                len--;
            }
            else {
                break;
            }
        }

        write_u8(len);
        for (size_t c = 0; c < len; c++, value >>= 8) {
            buffer_->emplace_back(value & 0xFFu);
        }
    }

    void write_u8(uint8_t value) {
        buffer_->emplace_back(value);
    }

    void flush()
    {
        stream_.write(buffer_->data(), buffer_->size());
        buffer_->clear();
        reserve_header();
    }

    void enter()
    {
        if (depth_ < depth_max_) {
            depth_++;
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR(
                "Internal error, SerializationState depth maximum of {} exceeded", depth_max_
            ).do_throw();
        }
    }

    void exit()
    {
        if (depth_) {
            depth_--;
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Internal error, SerializationState depth is zero").do_throw();
        }
    }

protected:
    void reserve_header() {
        reserve(4);
    }
};

class SerScope {
    SerializationState& state_;
public:
    SerScope(SerializationState& state): state_(state) {
        state_.enter();
    }

    ~SerScope() {
        state_.exit();
    }
};

class DeserializationState {
public:
};

}}
