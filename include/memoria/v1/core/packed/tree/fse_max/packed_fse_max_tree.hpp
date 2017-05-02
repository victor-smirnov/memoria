
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

#include <memoria/v1/core/packed/tree/fse_max/packed_fse_max_tree_base.hpp>
#include <memoria/v1/core/packed/buffer/packed_fse_input_buffer_ro.hpp>
#include <memoria/v1/core/packed/tools/packed_tools.hpp>


#include <memoria/v1/core/tools/static_array.hpp>
#include <memoria/v1/core/tools/optional.hpp>
#include <memoria/v1/core/tools/iobuffer/io_buffer.hpp>

namespace memoria {
namespace v1 {

template <typename ValueT, int32_t kBlocks, int32_t kBranchingFactor = PackedTreeBranchingFactor, int32_t kValuesPerBranch = PackedTreeBranchingFactor>
struct PkdFMTreeTypes {
    using Value = ValueT;

    static constexpr int32_t Blocks = kBlocks;
    static constexpr int32_t BranchingFactor = kBranchingFactor;
    static constexpr int32_t ValuesPerBranch = kValuesPerBranch;
};

template <typename Types> class PkdFMTree;


template <typename ValueT, int32_t kBlocks = 1, int32_t kBranchingFactor = PackedTreeBranchingFactor, int32_t kValuesPerBranch = PackedTreeBranchingFactor>
using PkdFMTreeT = PkdFMTree<PkdFMTreeTypes<ValueT, kBlocks, kBranchingFactor, kValuesPerBranch>>;



template <typename Types>
class PkdFMTree: public PkdFMTreeBase<typename Types::Value, Types::BranchingFactor, Types::ValuesPerBranch> {

    using Base      = PkdFMTreeBase<typename Types::Value, Types::BranchingFactor, Types::ValuesPerBranch>;
    using MyType    = PkdFMTree<Types>;

public:




    static constexpr uint32_t VERSION = 1;
    static constexpr int32_t Blocks = Types::Blocks;

    using Base::METADATA;
    using Base::index_size;
    using Base::SegmentsPerBlock;

    using FieldsList = MergeLists<
                typename Base::FieldsList,
                ConstValue<uint32_t, VERSION>,
                ConstValue<uint32_t, Blocks>
    >;

    using IndexValue = typename Types::Value;
    using Value      = typename Types::Value;

    using Values = core::StaticVector<IndexValue, Blocks>;

    using Metadata = typename Base::Metadata;

    using InputBuffer   = PackedFSERowOrderInputBuffer<PackedFSERowOrderInputBufferTypes<Value, Blocks>>;
    using InputType     = Values;

    using SizesT        = core::StaticVector<int32_t, Blocks>;
    using PtrsT         = core::StaticVector<Value*, Blocks>;
    using ConstPtrsT    = core::StaticVector<const Value*, Blocks>;

    class ReadState {
        ConstPtrsT values_;
        int32_t idx_ = 0;
    public:
        ConstPtrsT& values() {return values_;}
        int32_t& idx() {return idx_;}
        const ConstPtrsT& values() const {return values_;}
        const int32_t& idx() const {return idx_;}
    };

    static int32_t estimate_block_size(int32_t tree_capacity, int32_t density_hi = 1, int32_t density_lo = 1)
    {
        MEMORIA_V1_ASSERT(density_hi, ==, 1); // data density should not be set for this type of trees
        MEMORIA_V1_ASSERT(density_lo, ==, 1);

        return block_size(tree_capacity);
    }

    void init_tl(int32_t data_block_size)
    {
        Base::init_tl(data_block_size, Blocks);
    }

    void init(int32_t capacity = 0)
    {
        Base::init(empty_size(), Blocks * SegmentsPerBlock + 1);

        Metadata* meta = this->template allocate<Metadata>(METADATA);

        meta->size()        = 0;
        meta->max_size()    = capacity;
        meta->index_size()  = MyType::index_size(capacity);

        for (int32_t block = 0; block < Blocks; block++)
        {
            this->template allocateArrayBySize<IndexValue>(block * SegmentsPerBlock + 1, meta->index_size());
            this->template allocateArrayBySize<Value>(block * SegmentsPerBlock + 2, capacity);
        }
    }

    void init(const SizesT& sizes)
    {
        MyType::init(sizes[0]);
    }

    static int32_t block_size(int32_t capacity)
    {
        return Base::block_size(Blocks, capacity);
    }

    static int32_t packed_block_size(int32_t tree_capacity)
    {
        return block_size(tree_capacity);
    }


    int32_t block_size() const
    {
        return Base::block_size();
    }

    int32_t block_size(const MyType* other) const
    {
        return block_size(this->size() + other->size());
    }




    static int32_t elements_for(int32_t block_size)
    {
        return Base::tree_size(Blocks, block_size);
    }

    static int32_t expected_block_size(int32_t items_num)
    {
        return block_size(items_num);
    }



    static int32_t empty_size()
    {
        return block_size(0);
    }

    void reindex() {
        Base::reindex(Blocks);
    }

    void check() const {
        Base::check(Blocks);
    }

    void dump_index(std::ostream& out = cout) const {
        Base::dump_index(Blocks, out);
    }

    void dump(std::ostream& out = cout, bool dump_index = true) const {
        Base::dump(Blocks, out, dump_index);
    }

    bool check_capacity(int32_t size) const
    {
        MEMORIA_V1_ASSERT_TRUE(size >= 0);

        auto alloc = this->allocator();

        int32_t total_size          = this->size() + size;
        int32_t total_block_size    = MyType::block_size(total_size);
        int32_t my_block_size       = alloc->element_size(this);
        int32_t delta               = total_block_size - my_block_size;

        return alloc->free_space() >= delta;
    }





    Values get_values(int32_t idx) const
    {
        Values v;

        for (int32_t i = 0; i < Blocks; i++)
        {
            v[i] = this->value(i, idx);
        }

        return v;
    }

    Value get_values(int32_t idx, int32_t index) const
    {
        return this->value(index, idx);
    }

    Value getValue(int32_t index, int32_t idx) const
    {
        return this->value(index, idx);
    }


    Optional<Value> max(int32_t block) const
    {
        auto size = this->size();

        if (size > 0)
        {
            return this->value(block, size - 1);
        }
        else {
            return Optional<Value>();
        }
    }

    template <typename T>
    void addValues(int32_t idx, const core::StaticVector<T, Blocks>& values)
    {
        for (int32_t b = 0; b < Blocks; b++) {
            this->values(b)[idx] = values[b];
        }

        reindex();
    }

    void sums(Values& values) const
    {
        for (int32_t c = 0; c < Blocks; c++) {
            values[c] = this->max(c);
        }
    }

    void sums(int32_t start, int32_t end, Values& values) const
    {
        if (end - 1 > start)
        {
            for (int32_t c = 0; c < Blocks; c++)
            {
                values[c] = this->values(c)[end - 1];
            }
        }
    }

    template <typename T>
    void max(core::StaticVector<T, Blocks>& accum) const
    {
        for (int32_t block = 0; block < Blocks; block++)
        {
            accum[block] = this->max(block);
        }
    }

    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    void max(BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset <= Size - Blocks, "Invalid balanced tree structure");

        for (int32_t block = 0; block < Blocks; block++)
        {
            accum[block + Offset] = this->max(block);
        }
    }


    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    void sum(BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset <= Size - Blocks, "Invalid balanced tree structure");

        for (int32_t block = 0; block < Blocks; block++)
        {
            accum[block + Offset] = this->max(block);
        }
    }

    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    void sum(int32_t start, int32_t end, BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset <= Size - Blocks, "Invalid balanced tree structure");

        for (int32_t block = 0; block < Blocks; block++)
        {
            accum[block + Offset] = this->value(block, end);
        }
    }

    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    void sum(int32_t idx, BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset <= Size - Blocks, "Invalid balanced tree structure");

        for (int32_t block = 0; block < Blocks; block++)
        {
            accum[block + Offset] = this->value(block, idx);
        }
    }

    template <int32_t Offset, int32_t From, int32_t To, typename T, template <typename, int32_t, int32_t> class BranchNodeEntryItem>
    void sum(int32_t start, int32_t end, BranchNodeEntryItem<T, From, To>& accum) const
    {
        for (int32_t block = 0; block < Blocks; block++)
        {
            accum[block + Offset] = this->value(block, end);
        }
    }



    // ========================================= Insert/Remove/Resize ============================================== //

protected:
    void resize(Metadata* meta, int32_t size)
    {
        int32_t new_data_size  = meta->max_size() + size;
        int32_t new_index_size = MyType::index_size(new_data_size);

        for (int32_t block = 0; block < Blocks; block++)
        {
            Base::resizeBlock(SegmentsPerBlock * block + 1, new_index_size * sizeof(IndexValue));
            Base::resizeBlock(SegmentsPerBlock * block + 2, new_data_size * sizeof(Value));
        }

        meta->max_size()    += size;
        meta->index_size()  = new_index_size;
    }

public:
    void insertSpace(int32_t idx, int32_t room_length)
    {
        auto meta = this->metadata();

        if (idx > meta->size())
        {
            int a = 0; a++;
        }

        MEMORIA_V1_ASSERT(idx, <=, meta->size());

        int32_t capacity  = meta->capacity();

        if (capacity < room_length)
        {
            resize(meta, room_length - capacity);
        }

        for (int32_t block = 0; block < Blocks; block++)
        {
            auto* values = this->values(block);

            CopyBuffer(
                    values + idx,
                    values + idx + room_length,
                    meta->size() - idx
            );

            for (int32_t c = idx; c < idx + room_length; c++) {
                values[c] = Value();
            }
        }

        meta->size() += room_length;
    }

protected:
    void copyTo(MyType* other, int32_t copy_from, int32_t count, int32_t copy_to) const
    {
        MEMORIA_V1_ASSERT_TRUE(copy_from >= 0);
        MEMORIA_V1_ASSERT_TRUE(count >= 0);

        for (int32_t block = 0; block < Blocks; block++)
        {
            auto my_values    = this->values(block);
            auto other_values = other->values(block);

            CopyBuffer(
                    my_values + copy_from,
                    other_values + copy_to,
                    count
            );
        }
    }

public:
    void splitTo(MyType* other, int32_t idx)
    {
        int32_t total = this->size() - idx;
        other->insertSpace(0, total);

        copyTo(other, idx, total, 0);
        other->reindex();

        removeSpace(idx, this->size());
        reindex();
    }

    void mergeWith(MyType* other)
    {
        int32_t my_size     = this->size();
        int32_t other_size  = other->size();

        other->insertSpace(other_size, my_size);

        copyTo(other, 0, my_size, other_size);

        removeSpace(0, my_size);

        reindex();
        other->reindex();
    }


    void removeSpace(int32_t start, int32_t end)
    {
        remove(start, end);
    }

    void remove(int32_t start, int32_t end)
    {
        auto meta = this->metadata();

        int32_t room_length = end - start;
        int32_t size = meta->size();

        for (int32_t block = Blocks - 1; block >= 0; block--)
        {
            auto values = this->values(block);

            CopyBuffer(
                    values + end,
                    values + start,
                    size - end
            );

            for (int32_t c = start + size - end; c < size; c++)
            {
                values[c] = Value();
            }
        }

        meta->size() -= room_length;

        resize(meta, -room_length);

        reindex();
    }


    template <int32_t Offset, int32_t Size, typename T1, typename T2, template <typename, int32_t> class BranchNodeEntryItem>
    void _insert(int32_t idx, const core::StaticVector<T1, Blocks>& values, BranchNodeEntryItem<T2, Size>& accum)
    {
        insert(idx, values);

        sum<Offset>(this->size() - 1, accum);
    }

    template <int32_t Offset, int32_t Size, typename AccessorFn, typename T2, template <typename, int32_t> class BranchNodeEntryItem>
    void _insert_b(int32_t idx, BranchNodeEntryItem<T2, Size>& accum, AccessorFn&& values)
    {
        insertSpace(idx, 1);

        for (int32_t b = 0; b < Blocks; b++) {
            this->values(b)[idx] = values(b);
        }

        reindex();

        sum<Offset>(this->size() - 1, accum);
    }


    template <int32_t Offset, int32_t Size, typename T1, typename T2, template <typename, int32_t> class BranchNodeEntryItem>
    void _update(int32_t idx, const core::StaticVector<T1, Blocks>& values, BranchNodeEntryItem<T2, Size>& accum)
    {
        update(idx, values);

        sum<Offset>(this->size() - 1, accum);
    }

    template <int32_t Offset, int32_t Size, typename T2, template <typename, int32_t> class BranchNodeEntryItem, typename AccessorFn>
    void _update_b(int32_t idx, BranchNodeEntryItem<T2, Size>& accum, AccessorFn&& values)
    {
        for (int32_t b = 0; b < Blocks; b++)
        {
            this->values(b)[idx] = values(b);
        }

        reindex();

        sum<Offset>(this->size() - 1, accum);
    }


    template <int32_t Offset, int32_t Size, typename T1, typename T2, typename I, template <typename, int32_t> class BranchNodeEntryItem>
    void _update(int32_t idx, const std::pair<T1, I>& values, BranchNodeEntryItem<T2, Size>& accum)
    {
        this->setValue(values.first, idx, values.second);

        sum<Offset>(this->size() - 1, accum);
    }

    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    void _remove(int32_t idx, BranchNodeEntryItem<T, Size>& accum)
    {
        remove(idx, idx + 1);

        sum<Offset>(this->size() - 1, accum);
    }



    void insert(int32_t idx, int32_t size, std::function<const Values& (int32_t)> provider, bool reindex = true)
    {
        insertSpace(idx, size);

        typename Base::Value* values[Blocks];
        for (int32_t block  = 0; block < Blocks; block++)
        {
            values[block] = this->values(block);
        }

        for (int32_t c = idx; c < idx + size; c++)
        {
            const Values& vals = provider(c - idx);

            for (int32_t block = 0; block < Blocks; block++)
            {
                values[block][c] = vals[block];
            }
        }

        if (reindex) {
            this->reindex();
        }
    }

    template <typename T>
    void insert(int32_t idx, const core::StaticVector<T, Blocks>& values)
    {
        insertSpace(idx, 1);
        setValues(idx, values);
    }


    template <typename Adaptor>
    void _insert(int32_t pos, int32_t size, Adaptor&& adaptor)
    {
        populate(pos, size, std::forward<Adaptor>(adaptor));
        reindex();
    }

    template <typename Adaptor>
    void populate(int32_t pos, int32_t size, Adaptor&& adaptor)
    {
        insertSpace(pos, size);

        for (int32_t c = 0; c < size; c++)
        {
            for (int32_t block = 0; block < Blocks; block++)
            {
                const auto& item = adaptor(block, c);
                this->value(block, c + pos) = item;
            }
        }
    }



    SizesT insert_buffer(SizesT at, const InputBuffer* buffer, SizesT starts, SizesT ends, int32_t inserted)
    {
        auto buffer_values = buffer->values() + starts[0] * Blocks;

        _insert(at[0], inserted, [=](int32_t block, int32_t idx) -> const auto& {
            return buffer_values[idx * Blocks + block];
        });

        return at + SizesT(inserted);
    }

    void insert_buffer(int32_t at, const InputBuffer* buffer, int32_t start, int32_t inserted)
    {
        auto buffer_values = buffer->values() + start * Blocks;

        _insert(at, inserted, [=](int32_t block, int32_t idx) -> const auto& {
            return buffer_values[idx * Blocks + block];
        });
    }

    template <typename T>
    void update(int32_t idx, const core::StaticVector<T, Blocks>& values)
    {
        setValues(idx, values);
    }




    int64_t setValue(int32_t block, int32_t idx, const Value& value)
    {
        if (value != 0)
        {
            Value val = this->value(block, idx);
            this->value(block, idx) = value;

            return val - value;
        }
        else {
            return 0;
        }
    }



    template <typename T>
    void setValues(int32_t idx, const core::StaticVector<T, Blocks>& values)
    {
        for (int32_t b = 0; b < Blocks; b++) {
            this->values(b)[idx] = values[b];
        }

        reindex();
    }

    ReadState positions(int32_t idx) const
    {
        ReadState state;

        state.idx() = idx;

        for (int32_t b = 0; b < Blocks; b++) {
            state.values()[b] = this->values(b);
        }

        return state;
    }


    template <typename IOBuffer>
    bool readTo(ReadState& state, IOBuffer& buffer) const
    {
        for (int32_t b = 0; b < Blocks; b++)
        {
            auto val = state.values()[b][state.idx()];

            if (!IOBufferAdapter<Value>::put(buffer, val))
            {
                return false;
            }
        }

        state.idx()++;

        return true;
    }


    template <typename Fn>
    void read(int32_t start, int32_t end, Fn&& fn) const
    {
        auto meta = this->metadata();

        MEMORIA_V1_ASSERT(start, >=, 0);
        MEMORIA_V1_ASSERT(start, <=, end);
        MEMORIA_V1_ASSERT(end, <=, meta->size());

        const Value* values[Blocks];
        for (int32_t b = 0; b < Blocks; b++)
        {
            values[b] = this->values(b);
        }

        for (int32_t c = start; c < end; c++)
        {
            for (int32_t b = 0; b < Blocks; b++)
            {
                fn(b, values[b][c]);
            }

            fn.next();
        }
    }


    template <typename Fn>
    void read(int32_t block, int32_t start, int32_t end, Fn&& fn) const
    {
        auto meta = this->metadata();

        MEMORIA_V1_ASSERT(start, >=, 0);
        MEMORIA_V1_ASSERT(start, <=, end);
        MEMORIA_V1_ASSERT(end, <=, meta->size());

        auto values = this->values(block);
        for (int32_t c = start; c < end; c++)
        {
            fn(block, values[c]);
            fn.next();
        }
    }

    void clear()
    {
        init();

        if (Base::has_allocator())
        {
            auto alloc = this->allocator();
            int32_t empty_size = MyType::empty_size();
            alloc->resizeBlock(this, empty_size);
        }
    }

    void clear(int32_t start, int32_t end)
    {
        for (int32_t block = 0; block < Blocks; block++)
        {
            auto values = this->values(block);

            for (int32_t c = start; c < end; c++)
            {
                values[c] = 0;
            }
        }
    }

    void generateDataEvents(IPageDataEventHandler* handler) const
    {
//        Base::generateDataEvents(handler);

        handler->startStruct();
        handler->startGroup("FSM_TREE");

        auto meta = this->metadata();

        handler->value("SIZE",          &meta->size());
        handler->value("MAX_SIZE",      &meta->max_size());
        handler->value("INDEX_SIZE",    &meta->index_size());

        handler->startGroup("INDEXES", meta->index_size());

        auto index_size = meta->index_size();

        const IndexValue* index[Blocks];

        for (int32_t b = 0; b < Blocks; b++) {
            index[b] = this->index(b);
        }

        for (int32_t c = 0; c < index_size; c++)
        {
            handler->value("INDEX", PageValueProviderFactory::provider(Blocks, [&](int32_t idx) {
                return index[idx][c];
            }));
        }

        handler->endGroup();

        handler->startGroup("DATA", meta->size());



        const Value* values[Blocks];

        for (int32_t b = 0; b < Blocks; b++) {
            values[b] = this->values(b);
        }

        for (int32_t c = 0; c < meta->size(); c++)
        {
            handler->value("TREE_ITEM", PageValueProviderFactory::provider(Blocks, [&](int32_t idx) {
                return values[idx][c];
            }));
        }

        handler->endGroup();

        handler->endGroup();

        handler->endStruct();
    }

    void serialize(SerializationData& buf) const
    {
        Base::serialize(buf);

        const Metadata* meta = this->metadata();

        FieldFactory<int32_t>::serialize(buf, meta->size());
        FieldFactory<int32_t>::serialize(buf, meta->max_size());
        FieldFactory<int32_t>::serialize(buf, meta->index_size());

        for (int32_t b = 0; b < Blocks; b++)
        {
            FieldFactory<IndexValue>::serialize(buf, this->index(b), meta->index_size());
            FieldFactory<Value>::serialize(buf, this->values(b), meta->size());
        }
    }

    void deserialize(DeserializationData& buf)
    {
        Base::deserialize(buf);

        Metadata* meta = this->metadata();

        FieldFactory<int32_t>::deserialize(buf, meta->size());
        FieldFactory<int32_t>::deserialize(buf, meta->max_size());
        FieldFactory<int32_t>::deserialize(buf, meta->index_size());

        for (int32_t b = 0; b < Blocks; b++)
        {
            FieldFactory<IndexValue>::deserialize(buf, this->index(b), meta->index_size());
            FieldFactory<Value>::deserialize(buf, this->values(b), meta->size());
        }
    }
};

template <typename Types>
struct PkdStructSizeType<PkdFMTree<Types>> {
    static const PackedSizeType Value = PackedSizeType::FIXED;
};


template <typename Types>
struct StructSizeProvider<PkdFMTree<Types>> {
    static const int32_t Value = PkdFMTree<Types>::Blocks;
};

template <typename Types>
struct IndexesSize<PkdFMTree<Types>> {
    static const int32_t Value = PkdFMTree<Types>::Blocks;
};


template <typename T>
struct PkdSearchKeyTypeProvider<PkdFMTree<T>> {
	using Type = Optional<typename PkdFMTree<T>::Value>;
};


}}
