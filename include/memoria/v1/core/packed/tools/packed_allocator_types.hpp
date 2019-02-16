
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
#include <memoria/v1/core/exceptions/exceptions.hpp>
#include <memoria/v1/core/tools/reflection.hpp>
#include <memoria/v1/profiles/common/block_operations.hpp>

namespace memoria {
namespace v1 {

template <typename PkdStruct>
struct AccumType: HasType<typename PkdStruct::Value> {};

template <typename PkdStruct>
struct PkdStructInputType: HasType<typename PkdStruct::InputType> {};

enum class PkdSearchType {SUM, MAX};

template <typename PkdStruct>
struct PkdSearchTypeProvider {
    static constexpr PkdSearchType Value = PkdStruct::KeySearchType;
};

template <typename PkdStruct>
struct PkdSearchKeyTypeProvider {
    using Type = typename PkdStruct::IndexValue;
};

template <typename T> struct StructSizeProvider;


template <typename PkdStruct>
struct PkdStructSizeType {
    static const PackedSizeType Value = PkdStruct::SizeType;
};

template <typename PkdStruct>
struct PkdStructInputBufferType {
    using Type = typename PkdStruct::InputBuffer;
};




template <typename List> struct PackedListStructSizeType;

template <typename Head, typename... Tail>
struct PackedListStructSizeType<TL<Head, Tail...>> {
    static const PackedSizeType HeadValue = PkdStructSizeType<Head>::Value;

    static const PackedSizeType Value =
            (HeadValue == PackedSizeType::VARIABLE) ?
                    PackedSizeType::VARIABLE :
                    PackedListStructSizeType<TL<Tail...>>::Value;
};

template <>
struct PackedListStructSizeType<TL<>> {
    static const PackedSizeType Value = PackedSizeType::FIXED;
};


template <PackedSizeType... SizeTypes> struct PackedSizeTypeList;

template <PackedSizeType Head, PackedSizeType... Tail>
struct PackedSizeTypeList<Head, Tail...> {
    static const PackedSizeType Value = (Head == PackedSizeType::VARIABLE) ? PackedSizeType::VARIABLE : PackedSizeTypeList<Tail...>::Value;
};

template <>
struct PackedSizeTypeList<> {
    static const PackedSizeType Value = PackedSizeType::FIXED;
};

class PackedOOMException: public MemoriaThrowableLW {
    const char* msg_;
public:

    PackedOOMException(const char* source, const char* msg):
        MemoriaThrowableLW(source ),
        msg_(msg)
    {}

    PackedOOMException(const char* source): MemoriaThrowableLW(source), msg_("Packed Structure Memory Allocation Failed") {}

    virtual void dump(std::ostream& out) const noexcept
    {
        if (msg_ != nullptr) {
            out << "PackedOOMException at " << source_ << ": " << msg_;
        }
        else {
            out << "PackedOOMException at " << source_;
        }
    }
};



static inline void OOM_THROW_IF_FAILED(OpStatus status, const char* source) {
    if (isFail(status)) {
        throw PackedOOMException(source);
    }
}

static inline OpStatus toOpStatus(int32_t res)
{
    return res >= 0 ? OpStatus::OK : OpStatus::FAIL;
}

template <typename T>
static inline void OOM_THROW_IF_FAILED(const OpStatusT<T>& status, const char* source) {
    if (isFail(status)) {
        throw PackedOOMException(source);
    }
}

template <typename T>
static inline void OOM_THROW_IF_FAILED(const T* ptr, const char* source) {
    if (!ptr) {
        throw PackedOOMException(source);
    }
}

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


    static const uint32_t VERSION                   = 1;
    static const int32_t AlignmentBlock             = PackedAllocationAlignment;


    typedef TypeList<
            ConstValue<uint32_t, VERSION>,
            decltype(allocator_offset_)
    >                                           FieldsList;

    PackedAllocatable() = default;

    const int32_t& allocator_offset() const {return allocator_offset_;}

    void setTopLevelAllocator()
    {
        allocator_offset() = 0;
    }


    bool has_allocator() const
    {
        return allocator_offset_ > 0;
    }

    void setAllocatorOffset(const void* allocator)
    {
        const char* my_ptr = T2T<const char*>(this);
        const char* alc_ptr = T2T<const char*>(allocator);
        size_t diff = T2T<size_t>(my_ptr - alc_ptr);
        allocator_offset() = diff;
    }

    PackedAllocator* allocator()
    {
        if (allocator_offset() > 0)
        {
            uint8_t* my_ptr = T2T<uint8_t*>(this);
            return T2T<PackedAllocator*>(my_ptr - allocator_offset());
        }
        else {
            MMA1_THROW(RuntimeException()) << WhatCInfo("No allocation is defined for this object");
        }
    }

    const PackedAllocator* allocator() const
    {
        if (allocator_offset() > 0)
        {
            const uint8_t* my_ptr = T2T<const uint8_t*>(this);
            return T2T<const PackedAllocator*>(my_ptr - allocator_offset());
        }
        else {
            MMA1_THROW(RuntimeException()) << WhatCInfo("No allocation is defined for this object");
        }
    }

    static constexpr int32_t roundUpBytesToAlignmentBlocks(int32_t value)
    {
        return (value / AlignmentBlock + (value % AlignmentBlock ? 1 : 0)) * AlignmentBlock;
    }

    static constexpr int32_t roundDownBytesToAlignmentBlocks(int32_t value)
    {
        return (value / AlignmentBlock) * AlignmentBlock;
    }

    static constexpr int32_t roundUpBitsToAlignmentBlocks(int32_t bits)
    {
        return roundUpBytesToAlignmentBlocks(roundUpBitToBytes(bits));
    }

    static constexpr int32_t roundDownBitsToAlignmentBlocks(int32_t bits)
    {
        return roundDownBytesToAlignmentBlocks(roundDownBitsToBytes(bits));
    }

    static constexpr int32_t roundUpBitToBytes(int32_t bits)
    {
        return bits / 8 + (bits % 8 > 0);
    }

    static constexpr int32_t roundDownBitsToBytes(int32_t bits)
    {
        return bits / 8 + (bits % 8 > 0);
    }

    static constexpr int32_t divUp(int32_t value, int32_t divider) {
        return (value / divider) + (value % divider ? 1 : 0);
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

    AllocationBlock(int32_t size, int32_t offset, uint8_t* ptr): size_(size), offset_(offset), ptr_(ptr) {}
    AllocationBlock(): size_{}, offset_{}, ptr_{nullptr} {}

    int32_t size() const    {return size_;}
    int32_t offset() const  {return offset_;}
    uint8_t* ptr() const  {return ptr_;}
    bool is_empty() const {return size_ == 0;}

    template <typename T>
    const T* cast() const {
        return T2T<const T*>(ptr_);
    }

    template <typename T>
    T* cast() {
        return T2T<T*>(ptr_);
    }

    operator bool() const {
        return ptr_ != nullptr;
    }
};


struct AllocationBlockConst {
    int32_t size_;
    int32_t offset_;
    const uint8_t* ptr_;

    AllocationBlockConst(int32_t size, int32_t offset, const uint8_t* ptr): size_(size), offset_(offset), ptr_(ptr) {}

    int32_t size() const    {return size_;}
    int32_t offset() const  {return offset_;}
    const uint8_t* ptr() const    {return ptr_;}

    operator bool() const {return true;}

    template <typename T>
    const T* cast() const {
        return T2T<const T*>(ptr_);
    }
};


template <typename T>
constexpr bool IsPackedStructV = std::is_standard_layout<T>::value && std::is_trivially_copyable<T>::value;




// ====================== PackedStruct Traits ====================


template <typename PkdStruct> struct PackedStructTraits;



}}
