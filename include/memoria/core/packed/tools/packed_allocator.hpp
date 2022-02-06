
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

    typedef PackedAllocatable                                                   Base;
    typedef PackedAllocator                                                     MyType;
    typedef PackedAllocator                                                     Allocator;

public:

    typedef uint64_t                                                             Bitmap;

    static const uint32_t VERSION                                                   = 1;

private:
    PackedAllocatable allocatable_;

    int32_t block_size_;
    int32_t layout_size_;
    int32_t bitmap_size_;

    uint8_t buffer_[1];

public:

    using FieldsList = MergeLists<
        typename Base::FieldsList,

        UInt32Value<VERSION>,
        decltype(block_size_),
        decltype(layout_size_),
        decltype(bitmap_size_)
    >;

    PackedAllocator() noexcept = default;

    PackedAllocatable& allocatable() {return allocatable_;}
    const PackedAllocatable& allocatable() const {return allocatable_;}


    bool is_allocatable(int32_t idx) const noexcept
    {
        const Bitmap* bmp = bitmap();
        return GetBit(bmp, idx);
    }

    Bitmap* bitmap() noexcept {
        return ptr_cast<Bitmap>(buffer_ + layout_size_);
    }

    const Bitmap* bitmap() const noexcept {
        return ptr_cast<const Bitmap>(buffer_ + layout_size_);
    }

    int32_t allocated() const noexcept {
        return element_offset(elements());
    }

    int32_t client_area() const noexcept {
        return block_size_ - my_size() - layout_size_ - bitmap_size_;
    }

    int32_t free_space() const noexcept {
        int32_t client_area = this->client_area();
        int32_t allocated = this->allocated();
        return client_area - allocated;
    }

    int32_t elements() const noexcept {
        return layout_size_/4 - 1;
    }

    uint8_t* base() noexcept {
        return buffer_ + layout_size_ + bitmap_size_;
    }

    const uint8_t* base() const noexcept {
        return buffer_ + layout_size_ + bitmap_size_;
    }

    int32_t layout_size() const noexcept {
        return layout_size_;
    }

    int32_t bitmap_size() const noexcept {
        return bitmap_size_;
    }

    int32_t block_size() const noexcept {
        return block_size_;
    }

    void set_block_size(int32_t block_size) noexcept {
        this->block_size_ = block_size;
    }

    VoidResult init(int32_t block_size, int32_t blocks) noexcept
    {
        block_size_ = PackedAllocatable::roundDownBytesToAlignmentBlocks(block_size);

        int32_t layout_blocks = blocks + (blocks % 2 ? 1 : 2);

        layout_size_ = layout_blocks * sizeof(int32_t);

        memset(buffer_, 0, layout_size_);

        bitmap_size_ = PackedAllocatable::roundUpBitsToAlignmentBlocks(layout_blocks);

        Bitmap* bitmap = this->bitmap();
        memset(bitmap, 0, bitmap_size_);

        return VoidResult::of();
    }

    static constexpr int32_t empty_size(int32_t blocks) noexcept
    {
        return block_size(0, blocks);
    }

    static constexpr int32_t block_size(int32_t client_area, int32_t blocks) noexcept
    {
        return PackedAllocatable::roundUpBytesToAlignmentBlocks(
                my_size() +
                (blocks + (blocks % 2 ? 1 : 2)) * sizeof(int32_t) +
                PackedAllocatable::roundUpBitsToAlignmentBlocks(blocks) +
                PackedAllocatable::roundUpBytesToAlignmentBlocks(client_area)
        );
    }

    static constexpr int32_t client_area(int32_t block_size, int32_t blocks) noexcept
    {
        return PackedAllocatable::roundDownBytesToAlignmentBlocks(
                block_size -
                (my_size() + (blocks + (blocks % 2 ? 1 : 2)) * sizeof(int32_t)
                 + PackedAllocatable::roundUpBitsToAlignmentBlocks(blocks))
        );
    }

    int32_t computeElementOffset(const void* element) const noexcept
    {
        const uint8_t* base_ptr = base();
        const uint8_t* elt_ptr = ptr_cast<const uint8_t>(element);

        ptrdiff_t diff = reinterpret_cast<ptrdiff_t>(elt_ptr - base_ptr);

        return diff;
    }

    Int32Result resizeBlock(const void* element, int32_t new_size) noexcept
    {
        MEMORIA_TRY(idx, findElement(element));
        return resizeBlock(idx, new_size);
    }

    Int32Result resizeBlock(int32_t idx, int32_t new_size) noexcept
    {
        MEMORIA_ASSERT_RTN(new_size, >=, 0);

        int32_t allocation_size = PackedAllocatable::roundUpBytesToAlignmentBlocks(new_size);

        int32_t size  = element_size(idx);
        int32_t delta = allocation_size - size;

        if (delta > 0)
        {
        	int32_t free_space = this->free_space();
            if (delta > free_space)
            {
                MEMORIA_TRY_VOID(enlarge(delta));
            }

            moveElements(idx + 1, delta);
        }
        else if (delta < 0)
        {
            moveElements(idx + 1, delta);

            if (allocatable_.allocator_offset() > 0)
            {
                MEMORIA_TRY_VOID(pack());
            }
        }

        return Int32Result::of(allocation_size);
    }

    BoolResult try_allocation(int32_t idx, int32_t new_size) noexcept
    {
        int32_t allocation_size = PackedAllocatable::roundUpBytesToAlignmentBlocks(new_size);

        int32_t size  = element_size(idx);
        int32_t delta = allocation_size - size;

        if (delta > 0)
        {
            int32_t free_space = this->free_space();
            if (delta > free_space)
            {
                if (allocatable_.allocator_offset() > 0)
                {
                    Allocator* alloc = allocatable_.allocator();
                    MEMORIA_TRY(my_idx, findElement(this));
                    return alloc->try_allocation(my_idx, delta);
                }
                else {
                    return false;
                }
            }
        }

        return true;
    }


    int32_t* layout() noexcept {
        return ptr_cast<int32_t>(buffer_);
    }

    const int32_t* layout() const noexcept {
        return ptr_cast<const int32_t>(buffer_);
    }

    const int32_t& element_offset(int32_t idx) const noexcept
    {
        return *(ptr_cast<const int32_t>(buffer_) + idx);
    }

    //TODO: rename to segment_size ?
    MMA_NODISCARD int32_t element_size(int32_t idx) const noexcept
    {
        int32_t size2 = element_offset(idx + 1);
        int32_t size1 = element_offset(idx);
        return size2 - size1;
    }

    Int32Result element_size(const void* element_ptr) const noexcept
    {
        MEMORIA_TRY(idx, findElement(element_ptr));
        return element_size(idx);
    }


    Int32Result findElement(const void* element_ptr) const noexcept
    {
        int32_t offset  = computeElementOffset(element_ptr);

        MEMORIA_ASSERT_RTN(offset, >=, 0);

        for (int32_t c = 0; c < layout_size_ / 4; c++)
        {
            if (offset < element_offset(c))
            {
                return Int32Result::of(c - 1);
            }
        }

        return MEMORIA_MAKE_GENERIC_ERROR("Requested element is not found in this allocator");
    }


    template <typename T>
    const T* get(int32_t idx) const noexcept
    {
        static_assert(IsPackedAlignedV<T>, "Invalid type alignment for packed allocator (8 bytes at most)");

        const T* addr = ptr_cast<const T>(base() + element_offset(idx));
        return addr;
    }

    template <typename T>
    T* get(int32_t idx) noexcept
    {
        static_assert(IsPackedAlignedV<T>, "Invalid type alignment for packed allocator (8 bytes at most)");
        T* addr = ptr_cast<T>(base() + element_offset(idx));
        return addr;
    }

    bool is_empty(int idx) const noexcept
    {
        return element_size(idx) == 0;
    }

    AllocationBlock describe(int32_t idx) noexcept
    {
        int32_t offset  = element_offset(idx);
        int32_t size    = element_size(idx);

        return AllocationBlock(size, offset, base() + offset);
    }

    AllocationBlockConst describe(int32_t idx) const noexcept
    {
        int32_t offset  = element_offset(idx);
        int32_t size    = element_size(idx);

        return AllocationBlockConst(size, offset, base() + offset);
    }

    template <typename T>
    Result<T*> allocate(int32_t idx, int32_t block_size) noexcept
    {
        static_assert(IsPackedStructV<T>, "May allocate only Standard Layout types having PackedAllocatable as header");
        static_assert(IsPackedAlignedV<T>, "Invalid type alignment for packed allocator (8 bytes at most)");

        using ResultT = Result<T*>;

        MEMORIA_TRY(block, allocate(idx, block_size, PackedBlockType::ALLOCATABLE));

        T* object = block.cast<T>();

        MEMORIA_TRY_VOID(object->init(block.size()));

        return ResultT::of(object);
    }

    template <typename T>
    Result<T*> allocateSpace(int32_t idx, int32_t block_size) noexcept
    {
        using ResultT = Result<T*>;
        static_assert(IsPackedAlignedV<T>, "Invalid type alignment for packed allocator (8 bytes at most)");
        static_assert(IsPackedStructV<T>, "May allocate only Standard Layout types having PackedAllocatable as header");

        MEMORIA_TRY(block, allocate(idx, block_size, PackedBlockType::ALLOCATABLE));

        return ResultT::of(block.cast<T>());
    }

    template <typename T>
    Result<T*> allocateEmpty(int32_t idx) noexcept
    {
        using ResultT = Result<T*>;

        static_assert(IsPackedStructV<T>, "May allocate only Standard Layout types having PackedAllocatable as header");
        static_assert(IsPackedAlignedV<T>, "Invalid type alignment for packed allocator (8 bytes at most)");

        int32_t block_size = T::empty_size();

        MEMORIA_TRY(block, allocate(idx, block_size, PackedBlockType::ALLOCATABLE));

        T* object = block.cast<T>();

        MEMORIA_TRY_VOID(object->init());

        return ResultT::of(object);
    }


    template <typename T>
    Result<T*> allocateDefault(int32_t idx) noexcept
    {
        using ResultT = Result<T*>;

        static_assert(IsPackedStructV<T>, "May allocate only Standard Layout types having PackedAllocatable as header");
        static_assert(IsPackedAlignedV<T>, "Invalid type alignment for packed allocator (8 bytes at most)");

        int32_t available_client_area = this->free_space();
        int32_t block_size = T::default_size(available_client_area);

        MEMORIA_TRY(block, allocate(idx, block_size, PackedBlockType::ALLOCATABLE));

        T* object = block.cast<T>();

        MEMORIA_TRY_VOID(object->init_default(block_size));

        return ResultT::of(object);
    }



    Result<PackedAllocator*> allocateAllocator(int32_t idx, int32_t streams) noexcept
    {
        using ResultT = Result<PackedAllocator*>;
        int32_t block_size = PackedAllocator::empty_size(streams);

        MEMORIA_TRY(block, allocate(idx, block_size, PackedBlockType::ALLOCATABLE));

        PackedAllocator* object = block.cast<PackedAllocator>();

        MEMORIA_TRY_VOID(object->init(block_size, streams));

        return ResultT::of(object);
    }


    template <typename T>
    Result<T*> allocate(int32_t idx) noexcept
    {
        using ResultT = Result<T*>;
        static_assert(IsPackedAlignedV<T>, "Invalid type alignment for packed allocator (8 bytes at most)");
        static_assert(!std::is_base_of<PackedAllocatable, T>::value,
                "Only classes that are not derived from PackedAllocatable "
                "should be instantiated this way");

        MEMORIA_TRY(block, allocate(idx, sizeof(T), PackedBlockType::RAW_MEMORY));
        return ResultT::of(block.cast<T>());
    }

    template <typename T>
    Result<T*> allocateArrayByLength(int32_t idx, int32_t length) noexcept
    {
        using ResultT = Result<T*>;
        static_assert(IsPackedAlignedV<T>, "Invalid type alignment for packed allocator (8 bytes at most)");
        static_assert(!std::is_base_of<PackedAllocatable, T>::value,
                "Only classes that are not derived from PackedAllocatable "
                "should be instantiated this way");

        MEMORIA_TRY(block, allocate(idx, length, PackedBlockType::RAW_MEMORY));
        return ResultT::of(block.cast<T>());
    }

    template <typename T>
    Result<T*> allocateArrayBySize(int32_t idx, int32_t size) noexcept
    {
        using ResultT = Result<T*>;
        static_assert(IsPackedAlignedV<T>, "Invalid type alignment for packed allocator (8 bytes at most)");
        static_assert(!std::is_base_of<PackedAllocatable, T>::value,
                "Only classes that are not derived from PackedAllocatable "
                "should be instantiated this way");

        MEMORIA_TRY(block, allocate(idx, sizeof(T) * size, PackedBlockType::RAW_MEMORY));
        return ResultT::of(block.cast<T>());
    }


    Result<AllocationBlock> allocate(int32_t idx, int32_t size, PackedBlockType type) noexcept
    {
        using ResultT = Result<AllocationBlock>;
        int32_t allocation_size = PackedAllocatable::roundUpBytesToAlignmentBlocks(size);

        int free_space_v = free_space();
        if (allocation_size > free_space_v)
        {
            MEMORIA_TRY_VOID(enlarge(allocation_size - free_space_v));
        }

        moveElements(idx + 1, allocation_size);

        setBlockType(idx, type);

        int32_t offset = element_offset(idx);

        memset(base() + offset, 0, allocation_size);

        if (type == PackedBlockType::ALLOCATABLE)
        {
            PackedAllocatable* alc = ptr_cast<PackedAllocatable>(base() + offset);
            alc->setAllocatorOffset(this);
        }

        auto offs = base() + offset;

        MEMORIA_V1_ASSERT_ALIGN_RTN(offs, 8);

        return ResultT::of(allocation_size, offset, base() + offset);
    }

    VoidResult importBlock(int32_t idx, const PackedAllocator* src, int32_t src_idx) noexcept
    {
        auto src_block  = src->describe(src_idx);
        auto type       = src->block_type(src_idx);

        MEMORIA_TRY_VOID(resizeBlock(idx, src_block.size()));

        setBlockType(idx, type);

        auto tgt_block  = this->describe(idx);

        const uint8_t* src_ptr    = src_block.ptr();
        uint8_t* tgt_ptr          = tgt_block.ptr();

        CopyByteBuffer(src_ptr, tgt_ptr, src_block.size());

        if (src_block.size() > 0 && type == PackedBlockType::ALLOCATABLE)
        {
            PackedAllocatable* element = ptr_cast<PackedAllocatable>(tgt_block.ptr());
            element->setAllocatorOffset(this);
        }

        return VoidResult::of();
    }

    VoidResult free(int32_t idx) noexcept
    {
        int32_t size        = element_size(idx);
        moveElements(idx + 1, -size);

        if (allocatable_.allocator_offset() > 0)
        {
            MEMORIA_TRY_VOID(pack());
        }

        return VoidResult::of();
    }

    void clear(int32_t idx) noexcept
    {
        auto block = describe(idx);
        memset(block.ptr(), 0, block.size());
    }

    void setBlockType(int32_t idx, PackedBlockType type) noexcept
    {
        Bitmap* bitmap = this->bitmap();
        SetBit(bitmap, idx, type == PackedBlockType::ALLOCATABLE);
    }

    PackedBlockType block_type(int32_t idx) const noexcept
    {
        const Bitmap* bitmap = this->bitmap();
        return GetBit(bitmap, idx) ? PackedBlockType::ALLOCATABLE : PackedBlockType::RAW_MEMORY;
    }

    void dump(std::ostream& out = std::cout) const noexcept
    {
        out << "PackedAllocator Layout:" << std::endl;
        out << "Block Size: " << block_size_ << std::endl;
        out << "Layout Size: " << layout_size_ << std::endl;
        out << "Bitmap Size: " << bitmap_size_ << std::endl;

        out << "Allocated Size: " << allocated() << std::endl;
        out << "ClientArea Size: " << client_area() << std::endl;
        out << "FreeSpace Size: " << free_space() << std::endl;

        dumpLayout(out);

        out << "PackedAllocator Block Types Bitmap:" << std::endl;
        const Bitmap* bitmap = this->bitmap();

        dumpSymbols<Bitmap>(out, layout_size_/4, 1, [bitmap](int32_t idx){
            return GetBit(bitmap, idx);
        });
    }

    void dumpAllocator(std::ostream& out = std::cout) const noexcept {
    	dump(out);
    }

    void dumpLayout(std::ostream& out = std::cout) const noexcept
    {
        dumpArray<int32_t>(out, layout_size_/4, [this](int32_t idx){
            return this->element_offset(idx);
        });
    }


    Int32Result enlarge(int32_t delta) noexcept
    {
        return resize(PackedAllocatable::roundUpBytesToAlignmentBlocks(block_size_ + delta));
    }

    Int32Result shrink(int32_t delta) noexcept
    {
        return resize(PackedAllocatable::roundUpBytesToAlignmentBlocks(block_size_ - delta));
    }

    VoidResult resizeBlock(int32_t new_size) noexcept
    {
        block_size_ = new_size;
        return VoidResult::of();
    }

    Int32Result resize(int32_t new_size) noexcept
    {
        if (allocatable_.allocator_offset() > 0)
        {
            Allocator* alloc = allocatable_.allocator();
            MEMORIA_TRY(block_size_tmp, alloc->resizeBlock(this, new_size));
            block_size_ = block_size_tmp;
        }
        else if (new_size <= block_size_)
        {
            if (new_size >= allocated() + my_size() + layout_size_ + bitmap_size_)
            {
                block_size_ = new_size;
            }
            else {
                return MEMORIA_MAKE_PACKED_OOM_ERROR();
            }
        }
        else {
            return MEMORIA_MAKE_PACKED_OOM_ERROR();
        }

        return Int32Result::of(block_size_);
    }

    void forceResize(int32_t amount) noexcept
    {
        block_size_ += PackedAllocatable::roundDownBytesToAlignmentBlocks(amount);
    }

    Int32Result pack() noexcept
    {
        int32_t free_space = this->free_space();
        return resize(block_size_ - free_space);
    }

    MMA_NODISCARD int32_t compute_free_space_up() const noexcept
    {
        int32_t space{};

        const PackedAllocator* alloc = this;

        while (alloc)
        {
            space += alloc->free_space();

            alloc = alloc->allocatable_.allocator_or_null();
        }

        return space;
    }


    VoidResult generateDataEvents(IBlockDataEventHandler* handler) const
    {
        handler->startGroup("ALLOCATOR");

        handler->value("PARENT_ALLOCATOR", &allocatable_.allocator_offset());

        handler->value("BLOCK_SIZE",    &block_size_);

        int32_t client_area = this->client_area();
        int32_t free_space  = this->free_space();

        handler->value("CLIENT_AREA",  &client_area);
        handler->value("FREE_SPACE",   &free_space);

        handler->value("LAYOUT_SIZE",   &layout_size_);
        handler->value("BITMAP_SIZE",   &bitmap_size_);

        handler->endGroup();

        return VoidResult::of();
    }


    template <typename SerializationData>
    VoidResult serialize(SerializationData& buf) const noexcept
    {
        FieldFactory<int32_t>::serialize(buf, allocatable_.allocator_offset_);
        FieldFactory<int32_t>::serialize(buf, block_size_);
        FieldFactory<int32_t>::serialize(buf, layout_size_);
        FieldFactory<int32_t>::serialize(buf, bitmap_size_);

        int32_t layout_size = layout_size_ / 4;

        FieldFactory<int32_t>::serialize(buf, layout(), layout_size);

        FieldFactory<Bitmap>::serialize(buf, bitmap(), bitmap_size_/sizeof(Bitmap));

        return VoidResult::of();
    }


    template <typename DeserializationData>
    VoidResult deserialize(DeserializationData& buf) noexcept
    {
        FieldFactory<int32_t>::deserialize(buf, allocatable_.allocator_offset_);
        FieldFactory<int32_t>::deserialize(buf, block_size_);
        FieldFactory<int32_t>::deserialize(buf, layout_size_);
        FieldFactory<int32_t>::deserialize(buf, bitmap_size_);

        int32_t layout_size = layout_size_ / 4;

        FieldFactory<int32_t>::deserialize(buf, layout(), layout_size);

        FieldFactory<Bitmap>::deserialize(buf, bitmap(), bitmap_size_/sizeof(Bitmap));

        return VoidResult::of();
    }

    template <typename T, typename SerializationData>
    VoidResult serializeSegment(SerializationData& buf, int32_t segment) const noexcept
    {
        auto data = this->describe(segment);
        FieldFactory<T>::serialize(buf, ptr_cast<const T>(data.ptr()), data.size() / (int32_t)sizeof(T));

        return VoidResult::of();
    }

    template <typename T, typename DeserializationData>
    VoidResult deserializeSegment(DeserializationData& buf, int32_t segment) noexcept
    {
        auto data = this->describe(segment);
        FieldFactory<T>::deserialize(buf, ptr_cast<T>(data.ptr()), data.size() / (int32_t)sizeof(T));

        return VoidResult::of();
    }


    constexpr static int32_t my_size() noexcept
    {
        return sizeof(MyType);
    }

private:
    int32_t& set_element_offset(int32_t idx) noexcept
    {
        return *(ptr_cast<int32_t>(buffer_) + idx);
    }


    void moveElementsUp(int32_t idx, int delta) noexcept
    {
        int32_t layout_size = layout_size_/4;
        for (int32_t ii = layout_size - 2; ii >= idx; ii--) {
            AllocationBlock block = describe(ii);
            moveElementData(ii, block, delta);
        }

        for (int32_t ii = idx; ii < layout_size ; ii++) {
            set_element_offset(ii) += delta;
        }
    }

    void moveElementsDown(int32_t idx, int delta) noexcept
    {
        int32_t layout_size = layout_size_/4;

        for (int32_t e_idx = idx; e_idx < layout_size - 1; e_idx++) {
            AllocationBlock block = describe(e_idx);
            moveElementData(e_idx, block, delta);
        }

        for (int32_t e_idx = idx; e_idx < layout_size; e_idx++) {
            set_element_offset(e_idx) += delta;
        }
    }

    void moveElementData(int32_t idx, const AllocationBlock& block, int32_t delta) noexcept
    {
        if (block.size() > 0)
        {
            uint8_t* ptr = block.ptr();

            CopyByteBuffer(ptr, ptr + delta, block.size());

            if (is_allocatable(idx))
            {
                PackedAllocatable* element = ptr_cast<PackedAllocatable>(ptr + delta);
                element->setAllocatorOffset(this);
            }
        }
    }


    void moveElements(int32_t start_idx, int32_t delta) noexcept
    {
        if (delta > 0) {
            moveElementsUp(start_idx, delta);
        }
        else {
            moveElementsDown(start_idx, delta);
        }
    }

};

template <typename T>
T* get(PackedAllocator* alloc, psize_t idx) noexcept {
    return alloc->template get<T>(idx);
}

template <typename T>
const T* get(const PackedAllocator* alloc, psize_t idx) noexcept {
    return alloc->template get<T>(idx);
}

template <typename T>
T* get(PackedAllocator& alloc, psize_t idx) noexcept {
    return alloc.template get<T>(idx);
}

template <typename T>
const T* get(const PackedAllocator& alloc, psize_t idx) noexcept {
    return alloc.template get<T>(idx);
}


template <typename T>
Result<T*> allocate(PackedAllocator* alloc, psize_t idx) noexcept {
    return alloc->template allocate<T>(idx);
}


template <typename T>
Result<T*> allocate(PackedAllocator& alloc, psize_t idx) noexcept {
    return alloc.template allocate<T>(idx);
}

template <typename T>
Span<T> span(PackedAllocator* alloc, psize_t idx) noexcept {
    int32_t offset  = alloc->element_offset(idx);
    int32_t size    = alloc->element_size(idx);

    return Span<T>(ptr_cast<T>(alloc->base() + offset), size / sizeof(T));
}

template <typename T>
Span<const T> span(const PackedAllocator* alloc, psize_t idx) noexcept {
    int32_t offset  = alloc->element_offset(idx);
    int32_t size    = alloc->element_size(idx);

    return Span<const T>(ptr_cast<const T>(alloc->base() + offset), size / sizeof(T));
}



template <typename... Types> struct SerializeTool;

template <typename Head, typename... Tail>
struct SerializeTool<Head, Tail...> {
    template <typename SerializationData>
    static void serialize(const PackedAllocator* allocator, SerializationData& buf, int32_t idx = 0)
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
    static void serialize(const PackedAllocator* allocator, SerializationData& buf, int32_t idx = 0)
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
    static void serialize(const PackedAllocator* allocator, SerializationData& buf, int32_t idx = 0) {}
};

template <>
struct SerializeTool<TypeList<>> {
    template <typename SerializationData>
    static void serialize(const PackedAllocator* allocator, SerializationData& buf, int32_t idx = 0) {}
};



template <typename... Types> struct DeserializeTool;

template <typename Head, typename... Tail>
struct DeserializeTool<Head, Tail...> {

    template <typename DeserializationData>
    static void deserialize(PackedAllocator* allocator, DeserializationData& buf, int32_t idx = 0)
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
    static void deserialize(PackedAllocator* allocator, DeserializationData& buf, int32_t idx = 0)
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
    static void deserialize(PackedAllocator* allocator, DeserializationData& buf, int32_t idx = 0) {}
};

template <>
struct DeserializeTool<TypeList<>> {
    template <typename DeserializationData>
    static void deserialize(PackedAllocator* allocator, DeserializationData& buf, int32_t idx = 0) {}
};



template <typename... Types> struct GenerateDataEventsTool;

template <typename Head, typename... Tail>
struct GenerateDataEventsTool<Head, Tail...> {
    static void generateDataEvents(const PackedAllocator* allocator, IBlockDataEventHandler* handler, int32_t idx = 0)
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
    static void generateDataEvents(const PackedAllocator* allocator, IBlockDataEventHandler* handler, int32_t idx = 0)
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
    static void generateDataEvents(PackedAllocator* allocator, IBlockDataEventHandler* handler, int32_t idx = 0) {}
};

template <>
struct GenerateDataEventsTool<TypeList<>> {
    static void generateDataEvents(PackedAllocator* allocator, IBlockDataEventHandler* handler, int32_t idx = 0) {}
};

template <typename PkdStructSO>
void dump(const PkdStructSO* ps, std::ostream& out = std::cout)
{
    TextBlockDumper dumper(out);
    ps->generateDataEvents(&dumper);
}


}
