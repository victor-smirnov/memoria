
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
#include <memoria/v1/core/tools/assert.hpp>

#include <limits>

namespace memoria {
namespace v1 {


template <typename Value_ = int64_t, int32_t Indexes_ = 0, PkdSearchType SearchType_ = PkdSearchType::SUM>
class PackedSizedStruct: public PackedAllocatable {

    typedef PackedAllocatable                                                   Base;

public:
    static const uint32_t VERSION = 1;
    static constexpr int32_t Indexes = Indexes_;
    static constexpr PkdSearchType KeySearchType = SearchType_;


    using MyType = PackedSizedStruct;


    using Value = typename std::remove_reference<Value_>::type;

    static constexpr int32_t Blocks = Indexes;



    using InputBuffer = MyType;

    using Values = core::StaticVector<Value, Blocks>;

    using InputType = Values;
    using IndexValue = Value;
    using SizesT = core::StaticVector<int32_t, Blocks>;
    using ReadState = SizesT;

    class AppendState {
        int32_t size_ = 0;
    public:
        int32_t& size() {return size_;}
        const int32_t& size() const {return size_;}
    };



private:

    int32_t size_;

public:
    PackedSizedStruct() {}

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


    void init(int32_t block_size)
    {
        size_ = 0;
    }

    void init(const SizesT& capacities)
    {
        size_ = 0;
    }

    static constexpr int32_t empty_size()
    {
        return sizeof(MyType);
    }


    void init()
    {
        size_ = 0;
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
    void setValues(int32_t idx, T&&) {}

    template <typename T>
    void insert(int32_t idx, T&&) {
        insertSpace(idx, 1);
    }

    template <int32_t Offset, typename T>
    void _insert(int32_t idx, T&&) {
        insertSpace(idx, 1);
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


    void copyTo(MyType* other) const
    {
        other->size_ = this->size_;
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

    void reindex() {}
    void check() const
    {
        MEMORIA_V1_ASSERT(size_, >=, 0);
    }

    void remove(int32_t start, int32_t end)
    {
        if (end < 0) {
            int32_t a = 0; a++;
        }

        MEMORIA_V1_ASSERT_TRUE(start >= 0);
        MEMORIA_V1_ASSERT_TRUE(end >= 0);

        int32_t room_length = end - start;
        size_ -= room_length;
    }

    void removeSpace(int32_t room_start, int32_t room_end) {
        remove(room_start, room_end);
    }

    void insertSpace(int32_t idx, int32_t room_length)
    {
        if (idx > this->size()) {
            int a = 0;
            a++;
        }

        if (room_length < 0) {
            int a = 0; a++;
        }


        MEMORIA_V1_ASSERT(idx, <=, this->size());
        MEMORIA_V1_ASSERT(idx, >=, 0);
        MEMORIA_V1_ASSERT(room_length, >=, 0);

        size_ += room_length;
    }

    void reset()
    {
        size_ = 0;
    }


    void splitTo(MyType* other, int32_t idx)
    {
        MEMORIA_V1_ASSERT(other->size(), ==, 0);

        int32_t split_size = this->size() - idx;
        other->insertSpace(0, split_size);

        removeSpace(idx, this->size());
    }

    void mergeWith(MyType* other)
    {
        int32_t my_size     = this->size();
        int32_t other_size  = other->size();

        other->insertSpace(other_size, my_size);

        removeSpace(0, my_size);
    }

    // ===================================== IO ============================================ //

    void insert(int32_t pos, Value val)
    {
        insertSpace(pos, 1);
    }

    void insert(int32_t block, int32_t pos, Value val)
    {
        insertSpace(pos, 1);
    }

    void insert(int32_t pos, int32_t start, int32_t size, const InputBuffer* buffer)
    {
        insertSpace(pos, size);
    }

    template <typename Adaptor>
    void insert(int32_t pos, int32_t size, Adaptor&& adaptor)
    {
        insertSpace(pos, size);
    }


    SizesT insert_buffer(SizesT at, const InputBuffer* buffer, SizesT starts, SizesT ends, int32_t size)
    {
        insertSpace(at[0], size);
        return at + SizesT(size);
    }

    int32_t insert_buffer(int32_t at, const InputBuffer* buffer, int32_t start, int32_t size)
    {
        insertSpace(at, size);
        return at + size;
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
    void _insert(int32_t pos, int32_t size, Adaptor&& adaptor)
    {
        insertSpace(pos, size);
    }


    template <int32_t Offset, typename Value, typename T, int32_t Size, template <typename, int32_t> class BranchNodeEntryItem>
    void _update(int32_t pos, Value&& val, BranchNodeEntryItem<T, Size>& accum)
    {}

    template <int32_t Offset, typename T, int32_t Size, template <typename, int32_t> class BranchNodeEntryItem, typename AccessorFn>
    void _update_b(int32_t pos, BranchNodeEntryItem<T, Size>& accum, AccessorFn&&)
    {}

    template <int32_t Offset, typename Value, typename T, int32_t Size, template <typename, int32_t> class BranchNodeEntryItem>
    void _insert(int32_t pos, Value&& val, BranchNodeEntryItem<T, Size>& accum)
    {
        if (Offset < Size)
        {
            accum[Offset] += 1;
        }

        insertSpace(pos, 1);
    }


    template <int32_t Offset, typename T, int32_t Size, template <typename, int32_t> class BranchNodeEntryItem, typename AccessorFn>
    void _insert_b(int32_t pos, BranchNodeEntryItem<T, Size>& accum, AccessorFn&&)
    {
        if (Offset < Size)
        {
            accum[Offset] += 1;
        }

        insertSpace(pos, 1);
    }


    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    void _remove(int32_t idx, BranchNodeEntryItem<T, Size>& accum)
    {
        remove(idx, idx + 1);
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


    void dump(std::ostream& out = cout) const
    {
        out<<"size_ = "<<size_<<endl;
    }


    void generateDataEvents(IPageDataEventHandler* handler) const
    {
        handler->startStruct();
        handler->startGroup("SIZED_STRUCT");

//        handler->value("ALLOCATOR",     &Base::allocator_offset());
        handler->value("SIZE",          &size_);

        handler->endGroup();
        handler->endStruct();
    }

    void serialize(SerializationData& buf) const
    {
        FieldFactory<int32_t>::serialize(buf, Base::allocator_offset_);
        FieldFactory<int32_t>::serialize(buf, size_);
    }

    void deserialize(DeserializationData& buf)
    {
        FieldFactory<int32_t>::deserialize(buf, Base::allocator_offset_);
        FieldFactory<int32_t>::deserialize(buf, size_);
    }
};


using StreamSize = PackedSizedStruct<int64_t, 1, PkdSearchType::SUM>;

template <typename T, int32_t V, PkdSearchType S>
struct PkdStructSizeType<PackedSizedStruct<T, V, S>> {
    static const PackedSizeType Value = PackedSizeType::FIXED;
};

template <typename T, int32_t V, PkdSearchType S>
struct StructSizeProvider<PackedSizedStruct<T, V, S>> {
    static const int32_t Value = PackedSizedStruct<T, V, S>::Blocks;
};



}}
