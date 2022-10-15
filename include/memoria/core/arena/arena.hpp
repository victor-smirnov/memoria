
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

#include <memoria/core/tools/result.hpp>
#include <memoria/core/memory/ptr_cast.hpp>

#include <memoria/core/memory/malloc.hpp>

#include <memoria/core/linked/common/arena.hpp>

#include <memoria/core/datatypes/datatype_ptrs.hpp>

#include <memoria/core/memory/memory.hpp>
#include <memoria/core/memory/ptr_cast.hpp>

#include <memoria/core/reflection/typehash.hpp>

#include <memoria/core/hermes/traits.hpp>

#include <memoria/core/flat_map/flat_hash_map.hpp>

#include <new>
#include <memory>
#include <vector>
#include <variant>

namespace memoria {

namespace hermes {
class HermesDocView;
}


namespace arena {

using ObjectTag = uint64_t;
static constexpr uint8_t SHORT_TYPETAG_LENGTH_BASE = 249;

constexpr size_t type_tag_size(ObjectTag tag)
{
    // Optimizing for small values;
    if (MMA_LIKELY(tag < SHORT_TYPETAG_LENGTH_BASE)) {
      return 1;
    }

    uint64_t mask = 0x00FF000000000000;
    for (size_t c = 8; c > 0; c--) {
      if (MMA_UNLIKELY((bool)(mask & tag))) {
        return c;
      }

      mask >>= 8;
    }

    return 1 + (tag >= SHORT_TYPETAG_LENGTH_BASE);
}

static inline constexpr ObjectTag read_type_tag(const uint8_t* array) noexcept
{
    uint64_t s0 = *(array - 1);
    if (MMA_LIKELY(s0 < SHORT_TYPETAG_LENGTH_BASE)) {
        return s0;
    }

    uint64_t value{};
    size_t size = s0 - SHORT_TYPETAG_LENGTH_BASE - 1;
    for (size_t c = 0; c < size; c++) {
        uint64_t token = *(array - c - 2);
        value |= token << (c * 8);
    }

    return value;
}

static inline ObjectTag read_type_tag(const void* addr) noexcept {
  return read_type_tag(ptr_cast<const uint8_t>(addr));
}


namespace detail_ {
    template <typename, typename = VoidT<>>
    struct HasObjectSizeHelper: std::false_type {};

    template <typename T>
    struct HasObjectSizeHelper<T, VoidT<decltype(T::UseObjectSize)>>: std::true_type {};
}


template <typename T>
constexpr bool HasObjectSize = detail_::HasObjectSizeHelper<T>::type::value;

template <typename T, bool HasObjectSize = HasObjectSize<T>>
struct ObjectSpaceAllocator;

template <typename T>
struct ObjectSpaceAllocator<T, false> {
    template <typename Arena, typename... CtrArgs>
    static auto allocate_object_space(size_t tag_size, Arena* arena, CtrArgs&&... args)
    {
        size_t size = sizeof(T);
        auto addr = arena->allocate_space(size, alignof(T), tag_size);
        return addr;
    }
};

template <typename T>
struct ObjectSpaceAllocator<T, true> {
    template <typename Arena, typename... CtrArgs>
    static auto allocate_object_space(size_t tag_size, Arena* arena, CtrArgs&&... args)
    {
        size_t size = T::object_size(std::forward<CtrArgs>(args)...);
        auto addr = arena->allocate_space(size, alignof(T), tag_size);
        return addr;
    }
};

enum class AllocationType {
    GROWABLE_SINGLE_CHUNK, MULTI_CHUNK
};

class ArenaAllocator;

template <typename T>
class AddrResolver {
    std::variant<T*, ptrdiff_t> storage_;
public:
    AddrResolver(): storage_() {}

    AddrResolver(T* addr) noexcept :
        storage_(addr)
    {}

    AddrResolver(ptrdiff_t offset) noexcept :
        storage_(offset)
    {}

    T* get(ArenaAllocator& arena) const noexcept;
};

class ArenaAllocator {
protected:
    using MemPtr = UniquePtr<uint8_t>;

    struct ChunkSize {
        size_t capacity{};
        size_t size{};
    };

    struct Chunk: ChunkSize {
        MemPtr memory;

        Chunk(MemPtr&& mem, size_t capacity) noexcept:
            ChunkSize({capacity, 0}),
            memory(std::move(mem))
        {}
    };

    AllocationType allocation_type_;
    size_t chunk_size_;
    std::vector<Chunk> chunks_;

    template <typename>
    friend class AddrResolver;

public:

    ArenaAllocator(AllocationType alc_type, size_t chunk_size, void* data, size_t data_size) noexcept :
        allocation_type_(alc_type),
        chunk_size_(chunk_size)
    {
        add_chunk(data_size);
        Chunk& head = this->head();
        memcpy(head.memory.get(), data, data_size);
    }


    ArenaAllocator(size_t chunk_size = 4096) noexcept :
        allocation_type_(AllocationType::MULTI_CHUNK),
        chunk_size_(chunk_size)
    {}

    ArenaAllocator(AllocationType alc_type, size_t chunk_size = 4096) noexcept :
        allocation_type_(alc_type),
        chunk_size_(chunk_size)
    {}

    ArenaAllocator(AllocationType alc_type, size_t chunk_size, const void* data, size_t segment_size):
        allocation_type_(alc_type),
        chunk_size_(chunk_size)
    {
        add_chunk(segment_size);
        head().size = segment_size;
        std::memcpy(head().memory.get(), data, segment_size);
    }

    ArenaAllocator(const ArenaAllocator&) = delete;



    bool is_chunked() const {
        return allocation_type_ == AllocationType::MULTI_CHUNK;
    }

    size_t chunk_size() const {
        return chunk_size_;
    }

    size_t chunks() const {
        return chunks_.size();
    }

    template <typename T>
    AddrResolver<T> get_resolver_for(T* addr)
    {
        if (allocation_type_ == AllocationType::MULTI_CHUNK) {
            return AddrResolver<T>(addr);
        }
        else {
            ptrdiff_t offset = reinterpret_cast<const uint8_t*>(addr) - head().memory.get();
            return AddrResolver<T>(offset);
        }
    }

    void switch_to_chunked_mode() {
        allocation_type_ = AllocationType::MULTI_CHUNK;
    }

    MMA_NODISCARD uint8_t* allocate_space(size_t size, size_t alignment, size_t tag_size)
    {
        if (MMA_UNLIKELY(chunks_.empty())) {
            return insert_into_new_chunk(size, alignment, tag_size);
        }

        ChunkSize chs = head();

        size_t addr   = align_up(chs.size, alignment);
        size_t upsize = addr - chs.size;

        while (upsize < tag_size) {
            addr += alignment;
            upsize += alignment;
        }

        if (chs.size + upsize + size <= chs.capacity) {
            head().size += (size + upsize);
            return head().memory.get() + addr;
        }
        else if (allocation_type_ == AllocationType::MULTI_CHUNK) {
            return insert_into_new_chunk(size, alignment, tag_size);
        }
        else {
            enlarge_chunk(upsize + size);

            head().size += (size + upsize);
            return head().memory.get() + addr;
        }
    }


    template <typename T>
    MMA_NODISCARD T* allocate_untagged_array(size_t size)
    {
        size_t alignment = alignof(T);
        T* array = ptr_cast<T>(allocate_space(size * sizeof(T), alignment, 0));

        for (size_t c = 0; c < size; c++) {
            (void)construct<T>(array + c);
        }

        return array;
    }

    template <typename T, typename... Args>
    MMA_NODISCARD T* allocate_object(Args&&... args)
    {
        constexpr ObjectTag tag = TypeHashV<T>;
        constexpr size_t tag_size = type_tag_size(tag);
        auto ptr = ObjectSpaceAllocator<T>::allocate_object_space(
            tag_size, this, std::forward<Args>(args)...
        );

        write_type_tag(ptr, tag);
        return this->template construct<T>(ptr, std::forward<Args>(args)...);
    }

    template <typename T, typename... Args>
    MMA_NODISCARD T* allocate_tagged_object(ObjectTag tag, Args&&... args)
    {
        size_t tag_size = type_tag_size(tag);
        auto ptr = ObjectSpaceAllocator<T>::allocate_object_space(
            tag_size, this, std::forward<Args>(args)...
        );

        write_type_tag(ptr, tag);
        return this->template construct<T>(ptr, std::forward<Args>(args)...);
    }

    template <typename T, typename... Args>
    MMA_NODISCARD T* allocate_object_untagged(Args&&... args) {
        auto ptr = ObjectSpaceAllocator<T>::allocate_object_space(
            0, this, std::forward<Args>(args)...
        );
        return this->template construct<T>(ptr, std::forward<Args>(args)...);
    }

    template <typename T, typename... Args>
    MMA_NODISCARD T* construct(void* ptr, Args&&... args)
    {
        return new (ptr) T(std::forward<Args>(args)...);
    }

    static void write_type_tag(uint8_t* base, ObjectTag tag) noexcept
    {
        uint8_t* buf = base - 1;
        if (MMA_LIKELY(tag < SHORT_TYPETAG_LENGTH_BASE)) {
            *buf = static_cast<uint8_t>(tag);
        }
        else {
            size_t tag_len = type_tag_size(tag);
            *buf = SHORT_TYPETAG_LENGTH_BASE + tag_len - 1;
            buf--;
            for (size_t c = 0; c < tag_len; c++) {
                *(buf - c) = static_cast<uint8_t>(tag >> (c * 8));
            }
        }
    }



    Chunk& head() {
        return chunks_[chunks_.size() - 1];
    }

private:

    void add_chunk(size_t size) {
        chunks_.emplace_back(
            allocate_system_zeroed<uint8_t>(size), size
        );
    }

    uint8_t* insert_into_new_chunk(size_t size, size_t alignment, size_t tag_size)
    {
        size_t addr   = 0;
        size_t upsize = 0;

        while (upsize < tag_size) {
            addr += alignment;
            upsize += alignment;
        }

        size_t alc_size = upsize + size;
        size_t chk_size = alc_size <= chunk_size_ ? chunk_size_ : alc_size;

        add_chunk(chk_size);

        head().size += alc_size;

        return head().memory.get() + addr;
    }





    void enlarge_chunk(size_t requested)
    {
        auto& chunk = head();
        size_t next_capaicty = chunk.capacity * 2;

        while (chunk.capacity + requested > next_capaicty)
        {
            next_capaicty *= 2;
        }

        auto new_ptr = allocate_system_zeroed<uint8_t>(next_capaicty);

        if (chunk.size > 0) {
            MemCpyBuffer(chunk.memory.get(), new_ptr.get(), chunk.size);
        }

        chunk.memory = std::move(new_ptr);
        chunk.capacity = next_capaicty;
    }

    static constexpr size_t align_up(size_t value, size_t alignment) noexcept
    {
        size_t mask = alignment - 1;
        size_t res = value & mask;

        if (MMA_UNLIKELY(res)) {
            return (value | mask) + 1;
        }

        return value;
    }
};

template <typename T>
inline T* AddrResolver<T>::get(ArenaAllocator& arena) const noexcept {
    if (storage_.index() == 0) {
        return std::get<0>(storage_);
    }
    else {
        ptrdiff_t offset = std::get<1>(storage_);
        return ptr_cast<T>(arena.head().memory.get() + offset);
    }
}

}}
