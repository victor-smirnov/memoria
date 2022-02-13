
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

#include <memoria/core/packed/tree/fse/packed_fse_quick_tree_base.hpp>

#include <memoria/core/tools/static_array.hpp>
#include <memoria/profiles/common/block_operations.hpp>

#include <memoria/core/packed/tree/fse/packed_fse_quick_tree_so.hpp>

#include <memoria/core/iovector/io_substream_base.hpp>

namespace memoria {

template <typename IndexDataTypeT, size_t kBlocks, typename ValueDataTypeT = IndexDataTypeT, size_t kBranchingFactor = PackedTreeBranchingFactor, size_t kValuesPerBranch = PackedTreeBranchingFactor>
struct PkdFQTreeTypes {
    using IndexDataType    = IndexDataTypeT;
    using ValueDataType    = IndexDataTypeT;

    using IndexType = typename DataTypeTraits<IndexDataType>::ViewType;
    using ValueType = typename DataTypeTraits<ValueDataType>::ViewType;

    static constexpr size_t Blocks = kBlocks;
    static constexpr size_t BranchingFactor = kBranchingFactor;
    static constexpr size_t ValuesPerBranch = kValuesPerBranch;
};

template <typename Types> class PkdFQTree;


template <typename IndexValueT, size_t kBlocks = 1, typename ValueT = IndexValueT, size_t kBranchingFactor = PackedTreeBranchingFactor, size_t kValuesPerBranch = PackedTreeBranchingFactor>
using PkdFQTreeT = PkdFQTree<PkdFQTreeTypes<IndexValueT, kBlocks, ValueT, kBranchingFactor, kValuesPerBranch>>;


using core::StaticVector;

template <typename Types>
class PkdFQTree: public PkdFQTreeBase<typename Types::IndexType, typename Types::ValueType, Types::BranchingFactor, Types::ValuesPerBranch> {

    using Base      = PkdFQTreeBase<typename Types::IndexType, typename Types::ValueType, Types::BranchingFactor, Types::ValuesPerBranch>;
    using MyType    = PkdFQTree<Types>;

public:

    static constexpr uint32_t VERSION = 1;
    static constexpr size_t Blocks   = Types::Blocks;


    using Base::METADATA;
    using Base::index_size;
    using Base::SegmentsPerBlock;

    using FieldsList = MergeLists<
                typename Base::FieldsList,
                ConstValue<uint32_t, VERSION>,
                ConstValue<uint32_t, Blocks>
    >;


    using IndexValue = typename Types::IndexType;
    using Value      = typename Types::ValueType;

    using Values     = core::StaticVector<IndexValue, Blocks>;
    using DataValues = core::StaticVector<Value, Blocks>;

    using AccumValue = Value;

    using Metadata = typename Base::Metadata;

    using SizesT = core::StaticVector<size_t, Blocks>;

    using ConstPtrsT = core::StaticVector<const Value*, Blocks>;

    using ExtData = DTTTypeDimensionsTuple<Value>;
    using SparseObject = PackedFSEQuickTreeSO<ExtData, MyType>;

    using Base::sum;

    class ReadState {
    protected:
        ConstPtrsT values_;
        size_t idx_ = 0;
    public:
        ReadState() {}
        ReadState(const ConstPtrsT& values, size_t idx): values_(values), idx_(idx) {}

        ConstPtrsT& values() {return values_;}
        size_t& local_pos() {return idx_;}
        const ConstPtrsT& values() const {return values_;}
        const size_t& local_pos() const {return idx_;}
    };


    class Iterator: public ReadState {
        size_t size_;
        Values data_values_;

        using ReadState::idx_;
        using ReadState::values_;

        size_t idx_backup_;

    public:
        Iterator() {}
        Iterator(const ConstPtrsT& values, size_t idx, size_t size):
            ReadState(values, idx),
            size_(size)
        {}

        size_t size() const {return size_;}

        bool has_next() const {return idx_ < size_;}

        void next()
        {
            for (size_t b = 0; b < Blocks; b++)
            {
                data_values_[b] = values_[b][idx_];
            }

            idx_++;
        }

        const auto& value(size_t block) {return data_values_[block];}

        void mark() {
            idx_backup_ = idx_;
        }

        void restore() {
            idx_ = idx_backup_;
        }
    };

    class BlockIterator {
        const Value* values_;
        size_t idx_ = 0;

        size_t size_;
        Value data_value_;

        size_t idx_backup_;
    public:
        BlockIterator() {}
        BlockIterator(const Value* values, size_t idx, size_t size):
            values_(values),
            idx_(idx),
            size_(size),
            data_value_(),
            idx_backup_()
        {}

        size_t size() const {return size_;}

        bool has_next() const {return idx_ < size_;}

        void next()
        {
            for (size_t b = 0; b < Blocks; b++)
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



    static size_t estimate_block_size(size_t tree_capacity, size_t density_hi = 1, size_t density_lo = 1)
    {
        MEMORIA_ASSERT(density_hi, ==, 1); // data density should not be set for this type of trees
        MEMORIA_ASSERT(density_lo, ==, 1);

        return block_size(tree_capacity);
    }

    VoidResult init_tl(size_t data_block_size)
    {
        return Base::init_tl(data_block_size, Blocks);
    }

    VoidResult init(size_t capacity = 0)
    {
        MEMORIA_TRY_VOID(Base::init(empty_size(), Blocks * SegmentsPerBlock + 1));

        MEMORIA_TRY(meta, this->template allocate<Metadata>(METADATA));
        meta->set_size(0);
        meta->set_max_size(capacity);
        meta->set_index_size(MyType::index_size(capacity));

        for (size_t block = 0; block < Blocks; block++)
        {
            MEMORIA_TRY_VOID(this->template allocate_array_by_size<IndexValue>(block * SegmentsPerBlock + 1, meta->index_size()));
            MEMORIA_TRY_VOID(this->template allocate_array_by_size<Value>(block * SegmentsPerBlock + 2, capacity));
        }

        return VoidResult::of();
    }

    VoidResult init_bs(size_t block_size)
    {
        auto elements_num = elements_for(block_size);
        return init_by_block(block_size, elements_num);
    }

    VoidResult init_by_block(size_t block_size, size_t capacity = 0)
    {
        MEMORIA_TRY_VOID(Base::init(block_size, Blocks * SegmentsPerBlock + 1));

        MEMORIA_TRY(meta, this->template allocate<Metadata>(METADATA));

        meta->set_size(0);
        meta->set_max_size(capacity);
        meta->set_index_size(MyType::index_size(capacity));

        for (size_t block = 0; block < Blocks; block++)
        {
            MEMORIA_TRY_VOID(this->template allocate_array_by_size<IndexValue>(block * SegmentsPerBlock + 1, meta->index_size()));
            MEMORIA_TRY_VOID(this->template allocate_array_by_size<Value>(block * SegmentsPerBlock + 2, capacity));
        }

        return VoidResult::of();
    }

    VoidResult init(const SizesT& sizes)
    {
        return MyType::init(sizes[0]);
    }

    static size_t block_size(size_t capacity)
    {
        return Base::block_size(Blocks, capacity);
    }

    static size_t packed_block_size(size_t tree_capacity)
    {
        return block_size(tree_capacity);
    }


    size_t block_size() const
    {
        return Base::block_size();
    }

    size_t block_size_for(const MyType* other) const
    {
        return block_size(this->size() + other->size());
    }




    static size_t elements_for(size_t block_size)
    {
        return Base::tree_size(Blocks, block_size);
    }


    static constexpr size_t default_size(size_t available_space)
    {
        return empty_size();
    }

    VoidResult init_default(size_t block_size) {
        return init();
    }


    static size_t empty_size()
    {
        return block_size(0);
    }



    VoidResult reindex() {
        Base::reindex(Blocks);
        return VoidResult::of();
    }

    void dump_index(std::ostream& out = std::cout) const {
        Base::dump_index(Blocks, out);
    }

    bool check_capacity(size_t size) const
    {
        MEMORIA_V1_ASSERT_TRUE(size >= 0);

        auto alloc = this->allocator();

        size_t total_size          = this->size() + size;
        size_t total_block_size    = MyType::block_size(total_size);
        size_t my_block_size       = alloc->element_size(this);
        size_t delta               = total_block_size - my_block_size;

        return alloc->free_space() >= delta;
    }


    // ================================ Container API =========================================== //



    template <typename Fn>
    void read(size_t block, size_t start, size_t end, Fn&& fn) const
    {
        MEMORIA_ASSERT(end, <=, this->size());
        MEMORIA_ASSERT(start, >=, 0);
        MEMORIA_ASSERT(start, <=, end);

        auto values = this->values(block);

        for (size_t c = start; c < end; c++)
        {
            fn(block, values[c]);
            fn.next();
        }
    }



    // ========================================= Insert/Remove/Resize ============================================== //


    // FIXME! use absolute size value, instead of relative one
    VoidResult resize(Metadata* meta, size_t size)
    {
        size_t new_data_size  = meta->max_size() + size;
        size_t new_index_size = MyType::index_size(new_data_size);

        for (size_t block = 0; block < Blocks; block++)
        {
            MEMORIA_TRY_VOID(Base::resize_block(SegmentsPerBlock * block + 1, new_index_size * sizeof(IndexValue)));
            MEMORIA_TRY_VOID(Base::resize_block(SegmentsPerBlock * block + 2, new_data_size * sizeof(Value)));
        }

        meta->set_max_size(meta->max_size() + size);
        meta->set_index_size(new_index_size);

        return VoidResult::of();
    }

    VoidResult insertSpace(size_t idx, size_t room_length)
    {
        auto meta = this->metadata();

        size_t capacity  = meta->capacity();

        if (capacity < room_length)
        {
            MEMORIA_TRY_VOID(resize(meta, room_length - capacity));
        }

        for (size_t block = 0; block < Blocks; block++)
        {
            auto* values = this->values(block);

            CopyBuffer(
                    values + idx,
                    values + idx + room_length,
                    meta->size() - idx
            );

            for (size_t c = idx; c < idx + room_length; c++) {
                values[c] = Value{};
            }
        }

        meta->add_size(room_length);

        return VoidResult::of();
    }



    VoidResult copyTo(MyType* other, size_t copy_from, size_t count, size_t copy_to) const
    {
        MEMORIA_V1_ASSERT_TRUE_RTN(copy_from >= 0);
        MEMORIA_V1_ASSERT_TRUE_RTN(count >= 0);

        for (size_t block = 0; block < Blocks; block++)
        {
            auto my_values    = this->values(block);
            auto other_values = other->values(block);

            CopyBuffer(
                    my_values + copy_from,
                    other_values + copy_to,
                    count
            );
        }

        return VoidResult::of();
    }

public:
    VoidResult splitTo(MyType* other, size_t idx)
    {
        size_t total = this->size() - idx;
        if (total > 0)
        {
            MEMORIA_TRY_VOID(other->insertSpace(0, total));
            MEMORIA_TRY_VOID(copyTo(other, idx, total, 0));
            MEMORIA_TRY_VOID(other->reindex());
            MEMORIA_TRY_VOID(removeSpace(idx, this->size()));

            return reindex();
        }
        else {
            return VoidResult::of();
        }
    }

    VoidResult mergeWith(MyType* other) const
    {
        size_t my_size     = this->size();
        size_t other_size  = other->size();

        MEMORIA_TRY_VOID(other->insertSpace(other_size, my_size));
        MEMORIA_TRY_VOID(copyTo(other, 0, my_size, other_size));

        return other->reindex();
    }




    VoidResult removeSpace(size_t start, size_t end)
    {
        return remove(start, end);
    }

    VoidResult remove(size_t start, size_t end)
    {
        auto meta = this->metadata();

        size_t room_length = end - start;
        size_t size = meta->size();

        MEMORIA_ASSERT_RTN(start + room_length, <=, size);

        for (size_t block_f = 0; block_f < Blocks; block_f++)
        {
            size_t block = Blocks - 1 - block_f;

            auto values = this->values(block);

            CopyBuffer(
                    values + end,
                    values + start,
                    size - end
            );

            for (size_t c = start + size - end; c < size; c++)
            {
                values[c] = 0;
            }
        }

        meta->sub_size(room_length);

        MEMORIA_TRY_VOID(resize(meta, -room_length));

        return reindex();
    }



    template <typename Iter>
    VoidResult populate_from_iterator(size_t start, size_t length, Iter&& iter)
    {
        MEMORIA_ASSERT_RTN(start, <=, this->size());
        MEMORIA_TRY_VOID(insertSpace(start, length));

        for (size_t c = 0; c < length; c++)
        {
            iter.next();
            for (size_t block = 0; block < Blocks; block++)
            {
                this->value(block, c + start) = iter.value(block);
            }
        }

        return VoidResult::of();
    }


    ReadState positions(size_t idx) const
    {
        ReadState state;

        state.local_pos() = idx;

        for (size_t b = 0; b < Blocks; b++) {
            state.values()[b] = this->values(b);
        }

        return state;
    }

    Iterator iterator(size_t idx) const
    {
        ConstPtrsT ptrs;

        for (size_t b = 0; b < Blocks; b++) {
            ptrs[b] = this->values(b);
        }

        return Iterator(ptrs, idx, this->size());
    }

    BlockIterator iterator(size_t block, size_t idx) const
    {
        return BlockIterator(this->values(block), idx, this->size());
    }

    VoidResult insert_io_substream(size_t at, const io::IOSubstream& substream, size_t start, size_t inserted)
    {
//        const io::IOColumnwiseFixedSizeArraySubstream<Value>& buffer
//                = io::substream_cast<io::IOColumnwiseFixedSizeArraySubstream<Value>>(substream);

//        if (isFail(insertSpace(at, inserted))) {
//            return VoidResult::FAIL;
//        }

//        for (size_t block = 0; block < Blocks; block++)
//        {
//            auto buffer_values = buffer.select(block, start);
//            CopyBuffer(buffer_values, this->values(block) + at, inserted);
//        }

        return VoidResult::of();
    }

    void configure_io_substream(io::IOSubstream& substream) const
    {
//        auto& view = io::substream_cast<IOSubstreamView>(substream);

//        io::FixedSizeArrayColumnMetadata<Value> columns[Blocks]{};

//        for (size_t blk = 0; blk < Blocks; blk++)
//        {
//            columns[blk].data_buffer = this->values(blk);
//            columns[blk].size = this->size();
//            columns[blk].capacity = columns[blk].size;
//        }

//        view.configure(columns);
    }

    template <typename T>
    VoidResult append(const StaticVector<T, Blocks>& values)
    {
        auto meta = this->metadata();

        for (size_t b = 0; b < Blocks; b++)
        {
            this->values(b)[meta->size()] = values[b];
        }

        meta->add_size(1);

        return VoidResult::of();
    }


    template <typename AccessorFn>
    VoidResult insert_entries(psize_t row_at, psize_t size, AccessorFn&& elements, bool reindex = true)
    {
        MEMORIA_TRY_VOID(this->insertSpace(row_at, size));

        for (psize_t c = 0; c < size; c++)
        {
            for (size_t block = 0; block < Blocks; block++)
            {
                this->value(block, c + row_at) = elements(block, c);
            }
        }

        if (reindex) {
            return this->reindex();
        }
        else {
            return VoidResult::of();
        }
    }

    void check() const {
    }

    VoidResult clear()
    {
        MEMORIA_TRY_VOID(init());

        if (this->allocatable().has_allocator())
        {
            auto alloc = this->allocatable().allocator();
            size_t empty_size = MyType::empty_size();
            MEMORIA_TRY_VOID(alloc->resize_block(this, empty_size));
        }

        return VoidResult::of();
    }

    VoidResult clear(size_t start, size_t end)
    {
        for (size_t block = 0; block < Blocks; block++)
        {
            auto values = this->values(block);

            for (size_t c = start; c < end; c++)
            {
                values[c] = 0;
            }
        }

        return VoidResult::of();
    }

    void generateDataEvents(IBlockDataEventHandler* handler) const
    {
        handler->startStruct();
        handler->startGroup("FSQ_TREE");

        auto meta = this->metadata();

        handler->value("SIZE",          &meta->size_imm());
        handler->value("MAX_SIZE",      &meta->max_size_imm());
        handler->value("INDEX_SIZE",    &meta->index_size_imm());

        handler->startGroup("INDEXES", meta->index_size());

        auto index_size = meta->index_size();

        const IndexValue* index[Blocks];

        for (size_t b = 0; b < Blocks; b++) {
            index[b] = this->index(b);
        }

        for (size_t c = 0; c < index_size; c++)
        {
            handler->value("INDEX", BlockValueProviderFactory::provider(Blocks, [&](size_t idx) {
                return index[idx][c];
            }));
        }

        handler->endGroup();

        handler->startGroup("DATA", meta->size());



        const Value* values[Blocks];

        for (size_t b = 0; b < Blocks; b++) {
            values[b] = this->values(b);
        }

        for (size_t c = 0; c < meta->size() ; c++)
        {
            handler->value("TREE_ITEM", BlockValueProviderFactory::provider(false, Blocks, [&](size_t idx) {
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

        FieldFactory<psize_t>::serialize(buf, meta->size_imm());
        FieldFactory<psize_t>::serialize(buf, meta->max_size_imm());
        FieldFactory<psize_t>::serialize(buf, meta->index_size_imm());

        for (size_t b = 0; b < Blocks; b++)
        {
            FieldFactory<IndexValue>::serialize(buf, this->index(b), meta->index_size_imm());
            FieldFactory<Value>::serialize(buf, this->values(b), meta->size_imm());
        }
    }


    template <typename DeserializationData>
    void deserialize(DeserializationData& buf)
    {
        Base::deserialize(buf);

        Metadata* meta = this->metadata();

        FieldFactory<psize_t>::deserialize(buf, meta->size_mut());
        FieldFactory<psize_t>::deserialize(buf, meta->max_size_mut());
        FieldFactory<psize_t>::deserialize(buf, meta->index_size_mut());

        for (size_t b = 0; b < Blocks; b++)
        {
            FieldFactory<IndexValue>::deserialize(buf, this->index(b), meta->index_size_mut());
            FieldFactory<Value>::deserialize(buf, this->values(b), meta->size_mut());
        }
    }

    Values access(size_t row_idx) const  {
        Values vv{};
        for (size_t c = 0; c < Blocks; c++) {
            vv[c] = this->value(c, row_idx);
        }
        return vv;
    }

    Value sum_for_rank(size_t start, size_t end, size_t symbol, SeqOpType seq_op) const {
        switch (seq_op) {
            case SeqOpType::EQ : return sum_for_rank_eq(start, end, symbol);
            case SeqOpType::NEQ: return sum_for_rank_neq(start, end, symbol);
            case SeqOpType::LT : return sum_for_rank_lt(start, end, symbol);
            case SeqOpType::LE : return sum_for_rank_le(start, end, symbol);
            case SeqOpType::GT : return sum_for_rank_gt(start, end, symbol);
            case SeqOpType::GE : return sum_for_rank_ge(start, end, symbol);

            default: MEMORIA_MAKE_GENERIC_ERROR("Unsupported SeqOpType: {}", (size_t)seq_op).do_throw();
        }
    }

    struct SelectResult {
        size_t idx;
        size_t size;
        Value rank;

        bool is_end() const {return idx >= size;}
    };

    SelectResult find_for_select_fw(size_t start, Value rank, size_t symbol, SeqOpType seq_op) const {
        switch (seq_op) {
            case SeqOpType::EQ : return find_for_select_fw_eq(start, rank, symbol);
            case SeqOpType::NEQ: return find_for_select_fw_fn(start, rank, symbol, FindFwNEQFn());
            case SeqOpType::LT : return find_for_select_fw_fn(start, rank, symbol, FindFwLTFn());
            case SeqOpType::LE : return find_for_select_fw_fn(start, rank, symbol, FindFwLEFn());
            case SeqOpType::GT : return find_for_select_fw_fn(start, rank, symbol, FindFwGTFn());
            case SeqOpType::GE : return find_for_select_fw_fn(start, rank, symbol, FindFwGEFn());
            case SeqOpType::EQ_NLT : return find_for_select_fw_nlt(start, rank, symbol);

            default: MEMORIA_MAKE_GENERIC_ERROR("Unsupported SeqOpType: {}", (size_t)seq_op).do_throw();
        }
    }

    SelectResult find_for_select_bw(size_t start, Value rank, size_t symbol, SeqOpType seq_op) const {
        switch (seq_op) {
            case SeqOpType::EQ : return find_for_select_bw_eq(start, rank, symbol);
            case SeqOpType::NEQ: return find_for_select_bw_fn(start, rank, symbol, FindFwNEQFn());
            case SeqOpType::LT : return find_for_select_bw_fn(start, rank, symbol, FindFwLTFn());
            case SeqOpType::LE : return find_for_select_bw_fn(start, rank, symbol, FindFwLEFn());
            case SeqOpType::GT : return find_for_select_bw_fn(start, rank, symbol, FindFwGTFn());
            case SeqOpType::GE : return find_for_select_bw_fn(start, rank, symbol, FindFwGEFn());
            case SeqOpType::EQ_NLT : return find_for_select_bw_nlt(start, rank, symbol);

            default: MEMORIA_MAKE_GENERIC_ERROR("Unsupported SeqOpType: {}", (size_t)seq_op).do_throw();
        }
    }

    Value sum_for_rank_eq(size_t start, size_t end, size_t symbol) const {
        return this->sum(symbol, start, end);
    }

    Value sum_for_rank_neq(size_t start, size_t end, size_t symbol) const
    {
        Value val{};

        for (size_t c = 0; c < Blocks; c++) {
            val += symbol != c ? this->sum(symbol, start, end) : 0;
        }

        return val;
    }

    Value sum_for_rank_lt(size_t start, size_t end, size_t symbol) const
    {
        Value val{};

        for (size_t c = 0; c < symbol; c++) {
            val += this->sum(symbol, start, end);
        }

        return val;
    }

    Value sum_for_rank_gt(size_t start, size_t end, size_t symbol) const
    {
        Value val{};

        for (size_t c = symbol + 1; c < Blocks; c++) {
            val += this->sum(symbol, start, end);
        }

        return val;
    }

    Value sum_for_rank_le(size_t start, size_t end, size_t symbol) const
    {
        Value val{};

        for (size_t c = 0; c <= symbol; c++) {
            val += this->sum(symbol, start, end);
        }

        return val;
    }

    Value sum_for_rank_ge(size_t start, size_t end, size_t symbol) const
    {
        Value val{};

        for (size_t c = symbol; c < Blocks; c++) {
            val += this->sum(symbol, start, end);
        }

        return val;
    }


    SelectResult find_for_select_fw_eq(size_t start, Value rank, size_t symbol) const
    {
        auto res = this->find_gt_fw(symbol, start, rank);
        return SelectResult{res.idx(), this->size(), res.prefix()};
    }



    SelectResult find_for_select_fw_nlt(size_t start, Value rank, size_t symbol) const
    {
        auto res_eq = find_for_select_fw_eq(start, rank, symbol);
        if (symbol > 0)
        {
            auto res_lt = find_for_select_fw_fn(start, rank, symbol, FindFwLTFn());
            if (res_lt.idx < res_eq.idx) {
                return res_lt;
            }
        }

        return res_eq;
    }

    struct FindFwNEQFn {
        Value sum(const MyType& tree, size_t idx, size_t symbol) const {
            Value tmp{};
            for (size_t c = 0; c < Blocks; c++) {
                tmp += c != symbol ? tree.value(c, idx) : 0;
            }
            return tmp;
        }
    };

    struct FindFwLTFn {
        Value sum(const MyType& tree, size_t idx, size_t symbol) const {
            Value tmp{};
            for (size_t c = 0; c < symbol; c++) {
                tmp += tree.value(c, idx);
            }
            return tmp;
        }
    };

    struct FindFwLEFn {
        Value sum(const MyType& tree, size_t idx, size_t symbol) const {
            Value tmp{};
            for (size_t c = 0; c <= symbol; c++) {
                tmp += tree.value(c, idx);
            }
            return tmp;
        }
    };

    struct FindFwGTFn {
        Value sum(const MyType& tree, size_t idx, size_t symbol) const {
            Value tmp{};
            for (size_t c = symbol + 1; c < Blocks; c++) {
                tmp += tree.value(c, idx);
            }
            return tmp;
        }
    };

    struct FindFwGEFn {
        Value sum(const MyType& tree, size_t idx, size_t symbol) const {
            Value tmp{};
            for (size_t c = symbol; c < Blocks; c++) {
                tmp += tree.value(c, idx);
            }
            return tmp;
        }
    };

    template <typename Fn>
    SelectResult find_for_select_fw_fn(size_t start, Value rank, size_t symbol, Fn&& fn) const
    {
        Value prefix{};
        size_t size = this->size();

        for (size_t c = start; c < size; c++)
        {
            Value tmp = fn.sum(*this, c, symbol);

            if (rank < prefix + tmp) {
                return SelectResult{c, size, prefix};
            }
            else {
                prefix += tmp;
            }
        }

        return SelectResult{size, size, prefix};
    }







    template <typename Fn>
    SelectResult find_for_select_bw_fn(size_t start, Value rank, size_t symbol, Fn&& fn) const
    {
        Value prefix{};
        size_t size = this->size();

        for (size_t c = start; c >= 0; c--)
        {
            Value tmp = fn.sum(*this, c, symbol);

            if (rank < prefix + tmp) {
                return SelectResult{c, size, prefix};
            }
            else {
                prefix += tmp;
            }
        }

        return SelectResult{size, size, prefix};
    }

    SelectResult find_for_select_bw_eq(size_t start, Value rank, size_t symbol) const
    {
        auto res = this->find_gt_bw(symbol, start, rank);
        return SelectResult{res.idx(), this->size(), res.prefix()};
    }



    SelectResult find_for_select_bw_nlt(size_t start, Value rank, size_t symbol) const
    {
        auto res_eq = find_for_select_bw_eq(start, rank, symbol);
        if (symbol > 0)
        {
            auto res_lt = find_for_select_bw_fn(start, rank, symbol, FindFwLTFn());

            if ((!res_lt.is_end()) && res_lt.idx > res_eq.idx) {
                return res_lt;
            }
        }

        return res_eq;
    }
};



template <typename Types>
struct PackedStructTraits<PkdFQTree<Types>>
{
    static constexpr PackedDataTypeSize DataTypeSize = PackedDataTypeSize::FIXED;

    using SearchKeyType = typename Types::IndexType;

    using SearchKeyDataType = typename Types::ValueDataType;
    static constexpr PkdSearchType KeySearchType = PkdSearchType::SUM;

    using AccumType = typename PkdFQTree<Types>::Value;

    static constexpr size_t Blocks = PkdFQTree<Types>::Blocks;
    static constexpr size_t Indexes = PkdFQTree<Types>::Blocks;

};


}
