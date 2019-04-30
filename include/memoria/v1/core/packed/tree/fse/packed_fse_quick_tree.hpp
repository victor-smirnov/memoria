
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

#include <memoria/v1/core/tools/static_array.hpp>
#include <memoria/v1/profiles/common/block_operations.hpp>

#include <memoria/v1/core/iovector/io_substream_col_array_fixed_size.hpp>
#include <memoria/v1/core/iovector/io_substream_col_array_fixed_size_view.hpp>

namespace memoria {
namespace v1 {

template <typename IndexValueT, int32_t kBlocks, typename ValueT = IndexValueT, int32_t kBranchingFactor = PackedTreeBranchingFactor, int32_t kValuesPerBranch = PackedTreeBranchingFactor>
struct PkdFQTreeTypes {
    using IndexValue    = IndexValueT;
    using Value         = ValueT;

    static constexpr int32_t Blocks = kBlocks;
    static constexpr int32_t BranchingFactor = kBranchingFactor;
    static constexpr int32_t ValuesPerBranch = kValuesPerBranch;
};

template <typename Types> class PkdFQTree;


template <typename IndexValueT, int32_t kBlocks = 1, typename ValueT = IndexValueT, int32_t kBranchingFactor = PackedTreeBranchingFactor, int32_t kValuesPerBranch = PackedTreeBranchingFactor>
using PkdFQTreeT = PkdFQTree<PkdFQTreeTypes<IndexValueT, kBlocks, ValueT, kBranchingFactor, kValuesPerBranch>>;


using core::StaticVector;

template <typename Types>
class PkdFQTree: public PkdFQTreeBase<typename Types::IndexValue, typename Types::Value, Types::BranchingFactor, Types::ValuesPerBranch> {

    using Base      = PkdFQTreeBase<typename Types::IndexValue, typename Types::Value, Types::BranchingFactor, Types::ValuesPerBranch>;
    using MyType    = PkdFQTree<Types>;

public:

    static constexpr uint32_t VERSION = 1;
    static constexpr int32_t Blocks   = Types::Blocks;

    using Base::METADATA;
    using Base::index_size;
    using Base::SegmentsPerBlock;

    using FieldsList = MergeLists<
                typename Base::FieldsList,
                ConstValue<uint32_t, VERSION>,
                ConstValue<uint32_t, Blocks>
    >;

    using IndexValue = typename Types::IndexValue;
    using Value      = typename Types::Value;

    using Values     = core::StaticVector<IndexValue, Blocks>;
    using DataValues = core::StaticVector<Value, Blocks>;

    using Metadata = typename Base::Metadata;

    using GrowableIOSubstream = io::IOColumnwiseFixedSizeArraySubstreamImpl<Value, Blocks>;
    using IOSubstreamView     = io::IOColumnwiseFixedSizeArraySubstreamViewImpl<Value, Blocks>;

    using SizesT = core::StaticVector<int32_t, Blocks>;

    using ConstPtrsT = core::StaticVector<const Value*, Blocks>;

    class ReadState {
    protected:
        ConstPtrsT values_;
        int32_t idx_ = 0;
    public:
        ReadState() {}
        ReadState(const ConstPtrsT& values, int32_t idx): values_(values), idx_(idx) {}

        ConstPtrsT& values() {return values_;}
        int32_t& local_pos() {return idx_;}
        const ConstPtrsT& values() const {return values_;}
        const int32_t& local_pos() const {return idx_;}
    };


    class Iterator: public ReadState {
        int32_t size_;
        Values data_values_;

        using ReadState::idx_;
        using ReadState::values_;

        int32_t idx_backup_;

    public:
        Iterator() {}
        Iterator(const ConstPtrsT& values, int32_t idx, int32_t size):
            ReadState(values, idx),
            size_(size)
        {}

        int32_t size() const {return size_;}

        bool has_next() const {return idx_ < size_;}

        void next()
        {
            for (int32_t b = 0; b < Blocks; b++)
            {
                data_values_[b] = values_[b][idx_];
            }

            idx_++;
        }

        const auto& value(int32_t block) {return data_values_[block];}

        void mark() {
            idx_backup_ = idx_;
        }

        void restore() {
            idx_ = idx_backup_;
        }
    };


    class BlockIterator {
        const Value* values_;
        int32_t idx_ = 0;

        int32_t size_;
        Value data_value_;

        int32_t idx_backup_;
    public:
        BlockIterator() {}
        BlockIterator(const Value* values, int32_t idx, int32_t size):
            values_(values),
            idx_(idx),
            size_(size),
            data_value_(),
            idx_backup_()
        {}

        int32_t size() const {return size_;}

        bool has_next() const {return idx_ < size_;}

        void next()
        {
            for (int32_t b = 0; b < Blocks; b++)
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


    static int32_t estimate_block_size(int32_t tree_capacity, int32_t density_hi = 1, int32_t density_lo = 1)
    {
        MEMORIA_V1_ASSERT(density_hi, ==, 1); // data density should not be set for this type of trees
        MEMORIA_V1_ASSERT(density_lo, ==, 1);

        return block_size(tree_capacity);
    }

    OpStatus init_tl(int32_t data_block_size)
    {
        return Base::init_tl(data_block_size, Blocks);
    }

    OpStatus init(int32_t capacity = 0)
    {
        if(isFail(Base::init(empty_size(), Blocks * SegmentsPerBlock + 1))) {
            return OpStatus::FAIL;
        }

        Metadata* meta = this->template allocate<Metadata>(METADATA);

        if(isFail(meta)) {
            return OpStatus::FAIL;
        }

        meta->size()        = 0;
        meta->max_size()    = capacity;
        meta->index_size()  = MyType::index_size(capacity);

        for (int32_t block = 0; block < Blocks; block++)
        {
            if(isFail(this->template allocateArrayBySize<IndexValue>(block * SegmentsPerBlock + 1, meta->index_size()))) {
                return OpStatus::FAIL;
            }

            if(isFail(this->template allocateArrayBySize<Value>(block * SegmentsPerBlock + 2, capacity))) {
                return OpStatus::FAIL;
            }
        }

        return OpStatus::OK;
    }

    OpStatus init_bs(int32_t block_size)
    {
        return init_by_block(block_size, elements_for(block_size));
    }

    OpStatus init_by_block(int32_t block_size, int32_t capacity = 0)
    {
        if(isFail(Base::init(block_size, Blocks * SegmentsPerBlock + 1))) {
            return OpStatus::FAIL;
        }

        Metadata* meta = this->template allocate<Metadata>(METADATA);
        if(isFail(meta)) {
            return OpStatus::FAIL;
        }

        meta->size()        = 0;
        meta->max_size()    = capacity;
        meta->index_size()  = MyType::index_size(capacity);

        for (int32_t block = 0; block < Blocks; block++)
        {
            if(isFail(this->template allocateArrayBySize<IndexValue>(block * SegmentsPerBlock + 1, meta->index_size()))) {
                return OpStatus::FAIL;
            }

            if(isFail(this->template allocateArrayBySize<Value>(block * SegmentsPerBlock + 2, capacity))) {
                return OpStatus::FAIL;
            }
        }

        return OpStatus::OK;
    }

    OpStatus init(const SizesT& sizes)
    {
        return MyType::init(sizes[0]);
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



    OpStatus reindex() {
        Base::reindex(Blocks);
        return OpStatus::OK;
    }

    void dump_index(std::ostream& out = std::cout) const {
        Base::dump_index(Blocks, out);
    }

    void dump(std::ostream& out = std::cout) const {
        TextBlockDumper dumper(out);
        generateDataEvents(&dumper);
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


    // ================================ Container API =========================================== //

    Values sums(int32_t from, int32_t to) const
    {
        Values vals;

        for (int32_t block = 0; block < Blocks; block++)
        {
            vals[block] = this->sum(block, from, to);
        }

        return vals;
    }


    Values sums() const
    {
        Values vals;

        for (int32_t block = 0; block < Blocks; block++)
        {
            vals[block] = this->sum(block);
        }

        return vals;
    }

    template <typename T>
    void sums(int32_t from, int32_t to, core::StaticVector<T, Blocks>& values) const
    {
        values += this->sums(from, to);
    }

    void sums(Values& values) const
    {
        values += this->sums();
    }


    void sums(int32_t idx, Values& values) const
    {
        addKeys(idx, values);
    }


    template <typename T>
    void max(StaticVector<T, Blocks>& accum) const
    {
        for (int32_t block = 0; block < Blocks; block++)
        {
            accum[block] = this->sum(block);
        }
    }


    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    void max(BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset <= Size - Blocks, "Invalid balanced tree structure");

        for (int32_t block = 0; block < Blocks; block++)
        {
            accum[block + Offset] = this->sum(block);
        }
    }




    template <typename... Args>
    auto sum(Args&&... args) const -> decltype(Base::sum(std::forward<Args>(args)...)) {
        return Base::sum(std::forward<Args>(args)...);
    }

    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    void sum(BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset <= Size - Blocks, "Invalid balanced tree structure");

        for (int32_t block = 0; block < Blocks; block++)
        {
            accum[block + Offset] += this->sum(block);
        }
    }

    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    void sum(int32_t start, int32_t end, BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset <= Size - Blocks, "Invalid balanced tree structure");

        for (int32_t block = 0; block < Blocks; block++)
        {
            accum[block + Offset] += this->sum(block, start, end);
        }
    }

    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    void sum(int32_t idx, BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset <= Size - Blocks, "Invalid balanced tree structure");

        for (int32_t block = 0; block < Blocks; block++)
        {
            accum[block + Offset] += this->value(block, idx);
        }
    }

    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    void sub(int32_t idx, BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset <= Size - Blocks, "Invalid balanced tree structure");

        for (int32_t block = 0; block < Blocks; block++)
        {
            accum[block + Offset] -= this->value(block, idx);
        }
    }


    template <int32_t Offset, int32_t From, int32_t To, typename T, template <typename, int32_t, int32_t> class BranchNodeEntryItem>
    void sum(int32_t start, int32_t end, BranchNodeEntryItem<T, From, To>& accum) const
    {
        for (int32_t block = 0; block < Blocks; block++)
        {
            accum[block + Offset] += this->sum(block, start, end);
        }
    }


    template <typename T>
    void _add(int32_t block, T& value) const
    {
        value += this->sum(block);
    }

    template <typename T>
    void _add(int32_t block, int32_t end, T& value) const
    {
        value += this->sum(block, end);
    }

    template <typename T>
    void _add(int32_t block, int32_t start, int32_t end, T& value) const
    {
        value += this->sum(block, start, end);
    }



    template <typename T>
    void _sub(int32_t block, T& value) const
    {
        value -= this->sum(block);
    }

    template <typename T>
    void _sub(int32_t block, int32_t end, T& value) const
    {
        value -= this->sum(block, end);
    }

    template <typename T>
    void _sub(int32_t block, int32_t start, int32_t end, T& value) const
    {
        value -= this->sum(block, start, end);
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

//    int32_t findNZLE(int32_t block, int32_t start) const
//    {
//    	MEMORIA_V1_ASSERT(block, <, (int32_t)Blocks);
//    	return this->findNZLE_(block, Blocks, start);
//    }






    template <typename Fn>
    void read(int32_t block, int32_t start, int32_t end, Fn&& fn) const
    {
        MEMORIA_V1_ASSERT(end, <=, this->size());
        MEMORIA_V1_ASSERT(start, >=, 0);
        MEMORIA_V1_ASSERT(start, <=, end);

        auto values = this->values(block);

        for (int32_t c = start; c < end; c++)
        {
            fn(block, values[c]);
            fn.next();
        }
    }



    template <typename Fn>
    void read(int32_t start, int32_t end, Fn&& fn) const
    {
        MEMORIA_V1_ASSERT(end, <=, this->size());
        MEMORIA_V1_ASSERT(start, >=, 0);
        MEMORIA_V1_ASSERT(start, <=, end);

        const Value* vals[Blocks];

        for (int32_t b = 0; b  < Blocks; b++) {
            vals[b] = this->values(b);
        }

        for (int32_t c = start; c < end; c++)
        {
            for (int32_t b = 0; b < Blocks; b++) {
                fn(b, vals[b][c]);
            }

            fn.next();
        }
    }





    // ========================================= Insert/Remove/Resize ============================================== //

protected:
    OpStatus resize(Metadata* meta, int32_t size)
    {
        int32_t new_data_size  = meta->max_size() + size;
        int32_t new_index_size = MyType::index_size(new_data_size);

        for (int32_t block = 0; block < Blocks; block++)
        {
            if (isFail(Base::resizeBlock(SegmentsPerBlock * block + 1, new_index_size * sizeof(IndexValue)))) {
                return OpStatus::FAIL;
            }

            if (isFail(Base::resizeBlock(SegmentsPerBlock * block + 2, new_data_size * sizeof(Value)))) {
                return OpStatus::FAIL;
            }
        }

        meta->max_size()    += size;
        meta->index_size()  = new_index_size;

        return OpStatus::OK;
    }

    OpStatus insertSpace(int32_t idx, int32_t room_length)
    {
        auto meta = this->metadata();

        int32_t capacity  = meta->capacity();

        if (capacity < room_length)
        {
            if (isFail(resize(meta, room_length - capacity))) {
                return OpStatus::FAIL;
            }
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
                values[c] = Value{};
            }
        }

        meta->size() += room_length;

        return OpStatus::OK;
    }



    OpStatus copyTo(MyType* other, int32_t copy_from, int32_t count, int32_t copy_to) const
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

        return OpStatus::OK;
    }

public:
    OpStatus splitTo(MyType* other, int32_t idx)
    {
        int32_t total = this->size() - idx;
        if(isFail(other->insertSpace(0, total))) {
            return OpStatus::FAIL;
        }

        if(isFail(copyTo(other, idx, total, 0))) {
            return OpStatus::FAIL;
        }

        if(isFail(other->reindex())) {
            return OpStatus::FAIL;
        }

        if(isFail(removeSpace(idx, this->size()))) {
            return OpStatus::FAIL;
        }

        return reindex();
    }

    OpStatus mergeWith(MyType* other)
    {
        int32_t my_size     = this->size();
        int32_t other_size  = other->size();

        if(isFail(other->insertSpace(other_size, my_size))) {
            return OpStatus::FAIL;
        }

        if(isFail(copyTo(other, 0, my_size, other_size))) {
            return OpStatus::FAIL;
        }

        if(isFail(removeSpace(0, my_size))) {
            return OpStatus::FAIL;
        }

        if(isFail(reindex())) {
            return OpStatus::FAIL;
        }

        return other->reindex();
    }

    template <typename TreeType>
    OpStatus transferDataTo(TreeType* other) const
    {
        if(isFail(other->insertSpace(0, this->size()))) {
            return OpStatus::FAIL;
        }

        int32_t size = this->size();

        for (int32_t block = 0; block < Blocks; block++)
        {
            const auto* my_values   = this->values(block);
            auto* other_values      = other->values(block);

            for (int32_t c = 0; c < size; c++)
            {
                other_values[c] = my_values[c];
            }
        }

        return other->reindex();
    }


    OpStatus removeSpace(int32_t start, int32_t end)
    {
        return remove(start, end);
    }

    OpStatus remove(int32_t start, int32_t end)
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
                values[c] = 0;
            }
        }

        meta->size() -= room_length;

        if(isFail(resize(meta, -room_length))) {
            return OpStatus::FAIL;
        }

        return reindex();
    }


    OpStatus insert(int32_t idx, int32_t size, std::function<Values (int32_t)> provider, bool reindex = true)
    {
        if(isFail(insertSpace(idx, size))) {
            return OpStatus::FAIL;
        }

        typename Base::Value* values[Blocks];
        for (int32_t block  = 0; block < Blocks; block++)
        {
            values[block] = this->values(block);
        }

        for (int32_t c = idx; c < idx + size; c++)
        {
            Values vals = provider(c - idx);

            for (int32_t block = 0; block < Blocks; block++)
            {
                values[block][c] = vals[block];
            }
        }

        if (reindex) {
            return this->reindex();
        }

        return OpStatus::OK;
    }

    template <typename T>
    OpStatus insert(int32_t idx, const core::StaticVector<T, Blocks>& values)
    {
        if (isFail(insertSpace(idx, 1))) {
            return OpStatus::FAIL;
        }

        return setValues(idx, values);
    }


    template <typename Adaptor>
    OpStatus _insert(int32_t pos, int32_t size, Adaptor&& adaptor)
    {
        if(isFail(populate(pos, size, std::forward<Adaptor>(adaptor)))) {
            return OpStatus::FAIL;
        }

        return reindex();
    }

    template <typename Adaptor>
    OpStatus populate(int32_t pos, int32_t size, Adaptor&& adaptor)
    {
        if (isFail(insertSpace(pos, size))) {
            return OpStatus::FAIL;
        }

        for (int32_t c = 0; c < size; c++)
        {
            for (int32_t block = 0; block < Blocks; block++)
            {
                const auto& item = adaptor(block, c);
                this->value(block, c + pos) = item;
            }
        }

        return OpStatus::OK;
    }


    template <typename Iter>
    OpStatus populate_from_iterator(int32_t start, int32_t length, Iter&& iter)
    {
        MEMORIA_V1_ASSERT(start, >=, 0);
        MEMORIA_V1_ASSERT(start, <=, this->size());

        MEMORIA_V1_ASSERT(length, >=, 0);

        if(isFail(insertSpace(start, length))) {
            return OpStatus::FAIL;
        }

        for (int32_t c = 0; c < length; c++)
        {
            iter.next();
            for (int32_t block = 0; block < Blocks; block++)
            {
                this->value(block, c + start) = iter.value(block);
            }
        }

        return OpStatus::OK;
    }


    ReadState positions(int32_t idx) const
    {
        ReadState state;

        state.local_pos() = idx;

        for (int32_t b = 0; b < Blocks; b++) {
            state.values()[b] = this->values(b);
        }

        return state;
    }

    Iterator iterator(int32_t idx) const
    {
        ConstPtrsT ptrs;

        for (int32_t b = 0; b < Blocks; b++) {
            ptrs[b] = this->values(b);
        }

        return Iterator(ptrs, idx, this->size());
    }

    BlockIterator iterator(int32_t block, int32_t idx) const
    {
        return BlockIterator(this->values(block), idx, this->size());
    }




    OpStatus insert_io_substream(int32_t at, io::IOSubstream& substream, int32_t start, int32_t inserted)
    {
        io::IOColumnwiseFixedSizeArraySubstream<Value>& buffer = io::substream_cast<io::IOColumnwiseFixedSizeArraySubstream<Value>>(substream);

        if (isFail(insertSpace(at, inserted))) {
            return OpStatus::FAIL;
        }

        for (int32_t block = 0; block < Blocks; block++)
        {
            auto buffer_values = buffer.select(block, start);
            CopyBuffer(buffer_values, this->values(block) + at, inserted);
        }

        return OpStatus::OK;
    }

    void configure_io_substream(io::IOSubstream& substream)
    {
        auto& view = io::substream_cast<IOSubstreamView>(substream);

        io::FixedSizeArrayColumnMetadata<Value> columns[Blocks]{};

        for (int32_t blk = 0; blk < Blocks; blk++)
        {
            columns[blk].data_buffer = this->values(blk);
            columns[blk].size = this->size();
            columns[blk].capacity = columns[blk].size;
        }

        view.configure(columns);
    }

    template <typename T>
    OpStatus append(const StaticVector<T, Blocks>& values)
    {
        auto meta = this->metadata();

        for (int32_t b = 0; b < Blocks; b++)
        {
            this->values(b)[meta->size()] = values[b];
        }

        meta->size()++;

        return OpStatus::OK;
    }

    template <typename T>
    OpStatus update(int32_t idx, const core::StaticVector<T, Blocks>& values)
    {
        return setValues(idx, values);
    }


    template <int32_t Offset, int32_t Size, typename T1, typename T2, template <typename, int32_t> class BranchNodeEntryItem>
    OpStatus _insert(int32_t idx, const core::StaticVector<T1, Blocks>& values, BranchNodeEntryItem<T2, Size>& accum)
    {
        if(isFail(insert(idx, values))) {
            return OpStatus::FAIL;
        }

        sum<Offset>(idx, accum);

        return OpStatus::OK;
    }

    template <int32_t Offset, int32_t Size, typename AccessorFn, typename T2, template <typename, int32_t> class BranchNodeEntryItem>
    OpStatus _insert_b(int32_t idx, BranchNodeEntryItem<T2, Size>& accum, AccessorFn&& values)
    {
        if(isFail(insertSpace(idx, 1))) {
            return OpStatus::FAIL;
        }

        for (int32_t b = 0; b < Blocks; b++) {
            this->values(b)[idx] = values(b);
        }

        if(isFail(reindex())) {
            return OpStatus::FAIL;
        }

        sum<Offset>(this->size() - 1, accum);

        return OpStatus::OK;
    }

    template <int32_t Offset, int32_t Size, typename AccessorFn, typename T2, template <typename, int32_t> class BranchNodeEntryItem>
    OpStatus _update_b(int32_t idx, BranchNodeEntryItem<T2, Size>& accum, AccessorFn&& values)
    {
        for (int32_t b = 0; b < Blocks; b++)
        {
            this->values(b)[idx] = values(b);
        }

        if(isFail(reindex())) {
            return OpStatus::FAIL;
        }

        //sum<Offset>(idx, accum);

        return OpStatus::OK;
    }


    template <int32_t Offset, int32_t Size, typename T1, typename T2, template <typename, int32_t> class BranchNodeEntryItem>
    OpStatus _update(int32_t idx, const core::StaticVector<T1, Blocks>& values, BranchNodeEntryItem<T2, Size>& accum)
    {
        sub<Offset>(idx, accum);

        update(idx, values);

        sum<Offset>(idx, accum);

        return OpStatus::OK;
    }


    template <int32_t Offset, int32_t Size, typename T1, typename T2, typename I, template <typename, int32_t> class BranchNodeEntryItem>
    OpStatus _update(int32_t idx, const std::pair<T1, I>& values, BranchNodeEntryItem<T2, Size>& accum)
    {
        sub<Offset>(idx, accum);

        this->setValue(values.first, idx, values.second);

        sum<Offset>(idx, accum);

        return OpStatus::OK;
    }

    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    OpStatus _remove(int32_t idx, BranchNodeEntryItem<T, Size>& accum)
    {
        sub<Offset>(idx, accum);
        return remove(idx, idx + 1);
    }


    OpStatusT<int64_t> setValue(int32_t block, int32_t idx, const Value& value)
    {
        // FIXME: Why do we skip setting if value is zero
        if (value != 0)
        {
            Value val = this->value(block, idx);
            this->value(block, idx) = value;

            return OpStatusT<int64_t>(val - value);
        }
        else {
            return OpStatusT<int64_t>(0);
        }
    }



    template <typename T>
    OpStatus setValues(int32_t idx, const core::StaticVector<T, Blocks>& values)
    {
        for (int32_t b = 0; b < Blocks; b++) {
            this->values(b)[idx] = values[b];
        }

        return reindex();
    }

    template <typename T>
    OpStatus addValues(int32_t idx, const core::StaticVector<T, Blocks>& values)
    {
        for (int32_t b = 0; b < Blocks; b++) {
            this->values(b)[idx] += values[b];
        }

        return reindex();
    }


    OpStatus addValue(int32_t block, int32_t idx, const Value& value)
    {
        if (value != 0)
        {
            this->value(block, idx) += value;
        }

        return reindex();
    }

    template <typename T, int32_t Indexes>
    OpStatus addValues(int32_t idx, int32_t from, int32_t size, const core::StaticVector<T, Indexes>& values)
    {
        for (int32_t block = 0; block < size; block++)
        {
            this->value(block, idx) += values[block + from];
        }

        return reindex();
    }




    void check() const {}

    OpStatus clear()
    {
        if(isFail(init())) {
            return OpStatus::FAIL;
        }

        if (this->allocatable().has_allocator())
        {
            auto alloc = this->allocatable().allocator();
            int32_t empty_size = MyType::empty_size();
            return toOpStatus(alloc->resizeBlock(this, empty_size));
        }

        return OpStatus::OK;
    }

    OpStatus clear(int32_t start, int32_t end)
    {
        for (int32_t block = 0; block < Blocks; block++)
        {
            auto values = this->values(block);

            for (int32_t c = start; c < end; c++)
            {
                values[c] = 0;
            }
        }

        return OpStatus::OK;
    }

    void generateDataEvents(IBlockDataEventHandler* handler) const
    {
        handler->startStruct();
        handler->startGroup("FSQ_TREE");

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
            handler->value("INDEX", BlockValueProviderFactory::provider(Blocks, [&](int32_t idx) {
                return index[idx][c];
            }));
        }

        handler->endGroup();

        handler->startGroup("DATA", meta->size());



        const Value* values[Blocks];

        for (int32_t b = 0; b < Blocks; b++) {
            values[b] = this->values(b);
        }

        for (int32_t c = 0; c < meta->size() ; c++)
        {
            handler->value("TREE_ITEM", BlockValueProviderFactory::provider(false, Blocks, [&](int32_t idx) {
                return values[idx][c];
            }));
        }

        handler->endGroup();

        handler->endGroup();

        handler->endStruct();
    }

    template <typename SerializationData>
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


    template <typename DeserializationData>
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
struct PkdStructSizeType<PkdFQTree<Types>> {
    static const PackedSizeType Value = PackedSizeType::FIXED;
};


template <typename Types>
struct StructSizeProvider<PkdFQTree<Types>> {
    static const int32_t Value = Types::Blocks;
};

template <typename Types>
struct IndexesSize<PkdFQTree<Types>> {
    static const int32_t Value = Types::Blocks;
};


}}
