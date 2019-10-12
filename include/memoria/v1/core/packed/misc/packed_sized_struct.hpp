
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
#include <memoria/v1/core/tools/accessors.hpp>
#include <memoria/v1/core/tools/assert.hpp>

#include <memoria/v1/core/iovector/io_substream_base.hpp>

#include <memoria/v1/core/packed/misc/packed_sized_struct_so.hpp>

#include <limits>

namespace memoria {
namespace v1 {



template <typename DataType_ = int64_t, int32_t Indexes_ = 0, PkdSearchType SearchType_ = PkdSearchType::SUM>
class PackedSizedStruct {

public:
    static const uint32_t VERSION = 1;
    static constexpr int32_t Indexes = Indexes_;
    static constexpr PkdSearchType KeySearchType = SearchType_;


    using MyType = PackedSizedStruct;

    using DataType = std::remove_reference_t<DataType_>;

    using Value = typename DataTypeTraits<DataType>::ViewType;
    using IndexDataType = DataType;

    static constexpr int32_t Blocks = Indexes;

    using Values = core::StaticVector<Value, Blocks>;

    using IndexValue = Value;
    using SizesT = core::StaticVector<int32_t, Blocks>;
    using ReadState = SizesT;

    using ExtData = DTTTypeDimensionsTuple<DataType>;
    using SparseObject = PackedSizedStructSO<ExtData, MyType>;

    class AppendState {
        int32_t size_ = 0;
    public:
        int32_t& size() {return size_;}
        const int32_t& size() const {return size_;}
    };



private:
    PackedAllocatable header_;

    int32_t size_;

public:
    PackedSizedStruct() = default;

    int32_t& size() {return size_;}
    const int32_t& size() const {return size_;}


    int32_t block_size() const
    {
        return sizeof(MyType);
    }

    int32_t block_size(const MyType* other) const
    {
        return block_size(size_ + other->size_);
    }

    static constexpr int32_t block_size(int32_t array_size)
    {
        return sizeof(MyType);
    }

    static constexpr int32_t block_size(SizesT array_size)
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


    OpStatus init(int32_t block_size)
    {
        size_ = 0;
        return OpStatus::OK;
    }

    OpStatus init(const SizesT& capacities)
    {
        size_ = 0;
        return OpStatus::OK;
    }

    static constexpr int32_t empty_size()
    {
        return sizeof(MyType);
    }


    OpStatus init()
    {
        size_ = 0;

        return OpStatus::OK;
    }

    SizesT data_capacity() const
    {
        return SizesT(size_ * 2);
    }

    template <typename Adaptor>
    static SizesT calculate_size(int32_t size, Adaptor&& fn)
    {
        return SizesT(size);
    }

    bool has_capacity_for(const SizesT& sizes) const
    {
        return true;
    }

    template <typename T>
    void max(T& accum) const
    {
        if (Indexes > 0)
        {
            accum[0] = size_;
        }
    }

    template <typename T>
    OpStatus setValues(int32_t idx, T&&) {return OpStatus::OK;}

    template <typename T>
    OpStatus insert(int32_t idx, T&&) {
        return insertSpace(idx, 1);
    }

    template <int32_t Offset, typename T>
    OpStatus _insert(int32_t idx, T&&) {
        return insertSpace(idx, 1);
    }


    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    void max(BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset <= Size - Indexes, "Invalid balanced tree structure");

        if (Indexes > 0)
        {
            accum[Offset] = size_;
        }
    }


    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    void sum(BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset <= Size - Indexes, "Invalid balanced tree structure");
        accum[Offset] += size_;
    }

    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    void sum(int32_t start, int32_t end, BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset <= Size - Indexes, "Invalid balanced tree structure");
        accum[Offset] += end - start;
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
        accum[Offset] += idx;
    }


    AppendState append_state() const
    {
        AppendState state;

        state.size() = this->size();

        return state;
    }


    template <typename IOBuffer>
    bool append_entry_from_iobuffer(AppendState& state, IOBuffer& buffer)
    {
        state.size()++;

        this->size()++;

        return true;
    }


    OpStatus copyTo(MyType* other) const
    {
        other->size_ = this->size_;

        return OpStatus::OK;
    }



    void restore(const AppendState& state)
    {
        this->size() = state.size();
    }




    template <typename T>
    int32_t append(int32_t size, T&&)
    {
        this->size_ += size;

        return size;
    }


    void fill(int32_t size)
    {
        this->size_ += size;
    }

    // =================================== Update ========================================== //

    OpStatus reindex() {return OpStatus::OK;}

    void check() const
    {
        MEMORIA_V1_ASSERT(size_, >=, 0);
    }

    OpStatus remove(int32_t start, int32_t end)
    {
        if (end < 0) {
            int32_t a = 0; a++;
        }

        MEMORIA_V1_ASSERT_TRUE(start >= 0);
        MEMORIA_V1_ASSERT_TRUE(end >= 0);

        int32_t room_length = end - start;
        size_ -= room_length;

        return OpStatus::OK;
    }

    OpStatus removeSpace(int32_t room_start, int32_t room_end) {
        return remove(room_start, room_end);
    }

    OpStatus insertSpace(int32_t idx, int32_t room_length)
    {
        MEMORIA_V1_ASSERT(idx, <=, this->size());
        MEMORIA_V1_ASSERT(idx, >=, 0);
        MEMORIA_V1_ASSERT(room_length, >=, 0);

        size_ += room_length;

        return OpStatus::OK;
    }

    OpStatus reset()
    {
        size_ = 0;

        return OpStatus::OK;
    }


    OpStatus splitTo(MyType* other, int32_t idx)
    {
        MEMORIA_V1_ASSERT(other->size(), ==, 0);

        int32_t split_size = this->size() - idx;
        if(isFail(other->insertSpace(0, split_size))) {
            return OpStatus::FAIL;
        }

        return removeSpace(idx, this->size());
    }

    OpStatus mergeWith(MyType* other)
    {
        int32_t my_size     = this->size();
        int32_t other_size  = other->size();

        if(isFail(other->insertSpace(other_size, my_size))) {
            return OpStatus::FAIL;
        }

        return removeSpace(0, my_size);
    }

    // ===================================== IO ============================================ //

    OpStatus insert(int32_t pos, Value val)
    {
        return insertSpace(pos, 1);
    }

    OpStatus insert(int32_t block, int32_t pos, Value val)
    {
        return insertSpace(pos, 1);
    }


    template <typename Adaptor>
    OpStatus insert(int32_t pos, int32_t size, Adaptor&& adaptor)
    {
        return insertSpace(pos, size);
    }



    Value value(int32_t, int32_t) const {
        return Value();
    }

    ReadState positions(int32_t idx) const {
        return ReadState(idx);
    }

    Values get_values(int32_t) const {
        return Values();
    }

    Values get_values(int32_t, int32_t) const {
        return Values();
    }


    template <typename Adaptor>
    OpStatus _insert(int32_t pos, int32_t size, Adaptor&& adaptor)
    {
        return insertSpace(pos, size);
    }


    template <int32_t Offset, typename Value, typename T, int32_t Size, template <typename, int32_t> class BranchNodeEntryItem>
    OpStatus _update(int32_t pos, Value&& val, BranchNodeEntryItem<T, Size>& accum)
    {
        return OpStatus::OK;
    }

    template <int32_t Offset, typename T, int32_t Size, template <typename, int32_t> class BranchNodeEntryItem, typename AccessorFn>
    OpStatus _update_b(int32_t pos, BranchNodeEntryItem<T, Size>& accum, AccessorFn&&)
    {
        return OpStatus::OK;
    }

    template <int32_t Offset, typename Value, typename T, int32_t Size, template <typename, int32_t> class BranchNodeEntryItem>
    OpStatus _insert(int32_t pos, Value&& val, BranchNodeEntryItem<T, Size>& accum)
    {
        if (Offset < Size)
        {
            accum[Offset] += 1;
        }

        return insertSpace(pos, 1);
    }


    template <int32_t Offset, typename T, int32_t Size, template <typename, int32_t> class BranchNodeEntryItem, typename AccessorFn>
    OpStatus _insert_b(int32_t pos, BranchNodeEntryItem<T, Size>& accum, AccessorFn&&)
    {
        if (Offset < Size)
        {
            accum[Offset] += 1;
        }

        return insertSpace(pos, 1);
    }


    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    OpStatus _remove(int32_t idx, BranchNodeEntryItem<T, Size>& accum)
    {
        if(isFail(remove(idx, idx + 1))) {
            return OpStatus::FAIL;
        }

        return OpStatus::OK;
    }


    template <typename IOBuffer>
    bool readTo(ReadState& state, IOBuffer& buffer) const
    {
        // Don't read anything into the buffer
        return true;
    }


    template <typename Fn>
    void read(int32_t start, int32_t end, Fn&& fn) const
    {
        read(0, start, end, std::forward<Fn>(fn));
    }



    template <typename Fn>
    void read(int32_t block, int32_t start, int32_t end, Fn&& fn) const
    {
        MEMORIA_V1_ASSERT(start, <=, size_);
        MEMORIA_V1_ASSERT(start, >=, 0);
        MEMORIA_V1_ASSERT(end, >=, 0);
        MEMORIA_V1_ASSERT(end, <=, size_);

        for (int32_t c = start; c < end; c++)
        {
            fn(block, Value());
            fn.next();
        }
    }


    // ==================================== Dump =========================================== //


    void dump(std::ostream& out = std::cout) const
    {
        out << "size_ = " << size_ << std::endl;
    }


    void generateDataEvents(IBlockDataEventHandler* handler) const
    {
        handler->startStruct();
        handler->startGroup("SIZED_STRUCT");

        handler->value("SIZE", &size_);

        handler->endGroup();
        handler->endStruct();
    }

    template <typename SerializationData>
    void serialize(SerializationData& buf) const
    {
        header_.serialize(buf);
        FieldFactory<int32_t>::serialize(buf, size_);
    }

    template <typename DeserializationData>
    void deserialize(DeserializationData& buf)
    {
        header_.deserialize(buf);
        FieldFactory<int32_t>::deserialize(buf, size_);
    }
};


using StreamSize = PackedSizedStruct<int64_t, 1, PkdSearchType::SUM>;

template <typename T, int32_t V, PkdSearchType S>
struct PackedStructTraits<PackedSizedStruct<T, V, S>>
{
    using SearchKeyDataType = T;

    static constexpr PackedDataTypeSize DataTypeSize = PackedDataTypeSize::FIXED;
    static constexpr PkdSearchType KeySearchType = S;
    static constexpr int32_t Blocks = PackedSizedStruct<T, V, S>::Blocks;
    static constexpr int32_t Indexes = PackedSizedStruct<T, V, S>::Indexes;
};


}}
