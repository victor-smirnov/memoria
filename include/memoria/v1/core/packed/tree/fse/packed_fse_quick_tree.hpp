
// Copyright 2015 Victor Smirnov
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

#include <memoria/v1/core/packed/tree/fse/packed_fse_quick_tree_base.hpp>
#include <memoria/v1/core/packed/buffer/packed_fse_input_buffer_ro.hpp>

#include <memoria/v1/core/tools/static_array.hpp>
#include <memoria/v1/metadata/page.hpp>

namespace memoria {
namespace v1 {

template <typename IndexValueT, Int kBlocks, typename ValueT = IndexValueT, Int kBranchingFactor = PackedTreeBranchingFactor, Int kValuesPerBranch = PackedTreeBranchingFactor>
struct PkdFQTreeTypes {
    using IndexValue    = IndexValueT;
    using Value         = ValueT;

    static constexpr Int Blocks = kBlocks;
    static constexpr Int BranchingFactor = kBranchingFactor;
    static constexpr Int ValuesPerBranch = kValuesPerBranch;
};

template <typename Types> class PkdFQTree;


template <typename IndexValueT, Int kBlocks = 1, typename ValueT = IndexValueT, Int kBranchingFactor = PackedTreeBranchingFactor, Int kValuesPerBranch = PackedTreeBranchingFactor>
using PkdFQTreeT = PkdFQTree<PkdFQTreeTypes<IndexValueT, kBlocks, ValueT, kBranchingFactor, kValuesPerBranch>>;


using v1::core::StaticVector;

template <typename Types>
class PkdFQTree: public PkdFQTreeBase<typename Types::IndexValue, typename Types::Value, Types::BranchingFactor, Types::ValuesPerBranch> {

    using Base      = PkdFQTreeBase<typename Types::IndexValue, typename Types::Value, Types::BranchingFactor, Types::ValuesPerBranch>;
    using MyType    = PkdFQTree<Types>;

public:

    static constexpr UInt VERSION = 1;
    static constexpr Int Blocks   = Types::Blocks;

    using Base::METADATA;
    using Base::index_size;
    using Base::SegmentsPerBlock;

    using FieldsList = MergeLists<
                typename Base::FieldsList,
                ConstValue<UInt, VERSION>,
                ConstValue<UInt, Blocks>
    >;

    using IndexValue = typename Types::IndexValue;
    using Value      = typename Types::Value;

    using Values = core::StaticVector<IndexValue, Blocks>;

    using Metadata = typename Base::Metadata;

    using InputBuffer   = PackedFSERowOrderInputBuffer<PackedFSERowOrderInputBufferTypes<Value, Blocks>>;
    using InputType     = Values;

    using SizesT = core::StaticVector<Int, Blocks>;

    using ConstPtrsT    = core::StaticVector<const Value*, Blocks>;

    class ReadState {
    protected:
        ConstPtrsT values_;
        Int idx_ = 0;
    public:
        ReadState() {}
        ReadState(const ConstPtrsT& values, Int idx): values_(values), idx_(idx) {}

        ConstPtrsT& values() {return values_;}
        Int& idx() {return idx_;}
        const ConstPtrsT& values() const {return values_;}
        const Int& idx() const {return idx_;}
    };


    class Iterator: public ReadState {
        Int size_;
        Values data_values_;

        using ReadState::idx_;
        using ReadState::values_;

        Int idx_backup_;

    public:
        Iterator() {}
        Iterator(const ConstPtrsT& values, Int idx, Int size):
            ReadState(values, idx),
            size_(size)
        {}

        Int size() const {return size_;}

        bool has_next() const {return idx_ < size_;}

        void next()
        {
            for (Int b = 0; b < Blocks; b++)
            {
                data_values_[b] = values_[b][idx_];
            }

            idx_++;
        }

        const auto& value(Int block) {return data_values_[block];}

        void mark() {
            idx_backup_ = idx_;
        }

        void restore() {
            idx_ = idx_backup_;
        }
    };


    class BlockIterator {
        const Value* values_;
        Int idx_ = 0;

        Int size_;
        Value data_value_;

        Int idx_backup_;
    public:
        BlockIterator() {}
        BlockIterator(const Value* values, Int idx, Int size):
            values_(values),
            idx_(idx),
            size_(size),
            data_value_(),
            idx_backup_()
        {}

        Int size() const {return size_;}

        bool has_next() const {return idx_ < size_;}

        void next()
        {
            for (Int b = 0; b < Blocks; b++)
            {
                data_value_ = values_[idx_];
            }

            idx_++;
        }

        const auto& value() {return data_value_;}

        void mark() {
            idx_backup_ = idx_;
        }

        void restore() {
            idx_ = idx_backup_;
        }
    };


    static Int estimate_block_size(Int tree_capacity, Int density_hi = 1, Int density_lo = 1)
    {
        MEMORIA_V1_ASSERT(density_hi, ==, 1); // data density should not be set for this type of trees
        MEMORIA_V1_ASSERT(density_lo, ==, 1);

        return block_size(tree_capacity);
    }

    void init_tl(Int data_block_size)
    {
        Base::init_tl(data_block_size, Blocks);
    }

    void init(Int capacity = 0)
    {
        Base::init(empty_size(), Blocks * SegmentsPerBlock + 1);

        Metadata* meta = this->template allocate<Metadata>(METADATA);

        meta->size()        = 0;
        meta->max_size()    = capacity;
        meta->index_size()  = MyType::index_size(capacity);

        for (Int block = 0; block < Blocks; block++)
        {
            this->template allocateArrayBySize<IndexValue>(block * SegmentsPerBlock + 1, meta->index_size());
            this->template allocateArrayBySize<Value>(block * SegmentsPerBlock + 2, capacity);
        }
    }


    void init_by_block(Int block_size, Int capacity = 0)
    {
        Base::init(block_size, Blocks * SegmentsPerBlock + 1);

        Metadata* meta = this->template allocate<Metadata>(METADATA);

        meta->size()        = 0;
        meta->max_size()    = capacity;
        meta->index_size()  = MyType::index_size(capacity);

        for (Int block = 0; block < Blocks; block++)
        {
            this->template allocateArrayBySize<IndexValue>(block * SegmentsPerBlock + 1, meta->index_size());
            this->template allocateArrayBySize<Value>(block * SegmentsPerBlock + 2, capacity);
        }
    }

    void init(const SizesT& sizes)
    {
        MyType::init(sizes[0]);
    }

    static Int block_size(Int capacity)
    {
        return Base::block_size(Blocks, capacity);
    }

    static Int packed_block_size(Int tree_capacity)
    {
        return block_size(tree_capacity);
    }


    Int block_size() const
    {
        return Base::block_size();
    }

    Int block_size(const MyType* other) const
    {
        return block_size(this->size() + other->size());
    }




    static Int elements_for(Int block_size)
    {
        return Base::tree_size(Blocks, block_size);
    }

    static Int expected_block_size(Int items_num)
    {
        return block_size(items_num);
    }



    static Int empty_size()
    {
        return block_size(0);
    }

    void reindex() {
        Base::reindex(Blocks);
    }

    void dump_index(std::ostream& out = cout) const {
        Base::dump_index(Blocks, out);
    }

    void dump(std::ostream& out = cout) const {
        TextPageDumper dumper(out);
        generateDataEvents(&dumper);
    }

    bool check_capacity(Int size) const
    {
        MEMORIA_V1_ASSERT_TRUE(size >= 0);

        auto alloc = this->allocator();

        Int total_size          = this->size() + size;
        Int total_block_size    = MyType::block_size(total_size);
        Int my_block_size       = alloc->element_size(this);
        Int delta               = total_block_size - my_block_size;

        return alloc->free_space() >= delta;
    }


    // ================================ Container API =========================================== //

    Values sums(Int from, Int to) const
    {
        Values vals;

        for (Int block = 0; block < Blocks; block++)
        {
            vals[block] = this->sum(block, from, to);
        }

        return vals;
    }


    Values sums() const
    {
        Values vals;

        for (Int block = 0; block < Blocks; block++)
        {
            vals[block] = this->sum(block);
        }

        return vals;
    }

    template <typename T>
    void sums(Int from, Int to, core::StaticVector<T, Blocks>& values) const
    {
        values += this->sums(from, to);
    }

    void sums(Values& values) const
    {
        values += this->sums();
    }


    void sums(Int idx, Values& values) const
    {
        addKeys(idx, values);
    }


    template <typename T>
    void max(StaticVector<T, Blocks>& accum) const
    {
        for (Int block = 0; block < Blocks; block++)
        {
            accum[block] = this->sum(block);
        }
    }


    template <Int Offset, Int Size, typename T, template <typename, Int> class BranchNodeEntryItem>
    void max(BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset <= Size - Blocks, "Invalid balanced tree structure");

        for (Int block = 0; block < Blocks; block++)
        {
            accum[block + Offset] = this->sum(block);
        }
    }




    template <typename... Args>
    auto sum(Args&&... args) const -> decltype(Base::sum(std::forward<Args>(args)...)) {
        return Base::sum(std::forward<Args>(args)...);
    }

    template <Int Offset, Int Size, typename T, template <typename, Int> class BranchNodeEntryItem>
    void sum(BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset <= Size - Blocks, "Invalid balanced tree structure");

        for (Int block = 0; block < Blocks; block++)
        {
            accum[block + Offset] += this->sum(block);
        }
    }

    template <Int Offset, Int Size, typename T, template <typename, Int> class BranchNodeEntryItem>
    void sum(Int start, Int end, BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset <= Size - Blocks, "Invalid balanced tree structure");

        for (Int block = 0; block < Blocks; block++)
        {
            accum[block + Offset] += this->sum(block, start, end);
        }
    }

    template <Int Offset, Int Size, typename T, template <typename, Int> class BranchNodeEntryItem>
    void sum(Int idx, BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset <= Size - Blocks, "Invalid balanced tree structure");

        for (Int block = 0; block < Blocks; block++)
        {
            accum[block + Offset] += this->value(block, idx);
        }
    }

    template <Int Offset, Int Size, typename T, template <typename, Int> class BranchNodeEntryItem>
    void sub(Int idx, BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset <= Size - Blocks, "Invalid balanced tree structure");

        for (Int block = 0; block < Blocks; block++)
        {
            accum[block + Offset] -= this->value(block, idx);
        }
    }


    template <Int Offset, Int From, Int To, typename T, template <typename, Int, Int> class BranchNodeEntryItem>
    void sum(Int start, Int end, BranchNodeEntryItem<T, From, To>& accum) const
    {
        for (Int block = 0; block < Blocks; block++)
        {
            accum[block + Offset] += this->sum(block, start, end);
        }
    }


    template <typename T>
    void _add(Int block, T& value) const
    {
        value += this->sum(block);
    }

    template <typename T>
    void _add(Int block, Int end, T& value) const
    {
        value += this->sum(block, end);
    }

    template <typename T>
    void _add(Int block, Int start, Int end, T& value) const
    {
        value += this->sum(block, start, end);
    }



    template <typename T>
    void _sub(Int block, T& value) const
    {
        value -= this->sum(block);
    }

    template <typename T>
    void _sub(Int block, Int end, T& value) const
    {
        value -= this->sum(block, end);
    }

    template <typename T>
    void _sub(Int block, Int start, Int end, T& value) const
    {
        value -= this->sum(block, start, end);
    }

    Values get_values(Int idx) const
    {
        Values v;

        for (Int i = 0; i < Blocks; i++)
        {
            v[i] = this->value(i, idx);
        }

        return v;
    }

    Value get_values(Int idx, Int index) const
    {
        return this->value(index, idx);
    }

    Value getValue(Int index, Int idx) const
    {
        return this->value(index, idx);
    }


    template <typename IOBuffer>
    bool readTo(ReadState& state, IOBuffer& buffer) const
    {
        for (Int b = 0; b < Blocks; b++)
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
    void read(Int block, Int start, Int end, Fn&& fn) const
    {
        MEMORIA_V1_ASSERT(end, <=, this->size());
        MEMORIA_V1_ASSERT(start, >=, 0);
        MEMORIA_V1_ASSERT(start, <=, end);

        auto values = this->values(block);

        for (Int c = start; c < end; c++)
        {
            fn(block, values[c]);
            fn.next();
        }
    }



    template <typename Fn>
    void read(Int start, Int end, Fn&& fn) const
    {
        MEMORIA_V1_ASSERT(end, <=, this->size());
        MEMORIA_V1_ASSERT(start, >=, 0);
        MEMORIA_V1_ASSERT(start, <=, end);

        const Value* vals[Blocks];

        for (Int b = 0; b  < Blocks; b++) {
            vals[b] = this->values(b);
        }

        for (Int c = start; c < end; c++)
        {
            for (Int b = 0; b < Blocks; b++) {
                fn(b, vals[b][c]);
            }

            fn.next();
        }
    }





    // ========================================= Insert/Remove/Resize ============================================== //

protected:
    void resize(Metadata* meta, Int size)
    {
        Int new_data_size  = meta->max_size() + size;
        Int new_index_size = MyType::index_size(new_data_size);

        for (Int block = 0; block < Blocks; block++)
        {
            Base::resizeBlock(SegmentsPerBlock * block + 1, new_index_size * sizeof(IndexValue));
            Base::resizeBlock(SegmentsPerBlock * block + 2, new_data_size * sizeof(Value));
        }

        meta->max_size()    += size;
        meta->index_size()  = new_index_size;
    }

    void insertSpace(Int idx, Int room_length)
    {
        auto meta = this->metadata();

        Int capacity  = meta->capacity();

        if (capacity < room_length)
        {
            resize(meta, room_length - capacity);
        }

        for (Int block = 0; block < Blocks; block++)
        {
            auto* values = this->values(block);

            CopyBuffer(
                    values + idx,
                    values + idx + room_length,
                    meta->size() - idx
            );

            for (Int c = idx; c < idx + room_length; c++) {
                values[c] = 0;
            }
        }

        meta->size() += room_length;
    }



    void copyTo(MyType* other, Int copy_from, Int count, Int copy_to) const
    {
        MEMORIA_V1_ASSERT_TRUE(copy_from >= 0);
        MEMORIA_V1_ASSERT_TRUE(count >= 0);

        for (Int block = 0; block < Blocks; block++)
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
    void splitTo(MyType* other, Int idx)
    {
        Int total = this->size() - idx;
        other->insertSpace(0, total);

        copyTo(other, idx, total, 0);
        other->reindex();

        removeSpace(idx, this->size());
        reindex();
    }

    void mergeWith(MyType* other)
    {
        Int my_size     = this->size();
        Int other_size  = other->size();

        other->insertSpace(other_size, my_size);

        copyTo(other, 0, my_size, other_size);

        removeSpace(0, my_size);

        reindex();
        other->reindex();
    }

    template <typename TreeType>
    void transferDataTo(TreeType* other) const
    {
        other->insertSpace(0, this->size());

        Int size = this->size();

        for (Int block = 0; block < Blocks; block++)
        {
            const auto* my_values   = this->values(block);
            auto* other_values      = other->values(block);

            for (Int c = 0; c < size; c++)
            {
                other_values[c] = my_values[c];
            }
        }

        other->reindex();
    }


    void removeSpace(Int start, Int end)
    {
        remove(start, end);
    }

    void remove(Int start, Int end)
    {
        auto meta = this->metadata();

        Int room_length = end - start;
        Int size = meta->size();

        for (Int block = Blocks - 1; block >= 0; block--)
        {
            auto values = this->values(block);

            CopyBuffer(
                    values + end,
                    values + start,
                    size - end
            );

            for (Int c = start + size - end; c < size; c++)
            {
                values[c] = 0;
            }
        }

        meta->size() -= room_length;

        resize(meta, -room_length);

        reindex();
    }


    void insert(Int idx, Int size, std::function<Values (Int)> provider, bool reindex = true)
    {
        insertSpace(idx, size);

        typename Base::Value* values[Blocks];
        for (Int block  = 0; block < Blocks; block++)
        {
            values[block] = this->values(block);
        }

        for (Int c = idx; c < idx + size; c++)
        {
            Values vals = provider(c - idx);

            for (Int block = 0; block < Blocks; block++)
            {
                values[block][c] = vals[block];
            }
        }

        if (reindex) {
            this->reindex();
        }
    }

    template <typename T>
    void insert(Int idx, const core::StaticVector<T, Blocks>& values)
    {
        insertSpace(idx, 1);
        setValues(idx, values);
    }






    template <typename Adaptor>
    void _insert(Int pos, Int size, Adaptor&& adaptor)
    {
        populate(pos, size, std::forward<Adaptor>(adaptor));
        reindex();
    }

    template <typename Adaptor>
    void populate(Int pos, Int size, Adaptor&& adaptor)
    {
        insertSpace(pos, size);

        for (Int c = 0; c < size; c++)
        {
            for (Int block = 0; block < Blocks; block++)
            {
                const auto& item = adaptor(block, c);
                this->value(block, c + pos) = item;
            }
        }
    }


    template <typename Iter>
    void populate_from_iterator(Int start, Int length, Iter&& iter)
    {
        MEMORIA_V1_ASSERT(start, >=, 0);
        MEMORIA_V1_ASSERT(start, <=, this->size());

        MEMORIA_V1_ASSERT(length, >=, 0);

        insertSpace(start, length);

        for (Int c = 0; c < length; c++)
        {
            iter.next();
            for (Int block = 0; block < Blocks; block++)
            {
                this->value(block, c + start) = iter.value(block);
            }
        }
    }


    ReadState positions(Int idx) const
    {
        ReadState state;

        state.idx() = idx;

        for (Int b = 0; b < Blocks; b++) {
            state.values()[b] = this->values(b);
        }

        return state;
    }

    Iterator iterator(Int idx) const
    {
        ConstPtrsT ptrs;

        for (Int b = 0; b < Blocks; b++) {
            ptrs[b] = this->values(b);
        }

        return Iterator(ptrs, idx, this->size());
    }

    BlockIterator iterator(Int block, Int idx) const
    {
        return BlockIterator(this->values(block), idx, this->size());
    }


    SizesT insert_buffer(SizesT at, const InputBuffer* buffer, SizesT starts, SizesT ends, Int inserted)
    {
        auto buffer_values = buffer->values() + starts[0] * Blocks;

        _insert(at[0], inserted, [&](Int block, Int idx) -> const auto& {
            return buffer_values[idx * Blocks + block];
        });

        return at + SizesT(inserted);
    }

    void insert_buffer(Int at, const InputBuffer* buffer, Int start, Int inserted)
    {
        auto buffer_values = buffer->values() + start * Blocks;

        _insert(at, inserted, [&](Int block, Int idx) -> const auto& {
            return buffer_values[idx * Blocks + block];
        });
    }

    template <typename T>
    void append(const StaticVector<T, Blocks>& values)
    {
        auto meta = this->metadata();

        for (Int b = 0; b < Blocks; b++)
        {
            this->values(b)[meta->size()] = values[b];
        }

        meta->size()++;
    }

    template <typename T>
    void update(Int idx, const core::StaticVector<T, Blocks>& values)
    {
        setValues(idx, values);
    }


    template <Int Offset, Int Size, typename T1, typename T2, template <typename, Int> class BranchNodeEntryItem>
    void _insert(Int idx, const core::StaticVector<T1, Blocks>& values, BranchNodeEntryItem<T2, Size>& accum)
    {
        insert(idx, values);

        sum<Offset>(idx, accum);
    }

    template <Int Offset, Int Size, typename AccessorFn, typename T2, template <typename, Int> class BranchNodeEntryItem>
    void _insert_b(Int idx, BranchNodeEntryItem<T2, Size>& accum, AccessorFn&& values)
    {
        insertSpace(idx, 1);

        for (Int b = 0; b < Blocks; b++) {
            this->values(b)[idx] = values(b);
        }

        reindex();

        sum<Offset>(this->size() - 1, accum);
    }


    template <Int Offset, Int Size, typename T1, typename T2, template <typename, Int> class BranchNodeEntryItem>
    void _update(Int idx, const core::StaticVector<T1, Blocks>& values, BranchNodeEntryItem<T2, Size>& accum)
    {
        sub<Offset>(idx, accum);

        update(idx, values);

        sum<Offset>(idx, accum);
    }


    template <Int Offset, Int Size, typename T1, typename T2, typename I, template <typename, Int> class BranchNodeEntryItem>
    void _update(Int idx, const std::pair<T1, I>& values, BranchNodeEntryItem<T2, Size>& accum)
    {
        sub<Offset>(idx, accum);

        this->setValue(values.first, idx, values.second);

        sum<Offset>(idx, accum);
    }

    template <Int Offset, Int Size, typename T, template <typename, Int> class BranchNodeEntryItem>
    void _remove(Int idx, BranchNodeEntryItem<T, Size>& accum)
    {
        sub<Offset>(idx, accum);
        remove(idx, idx + 1);
    }


    BigInt setValue(Int block, Int idx, const Value& value)
    {
        // FIXME: Why do we skip setting if value is zero
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
    void setValues(Int idx, const core::StaticVector<T, Blocks>& values)
    {
        for (Int b = 0; b < Blocks; b++) {
            this->values(b)[idx] = values[b];
        }

        reindex();
    }

    template <typename T>
    void addValues(Int idx, const core::StaticVector<T, Blocks>& values)
    {
        for (Int b = 0; b < Blocks; b++) {
            this->values(b)[idx] += values[b];
        }

        reindex();
    }


    void addValue(Int block, Int idx, const Value& value)
    {
        if (value != 0)
        {
            this->value(block, idx) += value;
        }

        reindex();
    }

    template <typename T, Int Indexes>
    void addValues(Int idx, Int from, Int size, const core::StaticVector<T, Indexes>& values)
    {
        for (Int block = 0; block < size; block++)
        {
            this->value(block, idx) += values[block + from];
        }

        reindex();
    }




    void check() const {}

    void clear()
    {
        init();

        if (Base::has_allocator())
        {
            auto alloc = this->allocator();
            Int empty_size = MyType::empty_size();
            alloc->resizeBlock(this, empty_size);
        }
    }

    void clear(Int start, Int end)
    {
        for (Int block = 0; block < Blocks; block++)
        {
            auto values = this->values(block);

            for (Int c = start; c < end; c++)
            {
                values[c] = 0;
            }
        }
    }

    void generateDataEvents(IPageDataEventHandler* handler) const
    {
//        Base::generateDataEvents(handler);

        handler->startStruct();
        handler->startGroup("FSQ_TREE");

        auto meta = this->metadata();

        handler->value("SIZE",          &meta->size());
        handler->value("MAX_SIZE",      &meta->max_size());
        handler->value("INDEX_SIZE",    &meta->index_size());

        handler->startGroup("INDEXES", meta->index_size());

        auto index_size = meta->index_size();

        const IndexValue* index[Blocks];

        for (Int b = 0; b < Blocks; b++) {
            index[b] = this->index(b);
        }

        for (Int c = 0; c < index_size; c++)
        {
            handler->value("INDEX", PageValueProviderFactory::provider(Blocks, [&](Int idx) {
                return index[idx][c];
            }));
        }

        handler->endGroup();

        handler->startGroup("DATA", meta->size());



        const Value* values[Blocks];

        for (Int b = 0; b < Blocks; b++) {
            values[b] = this->values(b);
        }

        for (Int c = 0; c < meta->size() ; c++)
        {
            handler->value("TREE_ITEM", PageValueProviderFactory::provider(false, Blocks, [&](Int idx) {
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

        FieldFactory<Int>::serialize(buf, meta->size());
        FieldFactory<Int>::serialize(buf, meta->max_size());
        FieldFactory<Int>::serialize(buf, meta->index_size());

        for (Int b = 0; b < Blocks; b++)
        {
            FieldFactory<IndexValue>::serialize(buf, this->index(b), meta->index_size());
            FieldFactory<Value>::serialize(buf, this->values(b), meta->size());
        }
    }

    void deserialize(DeserializationData& buf)
    {
        Base::deserialize(buf);

        Metadata* meta = this->metadata();

        FieldFactory<Int>::deserialize(buf, meta->size());
        FieldFactory<Int>::deserialize(buf, meta->max_size());
        FieldFactory<Int>::deserialize(buf, meta->index_size());

        for (Int b = 0; b < Blocks; b++)
        {
            FieldFactory<IndexValue>::deserialize(buf, this->index(b), meta->index_size());
            FieldFactory<Value>::deserialize(buf, this->values(b), meta->size());
        }
    }
};

template <typename Types>
struct PkdStructSizeType<PkdFQTree<Types>> {
    static const PackedSizeType Value = PackedSizeType::FIXED;
};


template <typename Types>
struct StructSizeProvider<PkdFQTree<Types>> {
    static const Int Value = Types::Blocks;
};

template <typename Types>
struct IndexesSize<PkdFQTree<Types>> {
    static const Int Value = Types::Blocks;
};


}}
