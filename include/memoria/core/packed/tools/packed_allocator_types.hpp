
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

#include <memoria/core/types.hpp>
#include <memoria/core/exceptions/exceptions.hpp>
#include <memoria/core/tools/reflection.hpp>
#include <memoria/profiles/common/block_operations.hpp>

#include <memoria/core/tools/bitmap.hpp>

#include <memoria/core/packed/tools/packed_tools.hpp>

#include <memoria/core/memory/ptr_cast.hpp>

namespace memoria {

template <typename PkdStruct> struct PackedStructTraits;

template <typename PkdStruct>
using AccumType = typename PackedStructTraits<PkdStruct>::AccumType;

enum class PkdSearchType {SUM, MAX};

template <typename PkdStruct>
constexpr PkdSearchType PkdKeySearchType = PackedStructTraits<PkdStruct>::KeySearchType;

template <typename PkdStruct>
using PkdSearchKeyType = typename PackedStructTraits<PkdStruct>::SearchKeyType;

template <typename PkdStruct>
using PkdSearchKeyDataType = typename PackedStructTraits<PkdStruct>::SearchKeyDataType;

template <typename PkdStruct>
using PkdSearchKeyDataType = typename PackedStructTraits<PkdStruct>::SearchKeyDataType;

template <typename PkdStruct>
constexpr PackedDataTypeSize PkdStructSizeType = PackedStructTraits<PkdStruct>::DataTypeSize;

template <typename PkdStruct>
constexpr int32_t PkdStructIndexes = PackedStructTraits<PkdStruct>::Indexes;


template <typename List> struct PackedListStructSizeType;

template <typename Head, typename... Tail>
struct PackedListStructSizeType<TL<Head, Tail...>> {
    static const PackedDataTypeSize HeadValue = PkdStructSizeType<Head>;

    static const PackedDataTypeSize Value =
            (HeadValue == PackedDataTypeSize::VARIABLE) ?
                    PackedDataTypeSize::VARIABLE :
                    PackedListStructSizeType<TL<Tail...>>::Value;
};

template <>
struct PackedListStructSizeType<TL<>> {
    static const PackedDataTypeSize Value = PackedDataTypeSize::FIXED;
};


template <PackedDataTypeSize... SizeTypes> struct PackedSizeTypeList;

template <PackedDataTypeSize Head, PackedDataTypeSize... Tail>
struct PackedSizeTypeList<Head, Tail...>
{
    static const PackedDataTypeSize Value =
            (Head == PackedDataTypeSize::VARIABLE) ?
                PackedDataTypeSize::VARIABLE :
                PackedSizeTypeList<Tail...>::Value;
};

template <>
struct PackedSizeTypeList<> {
    static const PackedDataTypeSize Value = PackedDataTypeSize::FIXED;
};


template <typename MyType, typename Base> class PackedAllocatorBase;

class PackedAllocator;

class PackedAllocatable {
protected:
    int32_t allocator_offset_;

    int32_t& allocator_offset() {return allocator_offset_;}

public:

    template <typename MyType, typename Base>
    friend class PackedAllocatorBase;

    friend class PackedAllocator;


    static constexpr uint32_t VERSION                   = 1;
    static constexpr int32_t AlignmentBlock             = PackedAllocationAlignment;


    using FieldsList = TypeList<
            ConstValue<uint32_t, VERSION>,
            decltype(allocator_offset_)
    >;

    PackedAllocatable() noexcept = default;

    const int32_t& allocator_offset() const {return allocator_offset_;}

    void setTopLevelAllocator() noexcept
    {
        allocator_offset() = 0;
    }


    bool has_allocator() const noexcept
    {
        return allocator_offset_ > 0;
    }

    void setAllocatorOffset(const void* allocator) noexcept
    {
        // TODO: check for UB.
        const char* my_ptr = ptr_cast<const char>(this);
        const char* alc_ptr = ptr_cast<const char>(allocator);
        ptrdiff_t diff = reinterpret_cast<ptrdiff_t>(my_ptr - alc_ptr);
        allocator_offset() = diff;
    }

    PackedAllocator* allocator() noexcept
    {
        if (allocator_offset() > 0)
        {
            uint8_t* my_ptr = ptr_cast<uint8_t>(this);
            return ptr_cast<PackedAllocator>(my_ptr - allocator_offset());
        }
        else {
            // FIXME: return an error
            terminate("No allocation is defined for this object");
        }
    }

    PackedAllocator* allocator_or_null() noexcept
    {
        if (allocator_offset() > 0)
        {
            uint8_t* my_ptr = ptr_cast<uint8_t>(this);
            return ptr_cast<PackedAllocator>(my_ptr - allocator_offset());
        }
        else {
            return nullptr;
        }
    }

    const PackedAllocator* allocator() const noexcept
    {
        if (allocator_offset() > 0)
        {
            const uint8_t* my_ptr = ptr_cast<const uint8_t>(this);
            return ptr_cast<const PackedAllocator>(my_ptr - allocator_offset());
        }
        else {
            // FIXME: return an error here
            terminate("No allocation is defined for this object");
        }
    }

    const PackedAllocator* allocator_or_null() const noexcept
    {
        if (allocator_offset() > 0)
        {
            const uint8_t* my_ptr = ptr_cast<const uint8_t>(this);
            return ptr_cast<const PackedAllocator>(my_ptr - allocator_offset());
        }
        else {
            return nullptr;
        }
    }

    static constexpr int32_t roundUpBytesToAlignmentBlocks(int32_t value) noexcept
    {
        return (value / AlignmentBlock + (value % AlignmentBlock ? 1 : 0)) * AlignmentBlock;
    }

    static constexpr int32_t roundDownBytesToAlignmentBlocks(int32_t value) noexcept
    {
        return (value / AlignmentBlock) * AlignmentBlock;
    }

    static constexpr int32_t roundUpBitsToAlignmentBlocks(int32_t bits) noexcept
    {
        return roundUpBytesToAlignmentBlocks(roundUpBitToBytes(bits));
    }

    static constexpr int32_t roundDownBitsToAlignmentBlocks(int32_t bits) noexcept
    {
        return roundDownBytesToAlignmentBlocks(roundDownBitsToBytes(bits));
    }

    static constexpr int32_t roundUpBitToBytes(int32_t bits) noexcept
    {
        return bits / 8 + (bits % 8 > 0);
    }

    static constexpr int32_t roundDownBitsToBytes(int32_t bits) noexcept
    {
        return bits / 8 + (bits % 8 > 0);
    }

    static constexpr int32_t divUp(int32_t value, int32_t divider) noexcept {
        return ::memoria::divUp(value, divider);
    }

    template <typename SerializationData>
    void serialize(SerializationData& buf) const
    {
        FieldFactory<int32_t>::serialize(buf, allocator_offset_);
    }

    template <typename DeserializationData>
    void deserialize(DeserializationData& buf)
    {
        FieldFactory<int32_t>::deserialize(buf, allocator_offset_);
    }
};



struct AllocationBlock {
    int32_t size_;
    int32_t offset_;
    uint8_t* ptr_;

    constexpr AllocationBlock(int32_t size, int32_t offset, uint8_t* ptr) noexcept:
        size_(size), offset_(offset), ptr_(ptr)
    {}

    constexpr AllocationBlock() noexcept:
        size_{}, offset_{}, ptr_{nullptr}
    {}

    int32_t size() const noexcept   {return size_;}
    int32_t offset() const noexcept {return offset_;}
    uint8_t* ptr() const noexcept   {return ptr_;}
    bool is_empty() const noexcept  {return size_ == 0;}

    template <typename T>
    const T* cast() const noexcept {
        return ptr_cast<const T>(ptr_);
    }

    template <typename T>
    T* cast() noexcept {
        return ptr_cast<T>(ptr_);
    }

    operator bool() const noexcept {
        return ptr_ != nullptr;
    }
};


struct AllocationBlockConst {
    int32_t size_;
    int32_t offset_;
    const uint8_t* ptr_;

    constexpr AllocationBlockConst(int32_t size, int32_t offset, const uint8_t* ptr) noexcept:
        size_(size), offset_(offset), ptr_(ptr)
    {}

    int32_t size() const noexcept   {return size_;}
    int32_t offset() const noexcept {return offset_;}
    const uint8_t* ptr() const noexcept {return ptr_;}

    operator bool() const noexcept {return true;}

    template <typename T>
    const T* cast() const noexcept {
        return ptr_cast<const T>(ptr_);
    }
};


}
