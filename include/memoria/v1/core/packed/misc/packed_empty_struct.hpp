
// Copyright 2016 Victor Smirnov
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

#include <memoria/v1/core/packed/tools/packed_allocator_types.hpp>
#include <memoria/v1/core/packed/buffer/packed_fse_input_buffer_ro.hpp>
#include <memoria/v1/core/tools/accessors.hpp>

#include <limits>

namespace memoria {
namespace v1 {


template <typename Value_ = BigInt, PkdSearchType SearchType_ = PkdSearchType::SUM>
class PackedEmptyStruct: public PackedAllocatable {

    typedef PackedAllocatable                                                   Base;

public:
    static const UInt VERSION = 1;
    static constexpr Int Indexes = 0;
    static constexpr PkdSearchType KeySearchType = SearchType_;


    using MyType = PackedEmptyStruct<Value_, SearchType_>;


    using Value = typename std::remove_reference<Value_>::type;

    static constexpr Int Blocks = Indexes;

    using InputBuffer = MyType;

    using Values = core::StaticVector<Value, Blocks>;

    using InputType = Values;
    using IndexValue = Value;
    using SizesT = core::StaticVector<Int, Blocks>;

private:

public:
    PackedEmptyStruct() = default;

    Int size() const {return 0;}

    Int block_size() const
    {
        return sizeof(MyType);
    }

    Int block_size(const MyType* other) const
    {
        return block_size();
    }

    static constexpr Int block_size(Int array_size)
    {
        return sizeof(MyType);
    }

    static constexpr Int packed_block_size(Int array_size)
    {
        return sizeof(MyType);
    }

    static Int elements_for(Int block_size)
    {
        size_t bsize = block_size;

        return bsize >= sizeof(MyType) ? std::numeric_limits<Int>::max() : 0;
    }


    void init(Int block_size){}

    void init(const SizesT& capacities){}

    static constexpr Int empty_size()
    {
        return sizeof(MyType);
    }


    void init()
    {

    }

    template <typename T>
    void max(T& accum) const
    {
    }

    template <typename T>
    void setValues(Int idx, T&&) {}

    template <typename T>
    void insert(Int idx, T&&) {}

    template <Int Offset, typename T>
    void _insert(Int idx, T&&) {}


    template <Int Offset, Int Size, typename T, template <typename, Int> class BranchNodeEntryItem>
    void max(BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset <= Size - Indexes, "Invalid balanced tree structure");
    }


    template <Int Offset, Int Size, typename T, template <typename, Int> class BranchNodeEntryItem>
    void sum(BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset <= Size - Indexes, "Invalid balanced tree structure");
    }

    template <Int Offset, Int Size, typename T, template <typename, Int> class BranchNodeEntryItem>
    void sum(Int start, Int end, BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset <= Size - Indexes, "Invalid balanced tree structure");
    }

    template <Int Offset, Int Size, typename T, template <typename, Int> class BranchNodeEntryItem>
    void sub(Int start, BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset <= Size - Indexes, "Invalid balanced tree structure");
    }

    template <Int Offset, Int Size, typename T, template <typename, Int> class BranchNodeEntryItem>
    void sum(Int idx, BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset <= Size - Indexes, "Invalid balanced tree structure");
    }

    template <typename T>
    Int append(Int size, T&&)
    {
        return size;
    }

    // =================================== Update ========================================== //

    void reindex() {}
    void check() const {}

    void remove(Int start, Int end)
    {

    }

    void removeSpace(Int room_start, Int room_end) {
        remove(room_start, room_end);
    }



    void reset()
    {

    }


    void splitTo(MyType* other, Int idx)
    {}

    void mergeWith(MyType* other)
    {}

    // ===================================== IO ============================================ //

    void insert(Int pos, Value val)
    {}

    void insert(Int block, Int pos, Value val)
    {}

    void insert(Int pos, Int start, Int size, const InputBuffer* buffer)
    {}

    template <typename Adaptor>
    void insert(Int pos, Int size, Adaptor&& adaptor)
    {}


    SizesT insert_buffer(SizesT at, const InputBuffer* buffer, SizesT starts, SizesT ends, Int size)
    {
        return at + SizesT(size);
    }

    Int insert_buffer(Int at, const InputBuffer* buffer, Int start, Int size)
    {
        return at + size;
    }

    Value value(Int, Int) const {
        return Value();
    }

    SizesT positions(Int idx) const {
        return SizesT(idx);
    }


    template <typename Adaptor>
    void _insert(Int pos, Int size, Adaptor&& adaptor)
    {
    }


    template <Int Offset, typename Value, typename T, Int Size, template <typename, Int> class BranchNodeEntryItem>
    void _update(Int pos, Value&& val, BranchNodeEntryItem<T, Size>& accum)
    {}

    template <Int Offset, typename Value, typename T, Int Size, template <typename, Int> class BranchNodeEntryItem>
    void _insert(Int pos, Value&& val, BranchNodeEntryItem<T, Size>& accum)
    {
    }

    template <Int Offset, Int Size, typename T, template <typename, Int> class BranchNodeEntryItem>
    void _remove(Int idx, BranchNodeEntryItem<T, Size>& accum)
    {

    }


    template <typename Fn>
    void read(Int block, Int start, Int end, Fn&& fn) const
    {
    }

    template <typename Fn>
    void read(Int start, Int end, Fn&& fn) const
    {

    }



    template <typename Fn>
    SizesT scan(Int start, Int end, Fn&& fn) const
    {
        return SizesT(end);
    }


    // ==================================== Dump =========================================== //


    void dump(std::ostream& out = cout) const
    {}


    void generateDataEvents(IPageDataEventHandler* handler) const
    {
        handler->startStruct();
        handler->startGroup("EMPTY_STRUCT");

        handler->value("ALLOCATOR",     &Base::allocator_offset());

        handler->endGroup();
        handler->endStruct();
    }

    void serialize(SerializationData& buf) const
    {
        FieldFactory<Int>::serialize(buf, Base::allocator_offset_);
    }

    void deserialize(DeserializationData& buf)
    {
        FieldFactory<Int>::deserialize(buf, Base::allocator_offset_);
    }
};




template <typename T, PkdSearchType S>
struct PkdStructSizeType<PackedEmptyStruct<T, S>> {
    static const PackedSizeType Value = PackedSizeType::FIXED;
};

template <typename T, PkdSearchType S>
struct StructSizeProvider<PackedEmptyStruct<T, S>> {
    static const Int Value = PackedEmptyStruct<T, S>::Blocks;
};



}}