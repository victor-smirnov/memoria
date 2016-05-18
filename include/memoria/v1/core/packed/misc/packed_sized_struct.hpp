
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


template <typename Value_ = BigInt, Int Indexes_ = 0, PkdSearchType SearchType_ = PkdSearchType::SUM>
class PackedSizedStruct: public PackedAllocatable {

    typedef PackedAllocatable                                                   Base;

public:
    static const UInt VERSION = 1;
    static constexpr Int Indexes = Indexes_;
    static constexpr PkdSearchType KeySearchType = SearchType_;


    using MyType = PackedSizedStruct;


    using Value = typename std::remove_reference<Value_>::type;

    static constexpr Int Blocks = Indexes;



    using InputBuffer = MyType;

    using Values = core::StaticVector<Value, Blocks>;

    using InputType = Values;
    using IndexValue = Value;
    using SizesT = core::StaticVector<Int, Blocks>;
    using ReadState = SizesT;

    class AppendState {
    	Int size_ = 0;
    public:
    	Int& size() {return size_;}
    	const Int& size() const {return size_;}
    };



private:

    Int size_;

public:
    PackedSizedStruct() {}

    Int& size() {return size_;}
    const Int& size() const {return size_;}


    Int block_size() const
    {
        return sizeof(MyType);
    }

    Int block_size(const MyType* other) const
    {
        return block_size(size_ + other->size_);
    }

    static constexpr Int block_size(Int array_size)
    {
        return sizeof(MyType);
    }

    static constexpr Int block_size(SizesT array_size)
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


    void init(Int block_size)
    {
        size_ = 0;
    }

    void init(const SizesT& capacities)
    {
        size_ = 0;
    }

    static constexpr Int empty_size()
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
    static SizesT calculate_size(Int size, Adaptor&& fn)
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
    void setValues(Int idx, T&&) {}

    template <typename T>
    void insert(Int idx, T&&) {
        insertSpace(idx, 1);
    }

    template <Int Offset, typename T>
    void _insert(Int idx, T&&) {
        insertSpace(idx, 1);
    }


    template <Int Offset, Int Size, typename T, template <typename, Int> class BranchNodeEntryItem>
    void max(BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset <= Size - Indexes, "Invalid balanced tree structure");

        if (Indexes > 0)
        {
            accum[Offset] = size_;
        }
    }


    template <Int Offset, Int Size, typename T, template <typename, Int> class BranchNodeEntryItem>
    void sum(BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset <= Size - Indexes, "Invalid balanced tree structure");
        accum[Offset] += size_;
    }

    template <Int Offset, Int Size, typename T, template <typename, Int> class BranchNodeEntryItem>
    void sum(Int start, Int end, BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset <= Size - Indexes, "Invalid balanced tree structure");
        accum[Offset] += end - start;
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
    Int append(Int size, T&&)
    {
        this->size_ += size;

        return size;
    }


    void fill(Int size)
    {
    	this->size_ += size;
    }

    // =================================== Update ========================================== //

    void reindex() {}
    void check() const
    {
    	MEMORIA_V1_ASSERT(size_, >=, 0);
    }

    void remove(Int start, Int end)
    {
    	if (end < 0) {
    		Int a = 0; a++;
    	}

        MEMORIA_V1_ASSERT_TRUE(start >= 0);
        MEMORIA_V1_ASSERT_TRUE(end >= 0);

        Int room_length = end - start;
        size_ -= room_length;
    }

    void removeSpace(Int room_start, Int room_end) {
        remove(room_start, room_end);
    }

    void insertSpace(Int idx, Int room_length)
    {
        if (idx > this->size()) {
        	int a = 0;
        	a++;
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


    void splitTo(MyType* other, Int idx)
    {
        MEMORIA_V1_ASSERT(other->size(), ==, 0);

        Int split_size = this->size() - idx;
        other->insertSpace(0, split_size);

        removeSpace(idx, this->size());
    }

    void mergeWith(MyType* other)
    {
        Int my_size     = this->size();
        Int other_size  = other->size();

        other->insertSpace(other_size, my_size);

        removeSpace(0, my_size);
    }

    // ===================================== IO ============================================ //

    void insert(Int pos, Value val)
    {
        insertSpace(pos, 1);
    }

    void insert(Int block, Int pos, Value val)
    {
        insertSpace(pos, 1);
    }

    void insert(Int pos, Int start, Int size, const InputBuffer* buffer)
    {
        insertSpace(pos, size);
    }

    template <typename Adaptor>
    void insert(Int pos, Int size, Adaptor&& adaptor)
    {
        insertSpace(pos, size);
    }


    SizesT insert_buffer(SizesT at, const InputBuffer* buffer, SizesT starts, SizesT ends, Int size)
    {
        insertSpace(at[0], size);
        return at + SizesT(size);
    }

    Int insert_buffer(Int at, const InputBuffer* buffer, Int start, Int size)
    {
        insertSpace(at, size);
        return at + size;
    }

    Value value(Int, Int) const {
        return Value();
    }

    ReadState positions(Int idx) const {
        return ReadState(idx);
    }

    Values get_values(Int) const {
        return Values();
    }

    Values get_values(Int, Int) const {
        return Values();
    }


    template <typename Adaptor>
    void _insert(Int pos, Int size, Adaptor&& adaptor)
    {
        insertSpace(pos, size);
    }


    template <Int Offset, typename Value, typename T, Int Size, template <typename, Int> class BranchNodeEntryItem>
    void _update(Int pos, Value&& val, BranchNodeEntryItem<T, Size>& accum)
    {}

    template <Int Offset, typename T, Int Size, template <typename, Int> class BranchNodeEntryItem, typename AccessorFn>
    void _update_b(Int pos, BranchNodeEntryItem<T, Size>& accum, AccessorFn&&)
    {}

    template <Int Offset, typename Value, typename T, Int Size, template <typename, Int> class BranchNodeEntryItem>
    void _insert(Int pos, Value&& val, BranchNodeEntryItem<T, Size>& accum)
    {
        if (Offset < Size)
        {
            accum[Offset] += 1;
        }

        insertSpace(pos, 1);
    }


    template <Int Offset, typename T, Int Size, template <typename, Int> class BranchNodeEntryItem, typename AccessorFn>
    void _insert_b(Int pos, BranchNodeEntryItem<T, Size>& accum, AccessorFn&&)
    {
        if (Offset < Size)
        {
            accum[Offset] += 1;
        }

        insertSpace(pos, 1);
    }


    template <Int Offset, Int Size, typename T, template <typename, Int> class BranchNodeEntryItem>
    void _remove(Int idx, BranchNodeEntryItem<T, Size>& accum)
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
    void read(Int start, Int end, Fn&& fn) const
    {
        read(0, start, end, std::forward<Fn>(fn));
    }



    template <typename Fn>
    void read(Int block, Int start, Int end, Fn&& fn) const
    {
        MEMORIA_V1_ASSERT(start, <=, size_);
        MEMORIA_V1_ASSERT(start, >=, 0);
        MEMORIA_V1_ASSERT(end, >=, 0);
        MEMORIA_V1_ASSERT(end, <=, size_);

        for (Int c = start; c < end; c++)
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
        FieldFactory<Int>::serialize(buf, Base::allocator_offset_);
        FieldFactory<Int>::serialize(buf, size_);
    }

    void deserialize(DeserializationData& buf)
    {
        FieldFactory<Int>::deserialize(buf, Base::allocator_offset_);
        FieldFactory<Int>::deserialize(buf, size_);
    }
};


using StreamSize = PackedSizedStruct<BigInt, 1, PkdSearchType::SUM>;

template <typename T, Int V, PkdSearchType S>
struct PkdStructSizeType<PackedSizedStruct<T, V, S>> {
    static const PackedSizeType Value = PackedSizeType::FIXED;
};

template <typename T, Int V, PkdSearchType S>
struct StructSizeProvider<PackedSizedStruct<T, V, S>> {
    static const Int Value = PackedSizedStruct<T, V, S>::Blocks;
};



}}
