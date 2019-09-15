
// Copyright 2013 Victor Smirnov
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
#include <memoria/v1/core/types/type2type.hpp>
#include <memoria/v1/core/memory/malloc.hpp>

#include <memoria/v1/core/packed/tools/packed_allocator_types.hpp>

#include <memoria/v1/core/tools/bitmap.hpp>
#include <memoria/v1/core/tools/dump.hpp>
#include <memoria/v1/core/tools/assert.hpp>

#include <type_traits>

namespace memoria {
namespace v1 {

enum class PackedBlockType {
    RAW_MEMORY = 0, ALLOCATABLE = 1
};


class PackedAllocator {

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

    PackedAllocator() = default;

    PackedAllocatable& allocatable() {return allocatable_;}
    const PackedAllocatable& allocatable() const {return allocatable_;}


    bool is_allocatable(int32_t idx) const
    {
        const Bitmap* bmp = bitmap();
        return GetBit(bmp, idx);
    }

    Bitmap* bitmap() {
        return T2T<Bitmap*>(buffer_ + layout_size_);
    }

    const Bitmap* bitmap() const {
        return T2T<const Bitmap*>(buffer_ + layout_size_);
    }

    int32_t allocated() const {
        return element_offset(elements());
    }

    int32_t client_area() const {
        return block_size_ - my_size() - layout_size_ - bitmap_size_;
    }

    int32_t free_space() const {
        int32_t client_area = this->client_area();
        int32_t allocated = this->allocated();
        return client_area - allocated;
    }

    int32_t elements() const {
        return layout_size_/4 - 1;
    }

    uint8_t* base() {
        return buffer_ + layout_size_ + bitmap_size_;
    }

    const uint8_t* base() const {
        return buffer_ + layout_size_ + bitmap_size_;
    }

    int32_t layout_size() const {
        return layout_size_;
    }

    int32_t bitmap_size() const {
        return bitmap_size_;
    }

    int32_t block_size() const {
        return block_size_;
    }

    void set_block_size(int32_t block_size) {
        this->block_size_ = block_size;
    }

    OpStatus init(int32_t block_size, int32_t blocks)
    {
        block_size_ = PackedAllocatable::roundDownBytesToAlignmentBlocks(block_size);

        int32_t layout_blocks = blocks + (blocks % 2 ? 1 : 2);

        layout_size_ = layout_blocks * sizeof(int32_t);

        memset(buffer_, 0, layout_size_);

        bitmap_size_ = PackedAllocatable::roundUpBitsToAlignmentBlocks(layout_blocks);

        Bitmap* bitmap = this->bitmap();
        memset(bitmap, 0, bitmap_size_);

        return OpStatus::OK;
    }

    static constexpr int32_t empty_size(int32_t blocks)
    {
        return block_size(0, blocks);
    }

    static constexpr int32_t block_size(int32_t client_area, int32_t blocks)
    {
        return PackedAllocatable::roundUpBytesToAlignmentBlocks(
                my_size() +
                (blocks + (blocks % 2 ? 1 : 2))*sizeof(int32_t) +
                PackedAllocatable::roundUpBitsToAlignmentBlocks(blocks) +
                PackedAllocatable::roundUpBytesToAlignmentBlocks(client_area)
        );
    }

    static constexpr int32_t client_area(int32_t block_size, int32_t blocks)
    {
        return PackedAllocatable::roundDownBytesToAlignmentBlocks(
                block_size -
                (my_size() + (blocks + (blocks % 2 ? 1 : 2))*sizeof(int32_t)
                 + PackedAllocatable::roundUpBitsToAlignmentBlocks(blocks))
        );
    }

    int32_t computeElementOffset(const void* element) const
    {
        const uint8_t* base_ptr = base();
        const uint8_t* elt_ptr = T2T<const uint8_t*>(element);

        size_t diff = T2T<size_t>(elt_ptr - base_ptr);

        return diff;
    }

    MMA1_NODISCARD int32_t resizeBlock(const void* element, int32_t new_size)
    {
        int32_t idx = findElement(element);

        return resizeBlock(idx, new_size);
    }

    MMA1_NODISCARD int32_t resizeBlock(int32_t idx, int32_t new_size)
    {
        MEMORIA_V1_ASSERT(new_size, >=, 0);

        int32_t allocation_size = PackedAllocatable::roundUpBytesToAlignmentBlocks(new_size);

        int32_t size        = element_size(idx);
        int32_t delta       = allocation_size - size;

        if (delta > 0)
        {
        	int32_t free_space = this->free_space();
            if (delta > free_space)
            {
                if (isFail(enlarge(delta)))
                {
                    return -1;
                }
            }

            moveElements(idx + 1, delta);
        }
        else if (delta < 0)
        {
            moveElements(idx + 1, delta);

            if (allocatable_.allocator_offset() > 0)
            {
                if (isFail(pack())) {
                    return -1;
                }
            }
        }


        return allocation_size;
    }


    int32_t* layout() {
        return T2T<int32_t*>(buffer_);
    }

    const int32_t* layout() const {
        return T2T<const int32_t*>(buffer_);
    }

    const int32_t& element_offset(int32_t idx) const
    {
        return *(T2T<const int32_t*>(buffer_) + idx);
    }

    //TODO: rename to segment_size ?
    int32_t element_size(int32_t idx) const
    {
        int32_t size2 = element_offset(idx + 1);
        int32_t size1 = element_offset(idx);
        return size2 - size1;
    }

    int32_t element_size(const void* element_ptr) const
    {
        int32_t idx = findElement(element_ptr);
        return element_size(idx);
    }


    int32_t findElement(const void* element_ptr) const
    {
        int32_t offset  = computeElementOffset(element_ptr);

        if (offset < 0) {
            int a = 0; a++;
        }

        MEMORIA_V1_ASSERT(offset, >=, 0);

        for (int32_t c = 0; c < layout_size_ / 4; c++)
        {
            if (offset < element_offset(c))
            {
                return c - 1;
            }
        }

        MMA1_THROW(Exception()) << WhatCInfo("Requested element is not found in this allocator");
    }


    template <typename T>
    const T* get(int32_t idx) const
    {
        const T* addr = T2T<const T*>(base() + element_offset(idx));
        return addr;
    }

    template <typename T>
    T* get(int32_t idx)
    {
        T* addr = T2T<T*>(base() + element_offset(idx));
        return addr;
    }

    bool is_empty(int idx) const
    {
        return element_size(idx) == 0;
    }

    AllocationBlock describe(int32_t idx)
    {
        int32_t offset  = element_offset(idx);
        int32_t size    = element_size(idx);

        return AllocationBlock(size, offset, base() + offset);
    }

    AllocationBlockConst describe(int32_t idx) const
    {
        int32_t offset  = element_offset(idx);
        int32_t size    = element_size(idx);

        return AllocationBlockConst(size, offset, base() + offset);
    }

    template <typename T>
    MMA1_NODISCARD T* allocate(int32_t idx, int32_t block_size)
    {
        static_assert(IsPackedStructV<T>, "May allocate only Standard Layout types having PackedAllocatable as header");

        AllocationBlock block = allocate(idx, block_size, PackedBlockType::ALLOCATABLE);

        T* object = block.cast<T>();

        if(isFail(object->init(block.size()))) {
            return nullptr;
        }

        return object;
    }

    template <typename T>
    MMA1_NODISCARD T* allocateSpace(int32_t idx, int32_t block_size)
    {
        static_assert(IsPackedStructV<T>, "May allocate only Standard Layout types having PackedAllocatable as header");

        AllocationBlock block = allocate(idx, block_size, PackedBlockType::ALLOCATABLE);

        return block.cast<T>();
    }

    template <typename T>
    MMA1_NODISCARD T* allocateEmpty(int32_t idx)
    {
        static_assert(IsPackedStructV<T>, "May allocate only Standard Layout types having PackedAllocatable as header");

        int32_t block_size = T::empty_size();

        AllocationBlock block = allocate(idx, block_size, PackedBlockType::ALLOCATABLE);

        if (!block) {
            return nullptr;
        }

        T* object = block.cast<T>();

        if(isFail(object->init())) {
            return nullptr;
        }

        return object;
    }


    MMA1_NODISCARD PackedAllocator* allocateAllocator(int32_t idx, int32_t streams)
    {
        int32_t block_size = PackedAllocator::empty_size(streams);

        AllocationBlock block = allocate(idx, block_size, PackedBlockType::ALLOCATABLE);

        if (!block) {
            return nullptr;
        }

        PackedAllocator* object = block.cast<PackedAllocator>();

        if(isFail(object->init(block_size, streams))) {
            return nullptr;
        }

        return object;
    }


    template <typename T>
    MMA1_NODISCARD T* allocate(int32_t idx)
    {
        static_assert(!std::is_base_of<PackedAllocatable, T>::value,
                "Only classes that are not derived from PackedAllocatable "
                "should be instantiated this way");

        AllocationBlock block = allocate(idx, sizeof(T), PackedBlockType::RAW_MEMORY);
        return block.cast<T>();
    }

    template <typename T>
    MMA1_NODISCARD T* allocateArrayByLength(int32_t idx, int32_t length)
    {
        static_assert(!std::is_base_of<PackedAllocatable, T>::value,
                "Only classes that are not derived from PackedAllocatable "
                "should be instantiated this way");

        AllocationBlock block = allocate(idx, length, PackedBlockType::RAW_MEMORY);
        return block.cast<T>();
    }

    template <typename T>
    MMA1_NODISCARD T* allocateArrayBySize(int32_t idx, int32_t size)
    {
        static_assert(!std::is_base_of<PackedAllocatable, T>::value,
                "Only classes that are not derived from PackedAllocatable "
                "should be instantiated this way");

        AllocationBlock block = allocate(idx, sizeof(T) * size, PackedBlockType::RAW_MEMORY);
        return block.cast<T>();
    }


    MMA1_NODISCARD AllocationBlock allocate(int32_t idx, int32_t size, PackedBlockType type)
    {
        int32_t allocation_size = PackedAllocatable::roundUpBytesToAlignmentBlocks(size);

        int free_space_v = free_space();
        if (allocation_size > free_space_v)
        {
            if (enlarge(allocation_size - free_space_v) < 0)
            {
                return AllocationBlock();
            }
        }

        moveElements(idx + 1, allocation_size);

        setBlockType(idx, type);

        int32_t offset = element_offset(idx);

        memset(base() + offset, 0, allocation_size);

        if (type == PackedBlockType::ALLOCATABLE)
        {
            PackedAllocatable* alc = T2T<PackedAllocatable*>(base() + offset);
            alc->setAllocatorOffset(this);
        }

        auto offs = base() + offset;

        MEMORIA_V1_ASSERT_ALIGN(offs, 8);

        return AllocationBlock(allocation_size, offset, base() + offset);
    }

    OpStatus importBlock(int32_t idx, const PackedAllocator* src, int32_t src_idx)
    {
        auto src_block  = src->describe(src_idx);
        auto type       = src->block_type(src_idx);

        if (resizeBlock(idx, src_block.size()) < 0) {
            return OpStatus::FAIL;
        }

        setBlockType(idx, type);

        auto tgt_block  = this->describe(idx);

        const uint8_t* src_ptr    = src_block.ptr();
        uint8_t* tgt_ptr          = tgt_block.ptr();

        CopyByteBuffer(src_ptr, tgt_ptr, src_block.size());

        if (src_block.size() > 0 && type == PackedBlockType::ALLOCATABLE)
        {
            PackedAllocatable* element = T2T<PackedAllocatable*>(tgt_block.ptr());
            element->setAllocatorOffset(this);
        }

        return OpStatus::OK;
    }

    void free(int32_t idx)
    {
        int32_t size        = element_size(idx);
        moveElements(idx + 1, -size);

        if (allocatable_.allocator_offset() > 0)
        {
            if(isFail(pack()))
            {
                MMA1_THROW(RuntimeException()) << WhatCInfo("PackedAllocator::pack() failed");
            }
        }
    }

    void clear(int32_t idx)
    {
        auto block = describe(idx);
        memset(block.ptr(), 0, block.size());
    }

    void setBlockType(int32_t idx, PackedBlockType type)
    {
        Bitmap* bitmap = this->bitmap();
        SetBit(bitmap, idx, type == PackedBlockType::ALLOCATABLE);
    }

    PackedBlockType block_type(int32_t idx) const
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

        dumpLayout(out);

        out << "PackedAllocator Block Types Bitmap:" << std::endl;
        const Bitmap* bitmap = this->bitmap();

        dumpSymbols<Bitmap>(out, layout_size_/4, 1, [bitmap](int32_t idx){
            return GetBit(bitmap, idx);
        });
    }

    void dumpAllocator(std::ostream& out = std::cout) const {
    	dump(out);
    }

    void dumpLayout(std::ostream& out = std::cout) const
    {
        dumpArray<int32_t>(out, layout_size_/4, [this](int32_t idx){
            return this->element_offset(idx);
        });
    }


    MMA1_NODISCARD int32_t enlarge(int32_t delta)
    {
        return resize(PackedAllocatable::roundUpBytesToAlignmentBlocks(block_size_ + delta));
    }

    MMA1_NODISCARD int32_t shrink(int32_t delta)
    {
        return resize(PackedAllocatable::roundUpBytesToAlignmentBlocks(block_size_ - delta));
    }

    void resizeBlock(int32_t new_size)
    {
        block_size_ = new_size;
    }

    MMA1_NODISCARD int32_t resize(int32_t new_size)
    {
        if (allocatable_.allocator_offset() > 0)
        {
            Allocator* alloc = allocatable_.allocator();
            block_size_ = alloc->resizeBlock(this, new_size);
        }
        else if (new_size <= block_size_)
        {
            if (new_size >= allocated() + my_size() + layout_size_ + bitmap_size_)
            {
                block_size_ = new_size;
            }
            else {
                return -1;
            }
        }
        else {
            return -1;
        }

        return block_size_;
    }

    void forceResize(int32_t amount)
    {
        block_size_ += PackedAllocatable::roundDownBytesToAlignmentBlocks(amount);
    }

    MMA1_NODISCARD int32_t pack()
    {
        int32_t free_space = this->free_space();
        return resize(block_size_ - free_space);
    }

    MMA1_NODISCARD int32_t compute_free_space_up() const
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


    void generateDataEvents(IBlockDataEventHandler* handler) const
    {
        handler->startGroup("ALLOCATOR");

        handler->value("PARENT_ALLOCATOR", &allocatable_.allocator_offset());

        handler->value("BLOCK_SIZE",    &block_size_);

        int32_t client_area = this->client_area();
        int32_t free_space  = this->free_space();

        handler->value("CLIENT_AREA",   &client_area);
        handler->value("FREE_SPACE",   &free_space);

        handler->value("LAYOUT_SIZE",   &layout_size_);
        handler->value("BITMAP_SIZE",   &bitmap_size_);

        handler->endGroup();
    }


    template <typename SerializationData>
    void serialize(SerializationData& buf) const
    {
        FieldFactory<int32_t>::serialize(buf, allocatable_.allocator_offset_);
        FieldFactory<int32_t>::serialize(buf, block_size_);
        FieldFactory<int32_t>::serialize(buf, layout_size_);
        FieldFactory<int32_t>::serialize(buf, bitmap_size_);

        int32_t layout_size = layout_size_ / 4;

        FieldFactory<int32_t>::serialize(buf, layout(), layout_size);

        FieldFactory<Bitmap>::serialize(buf, bitmap(), bitmap_size_/sizeof(Bitmap));
    }


    template <typename DeserializationData>
    void deserialize(DeserializationData& buf)
    {
        FieldFactory<int32_t>::deserialize(buf, allocatable_.allocator_offset_);
        FieldFactory<int32_t>::deserialize(buf, block_size_);
        FieldFactory<int32_t>::deserialize(buf, layout_size_);
        FieldFactory<int32_t>::deserialize(buf, bitmap_size_);

        int32_t layout_size = layout_size_ / 4;

        FieldFactory<int32_t>::deserialize(buf, layout(), layout_size);

        FieldFactory<Bitmap>::deserialize(buf, bitmap(), bitmap_size_/sizeof(Bitmap));
    }

    template <typename T, typename SerializationData>
    void serializeSegment(SerializationData& buf, int32_t segment) const
    {
        auto data = this->describe(segment);
        FieldFactory<T>::serialize(buf, T2T<const T*>(data.ptr()), data.size() / (int32_t)sizeof(T));
    }

    template <typename T, typename DeserializationData>
    void deserializeSegment(DeserializationData& buf, int32_t segment)
    {
        auto data = this->describe(segment);
        FieldFactory<T>::deserialize(buf, T2T<T*>(data.ptr()), data.size() / (int32_t)sizeof(T));
    }


    constexpr static int32_t my_size()
    {
        return sizeof(MyType) - alignof(MyType);
    }


private:
    int32_t& set_element_offset(int32_t idx)
    {
        return *(T2T<int32_t*>(buffer_) + idx);
    }


    void moveElementsUp(int32_t idx, int delta)
    {
        int32_t layout_size = layout_size_/4;

        if (idx < layout_size - 1)
        {
            AllocationBlock block = describe(idx);

            moveElementsUp(idx + 1, delta);

            moveElementData(idx, block, delta);
        }

        set_element_offset(idx) += delta;
    }

    void moveElementsDown(int32_t idx, int delta)
    {
        int32_t layout_size = layout_size_/4;

        if (idx < layout_size - 1)
        {
            AllocationBlock block = describe(idx);

            moveElementData(idx, block, delta);

            set_element_offset(idx) += delta;

            moveElementsDown(idx + 1, delta);
        }
        else {
            set_element_offset(idx) += delta;
        }
    }

    void moveElementData(int32_t idx, const AllocationBlock& block, int32_t delta)
    {
        if (block.size() > 0)
        {
            uint8_t* ptr = block.ptr();

            CopyByteBuffer(ptr, ptr + delta, block.size());

            if (is_allocatable(idx))
            {
                PackedAllocatable* element = T2T<PackedAllocatable*>(ptr + delta);
                element->setAllocatorOffset(this);
            }
        }
    }


    void moveElements(int32_t start_idx, int32_t delta)
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
T* get(PackedAllocator* alloc, psize_t idx) {
    return alloc->template get<T>(idx);
}

template <typename T>
const T* get(const PackedAllocator* alloc, psize_t idx) {
    return alloc->template get<T>(idx);
}

template <typename T>
T* get(PackedAllocator& alloc, psize_t idx) {
    return alloc.template get<T>(idx);
}

template <typename T>
const T* get(const PackedAllocator& alloc, psize_t idx) {
    return alloc.template get<T>(idx);
}


template <typename T>
T* allocate(PackedAllocator* alloc, psize_t idx) {
    return alloc->template allocate<T>(idx);
}


template <typename T>
T* allocate(PackedAllocator& alloc, psize_t idx) {
    return alloc.template allocate<T>(idx);
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


}}
