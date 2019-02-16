
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


template <typename Value_ = int64_t, PkdSearchType SearchType_ = PkdSearchType::SUM>
class PackedEmptyStruct {

public:
    static const uint32_t VERSION = 1;
    static constexpr int32_t Indexes = 0;
    static constexpr PkdSearchType KeySearchType = SearchType_;


    using MyType = PackedEmptyStruct<Value_, SearchType_>;


    using Value = typename std::remove_reference<Value_>::type;

    static constexpr int32_t Blocks = Indexes;

    using InputBuffer = MyType;

    using Values = core::StaticVector<Value, Blocks>;

    using InputType = Values;
    using IndexValue = Value;
    using SizesT = core::StaticVector<int32_t, Blocks>;
    using ReadState = SizesT;

private:
    PackedAllocatable header_;
public:
    PackedEmptyStruct() = default;

    int32_t size() const {return 0;}

    int32_t block_size() const
    {
        return sizeof(MyType);
    }

    int32_t block_size(const MyType* other) const
    {
        return block_size();
    }

    static constexpr int32_t block_size(int32_t array_size)
    {
        return sizeof(MyType);
    }

    static constexpr int32_t packed_block_size(int32_t array_size)
    {
        return sizeof(MyType);
    }

    static int32_t elements_for(int32_t block_size)
    {
        size_t bsize = block_size;

        return bsize >= sizeof(MyType) ? std::numeric_limits<int32_t>::max() : 0;
    }


    OpStatus init(int32_t block_size) {return OpStatus::OK;}

    OpStatus init(const SizesT& capacities) {return OpStatus::OK;}

    static constexpr int32_t empty_size()
    {
        return sizeof(MyType);
    }


    OpStatus init()
    {
        return OpStatus::OK;
    }

    template <typename T>
    void max(T& accum) const
    {
    }

    template <typename T>
    OpStatus setValues(int32_t idx, T&&) {return OpStatus::OK;}

    template <typename T>
    OpStatus insert(int32_t idx, T&&) {return OpStatus::OK;}

    template <int32_t Offset, typename T>
    OpStatus _insert(int32_t idx, T&&) {return OpStatus::OK;}


    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    void max(BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset <= Size - Indexes, "Invalid balanced tree structure");
    }


    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    void sum(BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset <= Size - Indexes, "Invalid balanced tree structure");
    }

    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    void sum(int32_t start, int32_t end, BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset <= Size - Indexes, "Invalid balanced tree structure");
    }

    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    void sub(int32_t start, BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset <= Size - Indexes, "Invalid balanced tree structure");
    }

    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    void sum(int32_t idx, BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset <= Size - Indexes, "Invalid balanced tree structure");
    }

    template <typename T>
    int32_t append(int32_t size, T&&)
    {
        return size;
    }

    // =================================== Update ========================================== //

    OpStatus reindex() {return OpStatus::OK;}
    void check() const {}

    OpStatus remove(int32_t start, int32_t end)
    {
        return OpStatus::OK;
    }

    OpStatus removeSpace(int32_t room_start, int32_t room_end) {
        return remove(room_start, room_end);
    }



    OpStatus reset()
    {
        return OpStatus::OK;
    }


    OpStatus splitTo(MyType* other, int32_t idx)
    {
        return OpStatus::OK;
    }

    OpStatus mergeWith(MyType* other)
    {
        return OpStatus::OK;
    }

    // ===================================== IO ============================================ //

    OpStatus insert(int32_t pos, Value val)
    {
        return OpStatus::OK;
    }

    OpStatus insert(int32_t block, int32_t pos, Value val)
    {
        return OpStatus::OK;
    }

    OpStatus insert(int32_t pos, int32_t start, int32_t size, const InputBuffer* buffer)
    {
        return OpStatus::OK;
    }

    template <typename Adaptor>
    OpStatus insert(int32_t pos, int32_t size, Adaptor&& adaptor)
    {
        return OpStatus::OK;
    }


    OpStatusT<SizesT> insert_buffer(SizesT at, const InputBuffer* buffer, SizesT starts, SizesT ends, int32_t size)
    {
        return OpStatusT<SizesT>(at + SizesT(size));
    }

    OpStatusT<int32_t> insert_buffer(int32_t at, const InputBuffer* buffer, int32_t start, int32_t size)
    {
        return OpStatusT<int32_t>(at + size);
    }

    Values get_values(int32_t) const {
        return Values();
    }

    ReadState positions(int32_t idx) const {
        return ReadState(idx);
    }


    template <typename Adaptor>
    OpStatus _insert(int32_t pos, int32_t size, Adaptor&& adaptor)
    {
        return OpStatus::OK;
    }


    template <int32_t Offset, typename Value, typename T, int32_t Size, template <typename, int32_t> class BranchNodeEntryItem>
    OpStatus _update(int32_t pos, Value&& val, BranchNodeEntryItem<T, Size>& accum)
    {
        return OpStatus::OK;
    }

    template <int32_t Offset, typename Value, typename T, int32_t Size, template <typename, int32_t> class BranchNodeEntryItem>
    OpStatus _insert(int32_t pos, Value&& val, BranchNodeEntryItem<T, Size>& accum)
    {
        return OpStatus::OK;
    }

    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    OpStatus _remove(int32_t idx, BranchNodeEntryItem<T, Size>& accum)
    {
        return OpStatus::OK;
    }



    template <typename Fn>
    void read(int32_t block, int32_t start, int32_t end, Fn&& fn) const
    {
    }

    template <typename Fn>
    void read(int32_t start, int32_t end, Fn&& fn) const
    {

    }



    template <typename Fn>
    SizesT scan(int32_t start, int32_t end, Fn&& fn) const
    {
        return SizesT(end);
    }


    // ==================================== Dump =========================================== //


    void dump(std::ostream& out = std::cout) const
    {}


    void generateDataEvents(IBlockDataEventHandler* handler) const
    {
        handler->startStruct();
        handler->startGroup("EMPTY_STRUCT");
        handler->endGroup();
        handler->endStruct();
    }

    template <typename SerializationData>
    void serialize(SerializationData& buf) const
    {
        header_.serialize(buf);
    }

    template <typename DeserializationData>
    void deserialize(DeserializationData& buf)
    {
        header_.deserialize(buf);
    }
};




template <typename T, PkdSearchType S>
struct PkdStructSizeType<PackedEmptyStruct<T, S>> {
    static const PackedSizeType Value = PackedSizeType::FIXED;
};

template <typename T, PkdSearchType S>
struct StructSizeProvider<PackedEmptyStruct<T, S>> {
    static const int32_t Value = PackedEmptyStruct<T, S>::Blocks;
};



}}
