
// Copyright 2013-2022 Victor Smirnov
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
#include <memoria/core/memory/malloc.hpp>

#include <memoria/core/packed/tools/packed_allocator_types.hpp>
#include <memoria/profiles/common/block_operations.hpp>

#include <memoria/core/tools/bitmap.hpp>
#include <memoria/core/tools/dump.hpp>
#include <memoria/core/tools/assert.hpp>
#include <memoria/core/tools/span.hpp>

#include <memoria/core/memory/ptr_cast.hpp>

#include <type_traits>

namespace memoria {

enum class PackedBlockType {
    RAW_MEMORY = 0, ALLOCATABLE = 1
};


class alignas(8) PackedAllocator {


    using MyType    = PackedAllocator;
    using Allocator = PackedAllocator;

public:
    using Bitmap = uint64_t;


    static constexpr uint32_t VERSION = 1;

private:
    PackedAllocatable allocatable_;

    psize_t block_size_;
    psize_t layout_size_;
    psize_t bitmap_size_;

    uint8_t buffer_[1];

public:

    using FieldsList = MergeLists<
        typename PackedAllocatable::FieldsList,

        UInt32Value<VERSION>,
        decltype(block_size_),
        decltype(layout_size_),
        decltype(bitmap_size_)
    >;

    PackedAllocator() noexcept = default;

    PackedAllocatable& allocatable() noexcept {return allocatable_;}
    const PackedAllocatable& allocatable() const noexcept {return allocatable_;}


    bool is_allocatable(size_t idx) const noexcept
    {
        const Bitmap* bmp = bitmap();
        return GetBit(bmp, idx);
    }

    Bitmap* bitmap() noexcept {
        return ptr_cast<Bitmap>(buffer_ + layout_size_);
    }

    const Bitmap* bitmap() const  {
        return ptr_cast<const Bitmap>(buffer_ + layout_size_);
    }

    size_t allocated() const noexcept {
        return element_offset(elements());
    }

    size_t client_area() const noexcept {
        size_t m_s = my_size();
        return block_size_ - m_s - layout_size_ - bitmap_size_;
    }

    size_t free_space() const noexcept {
        size_t client_area  = this->client_area();
        size_t allocated    = this->allocated();
        return client_area - allocated;
    }

    size_t total_free_space() const noexcept
    {
        size_t space {};
        if (allocatable_.allocator_offset()) {
            const Allocator* alloc = allocatable_.allocator();
            space = alloc->total_free_space();
        }

        space += free_space();

        return space;
    }

    size_t elements() const noexcept {
        return layout_size_/4 - 1;
    }

    uint8_t* base() noexcept {
        return buffer_ + layout_size_ + bitmap_size_;
    }

    const uint8_t* base() const noexcept {
        return buffer_ + layout_size_ + bitmap_size_;
    }

    size_t layout_size() const noexcept {
        return layout_size_;
    }

    size_t bitmap_size() const noexcept {
        return bitmap_size_;
    }

    size_t block_size() const noexcept {
        return block_size_;
    }

    void set_block_size(size_t block_size) noexcept {
        this->block_size_ = block_size;
    }

    void init(size_t block_size, size_t blocks) noexcept
    {
        block_size_ = PackedAllocatable::round_down_bytes(block_size);

        size_t layout_blocks = blocks + (blocks % 2 ? 1 : 2);

        layout_size_ = layout_blocks * sizeof(psize_t);

        memset(buffer_, 0, layout_size_);

        bitmap_size_ = PackedAllocatable::round_up_bits(layout_blocks);

        Bitmap* bitmap = this->bitmap();
        memset(bitmap, 0, bitmap_size_);
    }

    static constexpr size_t empty_size(size_t blocks) noexcept
    {
        return block_size(0, blocks);
    }

    static constexpr size_t block_size(size_t client_area, size_t blocks) noexcept
    {
        return PackedAllocatable::round_up_bytes(
                my_size() +
                (blocks + (blocks % 2 ? 1 : 2)) * sizeof(psize_t) +
                PackedAllocatable::round_up_bits(blocks) +
                PackedAllocatable::round_up_bytes(client_area)
        );
    }

    static constexpr size_t client_area(size_t block_size, size_t blocks) noexcept
    {
        return PackedAllocatable::round_down_bytes(
                block_size -
                (my_size() + (blocks + (blocks % 2 ? 1 : 2)) * sizeof(psize_t)
                 + PackedAllocatable::round_up_bits(blocks))
        );
    }

    size_t compute_element_offset(const void* element) const noexcept
    {
        const uint8_t* base_ptr = base();
        const uint8_t* elt_ptr = ptr_cast<const uint8_t>(element);

        ptrdiff_t diff = reinterpret_cast<ptrdiff_t>(elt_ptr - base_ptr);

        return diff;
    }

    size_t resize_block(const void* element, size_t new_size)
    {
        size_t idx = find_element(element);
        return resize_block(idx, new_size);
    }

    size_t resize_block(size_t idx, size_t new_size)
    {
        MEMORIA_ASSERT(new_size, <=, std::numeric_limits<psize_t>::max());

        size_t allocation_size = PackedAllocatable::round_up_bytes(new_size);
        size_t size  = element_size(idx);
        if (allocation_size > size)
        {
            size_t delta = allocation_size - size;

            size_t free_space = this->free_space();
            if (delta > free_space)
            {
                enlarge(delta - free_space);
            }

            move_elements_up(idx + 1, delta);
        }
        else if (allocation_size < size)
        {
            size_t delta = size - allocation_size;
            move_elements_down(idx + 1, delta);

            if (allocatable_.allocator_offset())
            {
                pack();
            }
        }

        return allocation_size;
    }

    bool try_allocation(size_t idx, size_t new_size)
    {
        size_t allocation_size = PackedAllocatable::round_up_bytes(new_size);

        size_t size  = element_size(idx);
        size_t delta = allocation_size - size;

        if (delta > 0)
        {
            size_t free_space = this->free_space();
            if (delta > free_space)
            {
                if (allocatable_.allocator_offset() > 0)
                {
                    Allocator* alloc = allocatable_.allocator();
                    size_t my_idx = find_element(this);
                    return alloc->try_allocation(my_idx, delta - free_space);
                }
                else {
                    return false;
                }
            }
        }

        return true;
    }

    const Allocator* parent() const {
        return allocatable_.allocator_or_null();
    }

    Allocator* parent() {
        return allocatable_.allocator_or_null();
    }

    const Allocator* get_top_level() const
    {
        const Allocator* top_level_alloc = this;
        while (top_level_alloc->allocatable_.allocator_offset() > 0)
        {
            top_level_alloc = allocatable_.allocator();
        }

        return top_level_alloc;
    }

    Allocator* get_top_level()
    {
        Allocator* top_level_alloc = this;
        while (top_level_alloc->allocatable_.allocator_offset() > 0)
        {
            top_level_alloc = allocatable_.allocator();
        }

        return top_level_alloc;
    }


    psize_t* layout()  {
        return ptr_cast<psize_t>(buffer_);
    }

    const psize_t* layout() const  {
        return ptr_cast<const psize_t>(buffer_);
    }

    const psize_t& element_offset(size_t idx) const
    {
        return *(ptr_cast<const psize_t>(buffer_) + idx);
    }



    //TODO: rename to segment_size ?
    MMA_NODISCARD size_t element_size(size_t idx) const
    {
        size_t size2 = element_offset(idx + 1);
        size_t size1 = element_offset(idx);
        return size2 - size1;
    }

    size_t element_size_by_ptr(const void* element_ptr) const
    {
        size_t idx = find_element(element_ptr);
        return element_size(idx);
    }


    size_t find_element(const void* element_ptr) const
    {
        size_t offset  = compute_element_offset(element_ptr);

        MEMORIA_ASSERT(offset, >=, 0);

        for (psize_t c = 0; c < layout_size_ / 4; c++)
        {
            if (offset < element_offset(c))
            {
                return c - 1;
            }
        }

        MEMORIA_MAKE_GENERIC_ERROR("Requested element is not found in this allocator").do_throw();
    }


    template <typename T>
    const T* get(size_t idx) const
    {
        static_assert(IsPackedAlignedV<T>, "Invalid type alignment for packed allocator (8 bytes at most)");

        const T* addr = ptr_cast<const T>(base() + element_offset(idx));
        return addr;
    }

    template <typename T>
    T* get(size_t idx)
    {
        static_assert(IsPackedAlignedV<T>, "Invalid type alignment for packed allocator (8 bytes at most)");
        T* addr = ptr_cast<T>(base() + element_offset(idx));
        return addr;
    }

    bool is_empty(size_t idx) const
    {
        return element_size(idx) == 0;
    }

    AllocationBlock describe(size_t idx)
    {
        size_t offset  = element_offset(idx);
        size_t size    = element_size(idx);

        return AllocationBlock(size, offset, base() + offset);
    }

    AllocationBlockConst describe(size_t idx) const
    {
        size_t offset  = element_offset(idx);
        size_t size    = element_size(idx);

        return AllocationBlockConst(size, offset, base() + offset);
    }

    template <typename T>
    T* allocate(size_t idx, size_t block_size)
    {
        static_assert(IsPackedStructV<T>, "May allocate only Standard Layout types having PackedAllocatable as header");
        static_assert(IsPackedAlignedV<T>, "Invalid type alignment for packed allocator (8 bytes at most)");

        auto block = allocate(idx, block_size, PackedBlockType::ALLOCATABLE);

        T* object = block.cast<T>();

        object->init(block.size());

        return object;
    }

    template <typename T>
    T* allocate_space(size_t idx, size_t block_size)
    {
        static_assert(IsPackedAlignedV<T>, "Invalid type alignment for packed allocator (8 bytes at most)");
        static_assert(IsPackedStructV<T>, "May allocate only Standard Layout types having PackedAllocatable as header");

        auto block = allocate(idx, block_size, PackedBlockType::ALLOCATABLE);

        return block.cast<T>();
    }

    template <typename T>
    T* allocate_empty(size_t idx, bool do_init = true)
    {
        static_assert(IsPackedStructV<T>, "May allocate only Standard Layout types having PackedAllocatable as header");
        static_assert(IsPackedAlignedV<T>, "Invalid type alignment for packed allocator (8 bytes at most)");

        size_t block_size = T::empty_size();

        auto block = allocate(idx, block_size, PackedBlockType::ALLOCATABLE);

        T* object = block.cast<T>();

        if (do_init) {
            object->init();
        }

        return object;
    }


    template <typename T>
    T* allocate_default(size_t idx)
    {
        static_assert(IsPackedStructV<T>, "May allocate only Standard Layout types having PackedAllocatable as header");
        static_assert(IsPackedAlignedV<T>, "Invalid type alignment for packed allocator (8 bytes at most)");

        size_t available_client_area = this->free_space();
        size_t block_size = T::default_size(available_client_area);

        auto block = allocate(idx, block_size, PackedBlockType::ALLOCATABLE);

        T* object = block.cast<T>();

        object->init_default(block_size);

        return object;
    }



    PackedAllocator* allocate_allocator(size_t idx, size_t streams)
    {
        size_t block_size = PackedAllocator::empty_size(streams);

        auto block = allocate(idx, block_size, PackedBlockType::ALLOCATABLE);

        PackedAllocator* object = block.cast<PackedAllocator>();

        object->init(block_size, streams);

        return object;
    }


    template <typename T>
    T* allocate(size_t idx)
    {
        static_assert(IsPackedAlignedV<T>, "Invalid type alignment for packed allocator (8 bytes at most)");
        static_assert(!std::is_base_of<PackedAllocatable, T>::value,
                "Only classes that are not derived from PackedAllocatable "
                "should be instantiated this way");

        auto block = allocate(idx, sizeof(T), PackedBlockType::RAW_MEMORY);
        return block.cast<T>();
    }

    template <typename T>
    T* allocate_array_by_length(size_t idx, size_t length)
    {
        static_assert(IsPackedAlignedV<T>, "Invalid type alignment for packed allocator (8 bytes at most)");
        static_assert(!std::is_base_of<PackedAllocatable, T>::value,
                "Only classes that are not derived from PackedAllocatable "
                "should be instantiated this way");

        auto block = allocate(idx, length, PackedBlockType::RAW_MEMORY);
        return block.cast<T>();
    }

    template <typename T>
    T* allocate_array_by_size(size_t idx, size_t size)
    {
        static_assert(IsPackedAlignedV<T>, "Invalid type alignment for packed allocator (8 bytes at most)");
        static_assert(!std::is_base_of<PackedAllocatable, T>::value,
                "Only classes that are not derived from PackedAllocatable "
                "should be instantiated this way");

        auto block = allocate(idx, sizeof(T) * size, PackedBlockType::RAW_MEMORY);
        return block.cast<T>();
    }


    AllocationBlock allocate(size_t idx, size_t size, PackedBlockType type)
    {
        size_t allocation_size = PackedAllocatable::round_up_bytes(size);

        set_block_type(idx, type);

        resize_block(idx, allocation_size);

        size_t offset = element_offset(idx);

        memset(base() + offset, 0, allocation_size);

        if (type == PackedBlockType::ALLOCATABLE)
        {
            PackedAllocatable* alc = ptr_cast<PackedAllocatable>(base() + offset);
            alc->set_allocator_offset(this);
        }

        auto offs = base() + offset;

        MEMORIA_V1_ASSERT_ALIGN(offs, 8);

        return AllocationBlock(allocation_size, offset, base() + offset);
    }

    void import_block(size_t idx, const PackedAllocator* src, size_t src_idx)
    {
        auto src_block  = src->describe(src_idx);
        auto type       = src->block_type(src_idx);

        resize_block(idx, src_block.size());

        set_block_type(idx, type);

        auto tgt_block  = this->describe(idx);

        const uint8_t* src_ptr    = src_block.ptr();
        uint8_t* tgt_ptr          = tgt_block.ptr();

        CopyByteBuffer(src_ptr, tgt_ptr, src_block.size());

        if (src_block.size() > 0 && type == PackedBlockType::ALLOCATABLE)
        {
            PackedAllocatable* element = ptr_cast<PackedAllocatable>(tgt_block.ptr());
            element->set_allocator_offset(this);
        }
    }

    void free(size_t idx)
    {
        size_t size = element_size(idx);
        if (size > 0)
        {
            move_elements_down(idx + 1, size);

            if (allocatable_.allocator_offset() > 0) {
                pack();
            }
        }
    }

    void clear(size_t idx)
    {
        auto block = describe(idx);
        memset(block.ptr(), 0, block.size());
    }

    void set_block_type(size_t idx, PackedBlockType type)
    {
        Bitmap* bitmap = this->bitmap();
        SetBit(bitmap, idx, type == PackedBlockType::ALLOCATABLE);
    }

    PackedBlockType block_type(size_t idx) const
    {
        const Bitmap* bitmap = this->bitmap();
        return GetBit(bitmap, idx) ? PackedBlockType::ALLOCATABLE : PackedBlockType::RAW_MEMORY;
    }

    void dump(std::ostream& out = std::cout) const
    {
        out << "PackedAllocator Layout:" << std::endl;
        out << "Block Size: " << block_size_ << std::endl;
        out << "Layout Size: " << layout_size_ << std::endl;
        out << "Bitmap Size: " << bitmap_size_ << std::endl;

        out << "Allocated Size: " << allocated() << std::endl;
        out << "ClientArea Size: " << client_area() << std::endl;
        out << "FreeSpace Size: " << free_space() << std::endl;

        dump_layout(out);

        out << "PackedAllocator Block Types Bitmap:" << std::endl;
        const Bitmap* bitmap = this->bitmap();

        dumpSymbols<Bitmap>(out, layout_size_/4, 1, [bitmap](size_t idx){
            return GetBit(bitmap, idx);
        });
    }

    void dump_allocator(std::ostream& out = std::cout) const {
    	dump(out);
    }

    void dump_layout(std::ostream& out = std::cout) const
    {
        dumpArray<size_t>(out, layout_size_/4, [this](size_t idx){
            return this->element_offset(idx);
        });
    }


    size_t enlarge(size_t delta)
    {
        return resize(PackedAllocatable::round_up_bytes(block_size_ + delta));
    }

    size_t shrink(size_t delta)
    {
        return resize(PackedAllocatable::round_up_bytes(block_size_ - delta));
    }

    void resize_block(size_t new_size)
    {
        block_size_ = new_size;
    }

    size_t resize(size_t new_size)
    {
        if (allocatable_.allocator_offset() > 0)
        {
            Allocator* alloc = allocatable_.allocator();
            auto block_size_tmp = alloc->resize_block(this, new_size);
            block_size_ = block_size_tmp;
        }
        else if (new_size > block_size_)
        {
            std::cout << "OOM Error: " << new_size << " :: " << block_size_ << std::endl;
            MEMORIA_MAKE_PACKED_OOM_ERROR().do_throw();
        }

        return block_size_;
    }

    size_t pack()
    {
        size_t free_space = this->free_space();
        return resize(block_size_ - free_space);
    }

    MMA_NODISCARD size_t compute_free_space_up() const
    {
        size_t space{};

        const PackedAllocator* alloc = this;

        while (alloc)
        {
            space += alloc->free_space();
            alloc = alloc->allocatable_.allocator_or_null();
        }

        return space;
    }


    void generateDataEvents(IBlockDataEventHandler* handler) const
    {
        handler->startGroup("ALLOCATOR");

        handler->value("PARENT_ALLOCATOR", &allocatable_.allocator_offset());

        handler->value("BLOCK_SIZE",    &block_size_);

        size_t client_area = this->client_area();
        size_t free_space  = this->free_space();

        handler->value("CLIENT_AREA",  &client_area);
        handler->value("FREE_SPACE",   &free_space);

        handler->value("LAYOUT_SIZE",   &layout_size_);
        handler->value("BITMAP_SIZE",   &bitmap_size_);

        handler->endGroup();
    }


    template <typename SerializationData>
    void serialize(SerializationData& buf) const
    {
        FieldFactory<psize_t>::serialize(buf, allocatable_.allocator_offset_);
        FieldFactory<psize_t>::serialize(buf, block_size_);
        FieldFactory<psize_t>::serialize(buf, layout_size_);
        FieldFactory<psize_t>::serialize(buf, bitmap_size_);

        psize_t layout_size = layout_size_ / 4;

        FieldFactory<psize_t>::serialize(buf, layout(), layout_size);

        FieldFactory<Bitmap>::serialize(buf, bitmap(), bitmap_size_/sizeof(Bitmap));
    }


    template <typename DeserializationData>
    void deserialize(DeserializationData& buf)
    {
        FieldFactory<psize_t>::deserialize(buf, allocatable_.allocator_offset_);
        FieldFactory<psize_t>::deserialize(buf, block_size_);
        FieldFactory<psize_t>::deserialize(buf, layout_size_);
        FieldFactory<psize_t>::deserialize(buf, bitmap_size_);

        psize_t layout_size = layout_size_ / 4;

        FieldFactory<psize_t>::deserialize(buf, layout(), layout_size);

        FieldFactory<Bitmap>::deserialize(buf, bitmap(), bitmap_size_/sizeof(Bitmap));
    }

    template <typename T, typename SerializationData>
    void serializeSegment(SerializationData& buf, size_t segment) const
    {
        auto data = this->describe(segment);
        FieldFactory<T>::serialize(buf, ptr_cast<const T>(data.ptr()), data.size() / (psize_t)sizeof(T));
    }

    template <typename T, typename DeserializationData>
    void deserializeSegment(DeserializationData& buf, size_t segment)
    {
        auto data = this->describe(segment);
        FieldFactory<T>::deserialize(buf, ptr_cast<T>(data.ptr()), data.size() / (psize_t)sizeof(T));
    }


    constexpr static size_t my_size()  {
        return sizeof(MyType);
    }


    void print_layout() const
    {
        println("Packed allocator layout: {} {} {}", layout_size_/4, bitmap_size_, block_size_);
        for (size_t c = 0; c < layout_size_/4; c++) {
            AllocationBlockConst block = describe(c);
            println("\tblock: {} :: {} :: {}", c, block.offset(), block.size());
        }
    }


    PackedAllocatorUpdateState make_allocator_update_state()
    {
        const Allocator* alloc = get_top_level();
        return PackedAllocatorUpdateState(alloc->free_space());
    }

    void check_blocks() const
    {
        size_t total = allocated();
        if (total > block_size_) {
            MEMORIA_MAKE_GENERIC_ERROR(
                        "Total allocated data for PackedAllocator exceeds its block_size: {} {}",
                        total,
                        block_size_
            ).do_throw();
        }

        const PackedAllocator* parent = this->parent();
        if (parent) {
            size_t my_size = parent->element_size_by_ptr(this);
            if (my_size < block_size_) {
                MEMORIA_MAKE_GENERIC_ERROR(
                            "PackedAllocator's block_size_ > its parent's allocator element size: {} {}",
                            block_size_,
                            my_size
                ).do_throw();
            }
        }
    }

    void for_each_block(std::function<void (size_t, AllocationBlockConst)> fn) const {
        size_t size = elements();
        for (size_t s = 0; s < size; s++) {
            fn(s, describe(s));
        }
    }

private:
    psize_t& set_element_offset(size_t idx)
    {
        return *(ptr_cast<psize_t>(buffer_) + idx);
    }


    void move_elements_up(size_t idx, size_t delta)
    {
        size_t layout_size = layout_size_/4;
        size_t layout_end  = layout_size - 2;

        size_t size = layout_end + 1 - idx;

        for (size_t jj = 0; jj < size; jj++)
        {
            size_t ii = layout_end - jj;

            AllocationBlock block = describe(ii);
            move_element_data(ii, block, delta, true);
        }

        for (size_t ii = idx; ii < layout_size ; ii++) {
            set_element_offset(ii) += delta;
        }
    }

    void move_elements_down(size_t idx, size_t delta)
    {
        size_t layout_size = layout_size_/4;

        for (size_t e_idx = idx; e_idx < layout_size - 1; e_idx++) {
            AllocationBlock block = describe(e_idx);
            move_element_data(e_idx, block, delta, false);
        }

        for (size_t e_idx = idx; e_idx < layout_size; e_idx++) {
            set_element_offset(e_idx) -= delta;
        }
    }

    void move_element_data(size_t idx, const AllocationBlock& block, size_t delta, bool up_down)
    {
        if (block.size() > 0)
        {
            uint8_t* ptr = block.ptr();
            uint8_t* new_addr = up_down ? (ptr + delta) : (ptr - delta);

            CopyByteBuffer(ptr, new_addr, block.size());

            if (is_allocatable(idx)) {
                PackedAllocatable* element = ptr_cast<PackedAllocatable>(new_addr);
                element->set_allocator_offset(this);
            }
        }
    }
};

template <typename T>
T* get(PackedAllocator* alloc, size_t idx)  {
    return alloc->template get<T>(idx);
}

template <typename T>
const T* get(const PackedAllocator* alloc, size_t idx)  {
    return alloc->template get<T>(idx);
}

template <typename T>
T* get(PackedAllocator& alloc, size_t idx)  {
    return alloc.template get<T>(idx);
}

template <typename T>
const T* get(const PackedAllocator& alloc, size_t idx)  {
    return alloc.template get<T>(idx);
}


template <typename T>
T* allocate(PackedAllocator* alloc, size_t idx)  {
    return alloc->template allocate<T>(idx);
}


template <typename T>
T* allocate(PackedAllocator& alloc, size_t idx)  {
    return alloc.template allocate<T>(idx);
}

template <typename T>
Span<T> span(PackedAllocator* alloc, size_t idx)  {
    size_t offset  = alloc->element_offset(idx);
    size_t size    = alloc->element_size(idx);

    return Span<T>(ptr_cast<T>(alloc->base() + offset), size / sizeof(T));
}

template <typename T>
Span<const T> span(const PackedAllocator* alloc, size_t idx)  {
    size_t offset  = alloc->element_offset(idx);
    size_t size    = alloc->element_size(idx);

    return Span<const T>(ptr_cast<const T>(alloc->base() + offset), size / sizeof(T));
}



template <typename... Types> struct SerializeTool;

template <typename Head, typename... Tail>
struct SerializeTool<Head, Tail...> {
    template <typename SerializationData>
    static void serialize(const PackedAllocator* allocator, SerializationData& buf, size_t idx = 0)
    {
        if (!allocator->is_empty(idx))
        {
            const Head* obj = allocator->template get<Head>(idx);
            obj->serialize(buf);
        }

        SerializeTool<Tail...>::serialize(allocator, buf, idx + 1);
    }
};

template <typename Head, typename... Tail>
struct SerializeTool<TypeList<Head, Tail...>> {
    template <typename SerializationData>
    static void serialize(const PackedAllocator* allocator, SerializationData& buf, size_t idx = 0)
    {
        if (!allocator->is_empty(idx))
        {
            const Head* obj = allocator->template get<Head>(idx);
            obj->serialize(buf);
        }

        SerializeTool<Tail...>::serialize(allocator, buf, idx + 1);
    }
};

template <>
struct SerializeTool<> {
    template <typename SerializationData>
    static void serialize(const PackedAllocator* allocator, SerializationData& buf, size_t idx = 0) {}
};

template <>
struct SerializeTool<TypeList<>> {
    template <typename SerializationData>
    static void serialize(const PackedAllocator* allocator, SerializationData& buf, size_t idx = 0) {}
};



template <typename... Types> struct DeserializeTool;

template <typename Head, typename... Tail>
struct DeserializeTool<Head, Tail...> {

    template <typename DeserializationData>
    static void deserialize(PackedAllocator* allocator, DeserializationData& buf, size_t idx = 0)
    {
        if (!allocator->is_empty(idx))
        {
            Head* obj = allocator->template get<Head>(idx);
            obj->deserialize(buf);
        }

        DeserializeTool<Tail...>::deserialize(allocator, buf, idx + 1);
    }
};

template <typename Head, typename... Tail>
struct DeserializeTool<TypeList<Head, Tail...>> {

    template <typename DeserializationData>
    static void deserialize(PackedAllocator* allocator, DeserializationData& buf, size_t idx = 0)
    {
        if (!allocator->is_empty(idx))
        {
            Head* obj = allocator->template get<Head>(idx);
            obj->deserialize(buf);
        }

        DeserializeTool<Tail...>::deserialize(allocator, buf, idx + 1);
    }
};

template <>
struct DeserializeTool<> {
    template <typename DeserializationData>
    static void deserialize(PackedAllocator* allocator, DeserializationData& buf, size_t idx = 0) {}
};

template <>
struct DeserializeTool<TypeList<>> {
    template <typename DeserializationData>
    static void deserialize(PackedAllocator* allocator, DeserializationData& buf, size_t idx = 0) {}
};



template <typename... Types> struct GenerateDataEventsTool;

template <typename Head, typename... Tail>
struct GenerateDataEventsTool<Head, Tail...> {
    static void generateDataEvents(const PackedAllocator* allocator, IBlockDataEventHandler* handler, size_t idx = 0)
    {
        if (!allocator->is_empty(idx))
        {
            Head* obj = allocator->template get<Head>(idx);
            obj->generateDataEvents(handler);
        }

        DeserializeTool<Tail...>::generateDataEvents(allocator, handler, idx + 1);
    }
};

template <typename Head, typename... Tail>
struct GenerateDataEventsTool<TypeList<Head, Tail...>> {
    static void generateDataEvents(const PackedAllocator* allocator, IBlockDataEventHandler* handler, size_t idx = 0)
    {
        if (!allocator->is_empty(idx))
        {
            Head* obj = allocator->template get<Head>(idx);
            obj->generateDataEvents(handler);
        }

        DeserializeTool<Tail...>::generateDataEvents(allocator, handler, idx + 1);
    }
};

template <>
struct GenerateDataEventsTool<> {
    static void generateDataEvents(PackedAllocator* allocator, IBlockDataEventHandler* handler, size_t idx = 0) {}
};

template <>
struct GenerateDataEventsTool<TypeList<>> {
    static void generateDataEvents(PackedAllocator* allocator, IBlockDataEventHandler* handler, size_t idx = 0) {}
};

template <typename PkdStructSO>
void dump(const PkdStructSO* ps, std::ostream& out = std::cout)
{
    TextBlockDumper dumper(out);
    ps->generateDataEvents(&dumper);
}

}
