
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/core/packed/tree/vle/packed_vle_quick_tree_base_base.hpp>


namespace memoria {
namespace v1 {

class PkdVQTreeMetadata {
    Int size_;

public:
    PkdVQTreeMetadata() = default;

    Int& size() {return size_;}
    const Int& size() const {return size_;}

    template <typename, Int, Int, Int, typename> friend class PkdVQTreeBase1;
    template <typename, typename, template <typename> class Codec, Int, Int> friend class PkdVQTreeBase;
};



template <
    typename IndexValueT,
    typename ValueT,
    template <typename> class CodecT,
    Int kBranchingFactor,
    Int kValuesPerBranch
>
class PkdVQTreeBase: public PkdVQTreeBaseBase<IndexValueT, kBranchingFactor, kValuesPerBranch, 4, PkdVQTreeMetadata> {

    using Base      = PkdVQTreeBaseBase<IndexValueT, kBranchingFactor, kValuesPerBranch, 4, PkdVQTreeMetadata>;
    using MyType    = PkdVQTreeBase<IndexValueT, ValueT, CodecT, kBranchingFactor, kValuesPerBranch>;

public:
    static constexpr UInt VERSION = 1;

    using IndexValue    = IndexValueT;
    using Value         = ValueT;

    using typename Base::Metadata;
    using typename Base::TreeLayout;

    using OffsetsType   = UBigInt;

    using Codec         = CodecT<Value>;

    using ValueData = typename Codec::BufferType;

    static const Int BranchingFactor        = kBranchingFactor;
    static const Int ValuesPerBranch        = kValuesPerBranch;

    static const bool FixedSizeElement      = false;

    static constexpr Int ValuesPerBranchMask    = ValuesPerBranch - 1;
    static constexpr Int BranchingFactorMask    = BranchingFactor - 1;

    static constexpr Int ValuesPerBranchLog2    = Log2(ValuesPerBranch) - 1;
    static constexpr Int BranchingFactorLog2    = Log2(BranchingFactor) - 1;

    static constexpr Int SegmentsPerBlock = 4;


    static constexpr Int BITS_PER_OFFSET        = Codec::BitsPerOffset;
    static constexpr Int BITS_PER_DATA_VALUE    = Codec::ElementSize;

    using Base::BlocksStart;

    using Base::METADATA;
    using Base::DATA_SIZES;

    using Base::VALUES;
    using Base::VALUE_INDEX;
    using Base::OFFSETS;
    using Base::SIZE_INDEX;

    using typename Base::LocateResult;
    using typename Base::WalkerState;

    struct InitFn {
        Int blocks_;

        InitFn(Int blocks): blocks_(blocks) {}

        Int block_size(Int items_number) const {
            return MyType::block_size_equi(blocks_, items_number);
        }

        Int max_elements(Int block_size)
        {
            return block_size * 8 / BITS_PER_DATA_VALUE;
        }
    };

public:

    using Base::metadata;
    using Base::data_size;

    PkdVQTreeBase() = default;

    using FieldsList = MergeLists<
                typename Base::FieldsList,
                ConstValue<UInt, VERSION>,
                Value
    >;

    static constexpr Int value_segment_size(Int values)
    {
        return PackedAllocatable::roundUpBitsToAlignmentBlocks(values * BITS_PER_DATA_VALUE);
    }

    static constexpr Int number_of_offsets(Int values)
    {
        //FIXME: Number of offsets cant be zero
        return Base::divUpV(values);
    }

    static constexpr Int offsets_segment_size(Int values)
    {
        return PackedAllocator::roundUpBitsToAlignmentBlocks(number_of_offsets(values) * BITS_PER_OFFSET);
    }

    static constexpr Int index1_segment_size(Int index_size) {
        return PackedAllocator::roundUpBytesToAlignmentBlocks(index_size * sizeof(IndexValue));
    }

    static constexpr Int index2_segment_size(Int index_size) {
        return PackedAllocator::roundUpBytesToAlignmentBlocks(index_size * sizeof(int));
    }


//    void init(Int data_block_size, Int blocks)
//    {
//      Base::init(data_block_size, blocks * SegmentsPerBlock + BlocksStart);
//
//      Metadata* meta = this->template allocate<Metadata>(METADATA);
//      this->template allocateArrayBySize<Int>(DATA_SIZES, blocks);
//
//      meta->size()        = 0;
//
//      Int max_size        = FindTotalElementsNumber3(data_block_size, InitFn(blocks));
//      Int offsets_size    = offsets_segment_size(max_size);
//
//      Int values_segment_length = this->value_segment_size(max_size);
//      Int index_size      = this->index_size(max_size);
//
//      for (Int block = 0; block < blocks; block++)
//      {
//          this->template allocateArrayBySize<IndexValue>(block * SegmentsPerBlock + VALUE_INDEX + BlocksStart, index_size);
//          this->template allocateArrayBySize<Int>(block * SegmentsPerBlock + SIZE_INDEX + BlocksStart, index_size);
//          this->template allocateArrayBySize<Byte>(block * SegmentsPerBlock + OFFSETS + BlocksStart, offsets_size);
//          this->template allocateArrayBySize<Byte>(block * SegmentsPerBlock + VALUES + BlocksStart, values_segment_length);
//      }
//    }

    void init_tl(Int data_block_size, Int blocks)
    {
        Base::init(data_block_size, blocks * SegmentsPerBlock + BlocksStart);

        Metadata* meta = this->template allocate<Metadata>(METADATA);
        this->template allocateArrayBySize<Int>(DATA_SIZES, blocks);

        meta->size()        = 0;

        Int max_size        = 0;
        Int offsets_size    = offsets_segment_size(max_size);

        Int values_segment_length = this->value_segment_size(max_size);
        Int index_size      = this->index_size(max_size);

        for (Int block = 0; block < blocks; block++)
        {
            this->template allocateArrayBySize<IndexValue>(block * SegmentsPerBlock + VALUE_INDEX + BlocksStart, index_size);
            this->template allocateArrayBySize<Int>(block * SegmentsPerBlock + SIZE_INDEX + BlocksStart, index_size);
            this->template allocateArrayBySize<Byte>(block * SegmentsPerBlock + OFFSETS + BlocksStart, offsets_size);
            this->template allocateArrayBySize<Byte>(block * SegmentsPerBlock + VALUES + BlocksStart, values_segment_length);
        }
    }

//    void init(Int blocks)
//    {
//      Int block_size = MyType::tree_size(blocks, 0);
//      Base::init(block_size, blocks * SegmentsPerBlock + BlocksStart);
//
//      Metadata* meta = this->template allocate<Metadata>(Base::METADATA);
//      this->template allocateArrayBySize<Int>(Base::DATA_SIZES, blocks);
//
//      meta->size() = 0;
//
//      for (Int block = 0; block < blocks; block++)
//      {
//          this->template allocateArrayBySize<IndexValue>(block * SegmentsPerBlock + VALUE_INDEX + BlocksStart, 0);
//          this->template allocateArrayBySize<Value>(block * SegmentsPerBlock + SIZE_INDEX + BlocksStart, 0);
//          this->template allocateArrayBySize<Byte>(block * SegmentsPerBlock + OFFSETS + BlocksStart, 0);
//          this->template allocateArrayBySize<ValueData>(block * SegmentsPerBlock + VALUES + BlocksStart, 0);
//      }
//    }



    //FIXME: invalid block size calculation by capacity
    static Int block_size_equi(Int blocks, Int capacity)
    {
        Int metadata_length = Base::roundUpBytesToAlignmentBlocks(sizeof(Metadata));

        Int data_sizes_length = Base::roundUpBytesToAlignmentBlocks(blocks * sizeof(Int));

        Int index_size      = MyType::index_size(capacity);
        Int index_length    = Base::roundUpBytesToAlignmentBlocks(index_size * sizeof(IndexValue));
        Int sizes_length    = Base::roundUpBytesToAlignmentBlocks(index_size * sizeof(Int));

        Int values_length   = Base::roundUpBitsToAlignmentBlocks(capacity * BITS_PER_DATA_VALUE);

        Int offsets_length  = offsets_segment_size(capacity);

        Int  blocks_length  = index_length + values_length + offsets_length + sizes_length;

        return Base::block_size(
                metadata_length +
                data_sizes_length +
                blocks_length * blocks,
                blocks * SegmentsPerBlock + BlocksStart
        );
    }




    Int block_size() const
    {
        return Base::block_size();
    }

    static Int index_size(Int capacity)
    {
        TreeLayout layout;
        Base::compute_tree_layout(capacity, layout);
        return layout.index_size;
    }

    static Int tree_size(Int blocks, Int block_size)
    {
        return block_size >= (Int)sizeof(Value) ? FindTotalElementsNumber3(block_size, InitFn(blocks)) : 0;
    }


    Int offset(Int block, Int idx) const
    {
        return GetBits(offsets(block), idx * BITS_PER_OFFSET, BITS_PER_OFFSET);
    }

    Int offset(const OffsetsType* block, Int idx) const
    {
        return GetBits(block, idx * BITS_PER_OFFSET, BITS_PER_OFFSET);
    }

    void set_offset(Int block, Int idx, Int value)
    {
        SetBits(offsets(block), idx * BITS_PER_OFFSET, value, BITS_PER_OFFSET);
    }

    void set_offset(OffsetsType* block, Int idx, Int value)
    {
        SetBits(block, idx * BITS_PER_OFFSET, value, BITS_PER_OFFSET);
    }

    OffsetsType* offsets(Int block) {
        return this->template get<OffsetsType>(block * SegmentsPerBlock + OFFSETS + BlocksStart);
    }

    const OffsetsType* offsets(Int block) const {
        return this->template get<OffsetsType>(block * SegmentsPerBlock + OFFSETS + BlocksStart);
    }

    ValueData* values(Int block) {
        return this->template get<ValueData>(block * SegmentsPerBlock + VALUES + BlocksStart);
    }
    const ValueData* values(Int block) const {
        return this->template get<ValueData>(block * SegmentsPerBlock + VALUES + BlocksStart);
    }



    bool has_index(Int block) const {
        return this->element_size(block * SegmentsPerBlock + VALUE_INDEX + BlocksStart) > 0;
    }


    struct FindGEWalker {
        IndexValue sum_ = 0;
        IndexValue target_;

        IndexValue next_;

        Int idx_;

    public:
        FindGEWalker(IndexValue target): target_(target) {}

        template <typename T>
        bool compare(T value)
        {
            next_ = value;
            return sum_ + next_ >= target_;
        }

        void next() {
            sum_ += next_;
        }

        Int& idx() {return idx_;}
        const Int& idx() const {return idx_;}

        FindGEWalker& idx(Int idx) {
            idx_ = idx;
            return *this;
        }

        IndexValue prefix() const {
            return sum_;
        }

        FindGEWalker& adjust(Int base, Int size) {
            idx_ -= base;

            if (idx_ > size) {
                idx_ = size;
            }

            return *this;
        }

        FindGEWalker& adjust(Int base, Int size, IndexValue sum) {
            idx_ -= base;

            if (idx_ > size) {
                idx_ = size;
            }

            this->sum_ = sum;

            return *this;
        }

        FindGEWalker& adjust_bw(IndexValue sum) {
            idx_ = -1;

            this->sum_ = sum;

            return *this;
        }


        FindGEWalker& adjust_s(Int base, Int size, IndexValue sum) {
            idx_ -= base;

            if (idx_ > size) {
                idx_ = size;
            }

            this->sum_ -= sum;

            return *this;
        }
    };

    struct FindGTWalker {
        IndexValue sum_ = 0;
        IndexValue target_;

        IndexValue next_;

        Int idx_;
    public:
        FindGTWalker(IndexValue target): target_(target) {}

        template <typename T>
        bool compare(T value)
        {
            next_ = value;
            return sum_ + next_ > target_;
        }

        void next() {
            sum_ += next_;
        }

        Int& idx() {return idx_;}
        const Int& idx() const {return idx_;}

        FindGTWalker& idx(Int idx) {
            idx_ = idx;
            return *this;
        }

        IndexValue prefix() const {
            return sum_;
        }

        FindGTWalker& adjust(Int base, Int size) {
            idx_ -= base;

            if (idx_ > size) {
                idx_ = size;
            }

            return *this;
        }

        FindGTWalker& adjust(Int base, Int size, IndexValue sum) {
            idx_ -= base;

            if (idx_ > size) {
                idx_ = size;
            }

            this->sum_ = sum;

            return *this;
        }

        FindGTWalker& adjust_bw(IndexValue sum) {
            idx_ = -1;

            this->sum_ = sum;

            return *this;
        }

        FindGTWalker& adjust_s(Int base, Int size, IndexValue sum) {
            idx_ -= base;

            if (idx_ > size) {
                idx_ = size;
            }

            this->sum_ -= sum;

            return *this;
        }
    };



    auto gfind_ge(Int block, IndexValue value) const
    {
        return find(block, FindGEWalker(value));
    }

    auto gfind_gt(Int block, IndexValue value) const
    {
        return find(block, FindGTWalker(value));
    }

    auto gfind_ge_fw(Int block, Int start, IndexValue value) const
    {
        return walk_fw(block, start, FindGEWalker(value));
    }

    auto gfind_gt_fw(Int block, Int start, IndexValue value) const
    {
        return walk_fw(block, start, FindGTWalker(value));
    }


    auto gfind_ge_bw(Int block, Int start, IndexValue value) const
    {
        return walk_bw(block, start, FindGEWalker(value));
    }

    auto gfind_gt_bw(Int block, Int start, IndexValue value) const
    {
        return walk_bw(block, start, FindGTWalker(value));
    }

    class Location {
        Int window_start_;
        Int size_prefix_;
        Int pos_;
    public:
        Location(Int window_start, Int size_prefix, Int pos):
            window_start_(window_start), size_prefix_(size_prefix), pos_(pos)
        {}

        Int window_start() const {return window_start_;}
        Int size_prefix() const {return size_prefix_;}
        Int pos() const {return pos_;}
    };

    Int locate(Int block, Int idx) const
    {
        auto values = this->values(block);
        TreeLayout layout = Base::compute_tree_layout(this->data_size(block));

        return locate(layout, values, block, idx).idx;
    }

    LocateResult locate(TreeLayout& layout, const ValueData* values, Int block, Int idx) const
    {
        size_t data_size = this->data_size(block);

        if (data_size > 0) {

            LocateResult locate_result;

            if (layout.levels_max >= 0)
            {
                layout.valaue_block_size_prefix = this->size_index(block);
                layout.indexes = this->value_index(block);

                locate_result = this->locate_index(layout, idx);
            }

            Int window_num = locate_result.idx;

            Int window_start = window_num << ValuesPerBranchLog2;
            if (window_start >= 0)
            {
                Codec codec;

                size_t offset = this->offset(block, window_num);

                Int c = 0;
                Int local_idx = idx - locate_result.index_cnt;
                size_t pos;
                for (pos = window_start + offset; pos < data_size && c < local_idx; c++)
                {
                    auto len = codec.length(values, pos, data_size);
                    pos += len;
                }

                locate_result.idx = pos;

                return locate_result;
            }
            else {
                return LocateResult(data_size, locate_result.index_cnt);
            }
        }
        else {
            return LocateResult(0, 0);
        }
    }


    LocateResult locate_with_sum(TreeLayout& layout, const ValueData* values, Int block, Int idx) const
    {
        size_t data_size = this->data_size(block);
        if (data_size > 0)
        {
            LocateResult locate_result;

            if (layout.levels_max >= 0)
            {
                layout.valaue_block_size_prefix = this->size_index(block);
                layout.indexes = this->value_index(block);

                locate_result = this->locate_index_with_sum(layout, idx);
            }

            Int window_num = locate_result.idx;

            Int window_start = (window_num << ValuesPerBranchLog2);
            if (window_start >= 0)
            {
                Codec codec;

                size_t offset = this->offset(block, window_num);

                Int c = 0;
                Int local_idx = idx - locate_result.index_cnt;
                for (size_t pos = window_start + offset; pos < data_size && c < local_idx; c++)
                {
                    Value value;
                    auto len = codec.decode(values, value, pos, data_size);
                    pos += len;
                    locate_result.value_sum += value;
                }

                locate_result.idx = c + locate_result.index_cnt;

                return locate_result;
            }
            else {
                return LocateResult(data_size, locate_result.index_cnt, locate_result.value_sum);
            }
        }
        else {
            return LocateResult(0, 0, 0);
        }
    }


    template <typename Walker>
    auto find(Int block, Walker&& walker) const
    {
        auto metadata = this->metadata();
        auto values = this->values(block);

        size_t data_size = this->data_size(block);
        Int size = metadata->size();

        Codec codec;

        if (!this->has_index(block))
        {
            size_t pos = 0;
            for (Int c = 0; pos < data_size; c++)
            {
                Value value;
                size_t length = codec.decode(values, value, pos, data_size);

                if (walker.compare(value))
                {
                    return walker.idx(c);
                }
                else {
                    pos += length;
                    walker.next();
                }
            }

            return walker.idx(size);
        }
        else {
            TreeLayout data = this->compute_tree_layout(data_size);

            data.indexes = this->value_index(block);
            data.valaue_block_size_prefix = this->size_index(block);

            WalkerState state;

            Int idx = this->find_index(data, state, walker);

            if (idx >= 0)
            {
                size_t local_pos = (idx << ValuesPerBranchLog2) + this->offset(block, idx);

                for (Int local_idx = state.size_sum; local_pos < data_size; local_idx++)
                {
                    Value value;
                    size_t length = codec.decode(values, value, local_pos, data_size);

                    if (walker.compare(value))
                    {
                        return walker.idx(local_idx);
                    }
                    else {
                        local_pos += length;
                        walker.next();
                    }
                }

                return walker.idx(size);
            }
            else {
                return walker.idx(size);
            }
        }
    }



    template <typename Walker>
    auto walk_fw(Int block, Int start, Int size, Walker&& walker) const
    {
        auto values = this->values(block);

        Int data_size = this->data_size(block);
        TreeLayout layout = this->compute_tree_layout(data_size);

        auto lr = this->locate(layout, values, block, start);

        size_t pos = lr.idx;

        if (pos < data_size)
        {
            Codec codec;

            if (layout.levels_max < 0 || data_size - pos  < ValuesPerBranch)
            {
                for (Int c = start; pos < data_size; c++)
                {
                    Value value;
                    auto len = codec.decode(values, value, pos, data_size);

                    if (walker.compare(value))
                    {
                        return walker.idx(c);
                    }
                    else {
                        pos += len;
                        walker.next();
                    }
                }

                return walker.idx(size);
            }
            else {
                WalkerState state;

                state.size_sum = start;

                size_t window_end = (pos | ValuesPerBranchMask) + 1;

                Int c = start;
                for (c = start; pos < window_end; c++)
                {
                    Value value;
                    auto len = codec.decode(values, value, pos, data_size);

                    if (walker.compare(value))
                    {
                        return walker.idx(c);
                    }
                    else {
                        state.size_sum++;
                        pos += len;
                        walker.next();
                    }
                }

                Int idx = this->walk_index_fw(
                        layout,
                        state,
                        window_end >> ValuesPerBranchLog2,
                        layout.levels_max,
                        std::forward<Walker>(walker)
                );

                if (idx >= 0)
                {
                    size_t local_pos = (idx << ValuesPerBranchLog2) + this->offset(block, idx);

                    for (Int local_idx = state.size_sum; local_pos < data_size; local_idx++)
                    {
                        Value value;
                        size_t length = codec.decode(values, value, local_pos, data_size);

                        if (walker.compare(value))
                        {
                            return walker.idx(local_idx);
                        }
                        else {
                            local_pos += length;
                            walker.next();
                        }
                    }

                    return walker.idx(size);
                }
                else {
                    return walker.idx(size);
                }
            }
        }
        else {
            return walker.idx(size);
        }
    }


    template <typename Walker>
    auto walk_bw(Int block, Int start, Walker&& walker) const
    {
        auto values = this->values(block);
        auto offsets = this->offsets(block);

        size_t data_size = this->data_size(block);

        Codec codec;

        TreeLayout layout = this->compute_tree_layout(data_size);

        auto lr = this->locate(layout, values, block, start);

        size_t pos = lr.idx;

        ValueT value_data[ValuesPerBranch];

        if (pos < ValuesPerBranch)
        {
            size_t local_pos = 0;

            for (Int c = 0; c <= start; c++)
            {
                Value value;
                local_pos += codec.decode(values, value, local_pos, data_size);
                value_data[c] = value;
            }

            for (Int c = start; c >= 0; c--)
            {
                if (walker.compare(value_data[c]))
                {
                    return walker.idx(c);
                }
                else {
                    walker.next();
                }
            }

            return walker.idx(-1);
        }
        else {
            size_t window_start = (pos & ~ValuesPerBranchMask) + this->offset(offsets, pos >> ValuesPerBranchLog2);
            size_t window_end   = (pos | ValuesPerBranchMask) + 1;

            if (window_end > data_size) {
                window_end = data_size;
            }

            size_t local_pos = window_start;

            Int local_c;
            for (local_c = 0; local_pos <= pos && local_c <= start; local_c++)
            {
                Value value;
                local_pos += codec.decode(values, value, local_pos, data_size);
                value_data[local_c] = value;
            }

            Int window_size_prefix = lr.index_cnt;

            for (Int c = local_c - 1; c >= 0; c--)
            {
                if (walker.compare(value_data[c]))
                {
                    return walker.idx(c + window_size_prefix);
                }
                else {
                    walker.next();
                }
            }

            WalkerState state;
            state.size_sum = lr.index_cnt;

            Int idx = this->walk_index_bw(
                    layout,
                    state,
                    (window_start >> ValuesPerBranchLog2) - 1,
                    layout.levels_max,
                    std::forward<Walker>(walker)
            );

            if (idx >= 0)
            {
                Int pos2 = idx << ValuesPerBranchLog2;

                size_t window_start = (pos2 & ~ValuesPerBranchMask) + this->offset(offsets, pos2 >> ValuesPerBranchLog2);
                size_t window_end   = (pos2 | ValuesPerBranchMask) + 1;

                size_t local_pos = window_start;

                Int c;
                for (c = 0; local_pos < window_end; c++)
                {
                    Value value;
                    local_pos += codec.decode(values, value, local_pos, data_size);
                    value_data[c] = value;
                }

                state.size_sum -= c;

                c--;
                for (; c >= 0; c--)
                {
                    if (walker.compare(value_data[c]))
                    {
                        return walker.idx(c + state.size_sum);
                    }
                    else {
                        walker.next();
                    }
                }

                return walker.idx(-1);
            }
            else {
                return walker.idx(-1);
            }
        }
    }





    IndexValue gsum(Int block) const
    {
        auto meta = this->metadata();
        TreeLayout layout = this->compute_tree_layout(this->data_size(block));

        return gsum(layout, meta, block, meta->size());
    }



    IndexValue gsum(Int block, Int end) const
    {
        auto meta = this->metadata();
        TreeLayout layout = this->compute_tree_layout(this->data_size(block));

        return gsum(layout, meta, block, end);
    }

    IndexValue plain_gsum(Int block, Int end) const
    {
        IndexValue sum = 0;

        Int size = this->size();

        Int limit = end <= size ? end : size;

        auto values = this->values(block);

        Codec codec;

        size_t pos = 0;
        for (int c = 0; c < limit; c++)
        {
            Value value;
            auto len = codec.decode(values, value, pos);
            sum += value;
            pos += len;
        }

        return sum;
    }

    IndexValue gsum(Int block, Int start, Int end) const
    {
        TreeLayout layout = this->compute_tree_layout(this->data_size(block));

        return gsum(layout, this->metadata(), block, start, end);
    }

    IndexValue gsum(TreeLayout& layout, const Metadata* meta, Int block, Int end) const
    {
        if (end < meta->size())
        {
            auto* values = this->values(block);
            auto lr = this->locate_with_sum(layout, values, block, end);
            return lr.value_sum;
        }
        else if (has_index(block))
        {
            auto index = this->value_index(block);
            return index[0];
        }
        else {
            auto* values = this->values(block);

            Codec codec;
            size_t pos = 0;
            size_t data_size = this->data_size(block);

            IndexValue sum = 0;

            for(int c  = 0; pos < data_size && c < end; c++)
            {
                Value value;
                pos += codec.decode(values, value, pos, data_size);
                sum += value;
            }

            return sum;
        }
    }

    IndexValue gsum(TreeLayout& layout, const Metadata* meta, Int block, Int start, Int end) const
    {
        auto end_sum = gsum(layout, meta, block, end);
        auto start_sum = gsum(layout, meta, block, start);

        return end_sum - start_sum;
    }


    void reindex(Int blocks)
    {
        for (Int block = 0; block < blocks; block++)
        {
            Int data_size = this->data_size(block);
            TreeLayout layout = this->compute_tree_layout(data_size);

            this->reindex_block(block, layout);
        }
    }


    void check(Int blocks) const
    {
        for (Int block = 0; block < blocks; block++)
        {
            Int data_size = this->data_size(block);
            TreeLayout layout = this->compute_tree_layout(data_size);

            this->check_block(block, layout);
        }
    }


    void dump_index(Int blocks, std::ostream& out = cout) const
    {
        auto meta = this->metadata();

        out<<"size_         = "<<meta->size()<<std::endl;

        for (Int block = 0; block < blocks; block++)
        {
            out<<"++++++++++++++++++ Block: "<<block<<" ++++++++++++++++++"<<endl;

            auto data_size  = this->data_size(block);
            auto index_size = this->index_size(data_size);

            out<<"index_size_   = "<<index_size<<std::endl;

            TreeLayout layout = this->compute_tree_layout(data_size);

            if (layout.levels_max >= 0)
            {
                out<<"TreeLayout: "<<endl;

                out<<"Level sizes: ";
                for (Int c = 0; c <= layout.levels_max; c++) {
                    out<<layout.level_sizes[c]<<" ";
                }
                out<<endl;

                out<<"Level starts: ";
                for (Int c = 0; c <= layout.levels_max; c++) {
                    out<<layout.level_starts[c]<<" ";
                }
                out<<endl;
                out<<std::endl;

                auto value_indexes = this->value_index(block);
                auto size_indexes = this->size_index(block);

                out<<"Index:"<<endl;
                for (Int c = 0; c < index_size; c++)
                {
                    out<<c<<": "<<value_indexes[c]<<" "<<size_indexes[c]<<std::endl;
                }
            }
        }
    }

    void dump_block(Int block, std::ostream& out = std::cout) const
    {
        out<<"Dump values"<<std::endl;
        Codec codec;
        size_t pos = 0;

        auto values     = this->values(block);
        auto data_size  = this->data_size(block);

        for(Int c = 0; pos < data_size; c++)
        {
            ValueT value;
            auto len = codec.decode(values, value, pos);

            out<<c<<": "<<pos<<" "<<value<<std::endl;

            pos += len;
        }

        out<<std::endl;
    }






protected:
    void reindex_block(Int block)
    {
        TreeLayout layout = this->compute_tree_layout(this->data_size(block));
        reindex_block(block, layout);
    }

    void reindex_block(Int block, TreeLayout& layout)
    {
        if (layout.levels_max >= 0)
        {
            auto values     = this->values(block);
            auto indexes    = this->value_index(block);
            auto size_index = this->size_index(block);
            auto offsets    = this->offsets(block);

            this->clear(block * SegmentsPerBlock + Base::VALUE_INDEX + BlocksStart);
            this->clear(block * SegmentsPerBlock + Base::SIZE_INDEX + BlocksStart);
            this->clear(block * SegmentsPerBlock + Base::OFFSETS + BlocksStart);

            layout.indexes = indexes;
            layout.valaue_block_size_prefix = size_index;

            Int levels = layout.levels_max + 1;

            Int level_start = layout.level_starts[levels - 1];

            Int data_size = this->data_size(block);

            Codec codec;

            size_t pos = 0;
            IndexValueT value_sum = 0;
            Int size_cnt = 0;
            size_t threshold = ValuesPerBranch;

            set_offset(offsets, 0, 0);

            Int idx = 0;
            while(pos < data_size)
            {
                if (pos >= threshold)
                {
                    set_offset(offsets, idx + 1, pos - threshold);

                    indexes[level_start + idx] = value_sum;
                    size_index[level_start + idx] = size_cnt;

                    threshold += ValuesPerBranch;

                    idx++;

                    value_sum = 0;
                    size_cnt  = 0;
                }

                Value value;
                auto len = codec.decode(values, value, pos, data_size);

                value_sum += value;
                size_cnt++;

                pos += len;
            }

            indexes[level_start + idx] = value_sum;
            size_index[level_start + idx] = size_cnt;

            for (Int level = levels - 1; level > 0; level--)
            {
                Int previous_level_start = layout.level_starts[level - 1];
                Int previous_level_size  = layout.level_sizes[level - 1];

                Int current_level_start  = layout.level_starts[level];

                Int current_level_size = layout.level_sizes[level];

                for (int i = 0; i < previous_level_size; i++)
                {
                    IndexValue sum = 0;
                    Int sizes_sum  = 0;

                    Int start       = (i << BranchingFactorLog2) + current_level_start;
                    Int window_end  = ((i + 1) << BranchingFactorLog2);

                    Int end = (window_end <= current_level_size ? window_end : current_level_size) + current_level_start;

                    for (Int c = start; c < end; c++) {
                        sum += indexes[c];
                        sizes_sum += size_index[c];
                    }

                    indexes[previous_level_start + i] = sum;
                    size_index[previous_level_start + i] = sizes_sum;
                }
            }
        }
        else {
            this->clear(block * SegmentsPerBlock + Base::OFFSETS + BlocksStart);
        }
    }


    void check_block(Int block, TreeLayout& layout) const
    {
        Int data_size    = this->data_size(block);
        Int offsets_size = this->element_size(block * SegmentsPerBlock + Base::OFFSETS + BlocksStart);



        if (layout.levels_max >= 0)
        {
            MEMORIA_ASSERT(this->element_size(block * SegmentsPerBlock + VALUE_INDEX + BlocksStart), >, 0);
            MEMORIA_ASSERT(this->element_size(block * SegmentsPerBlock + SIZE_INDEX + BlocksStart), >, 0);

            auto values     = this->values(block);
            auto indexes    = this->value_index(block);
            auto size_index = this->size_index(block);
            auto offsets    = this->offsets(block);

            layout.indexes = indexes;
            layout.valaue_block_size_prefix = size_index;

            Int levels = layout.levels_max + 1;

            Int level_start = layout.level_starts[levels - 1];

            Codec codec;

            size_t pos = 0;
            IndexValueT value_sum = 0;
            Int size_cnt = 0;
            size_t threshold = ValuesPerBranch;
            Int total_size = 0;

            MEMORIA_ASSERT(offset(offsets, 0), ==, 0);

            Int idx = 0;
            while(pos < data_size)
            {
                if (pos >= threshold)
                {
                    MEMORIA_ASSERT(offset(offsets, idx + 1), ==, pos - threshold);

                    MEMORIA_ASSERT(indexes[level_start + idx], ==, value_sum);
                    MEMORIA_ASSERT(size_index[level_start + idx], ==, size_cnt);

                    threshold += ValuesPerBranch;

                    idx++;

                    total_size += size_cnt;

                    value_sum = 0;
                    size_cnt  = 0;
                }

                Value value;
                auto len = codec.decode(values, value, pos, data_size);

                value_sum += value;
                size_cnt++;

                pos += len;
            }

            MEMORIA_ASSERT((Int)pos, ==, data_size);

            MEMORIA_ASSERT(indexes[level_start + idx], ==, value_sum);
            MEMORIA_ASSERT(size_index[level_start + idx], ==, size_cnt);

            MEMORIA_ASSERT(this->size(), ==, size_cnt + total_size);

            for (Int level = levels - 1; level > 0; level--)
            {
                Int previous_level_start = layout.level_starts[level - 1];
                Int previous_level_size  = layout.level_sizes[level - 1];

                Int current_level_start  = layout.level_starts[level];

                Int current_level_size = layout.level_sizes[level];

                for (int i = 0; i < previous_level_size; i++)
                {
                    IndexValue sum = 0;
                    Int sizes_sum  = 0;

                    Int start       = (i << BranchingFactorLog2) + current_level_start;
                    Int window_end  = ((i + 1) << BranchingFactorLog2);

                    Int end = (window_end <= current_level_size ? window_end : current_level_size) + current_level_start;

                    for (Int c = start; c < end; c++) {
                        sum += indexes[c];
                        sizes_sum += size_index[c];
                    }

                    MEMORIA_ASSERT(indexes[previous_level_start + i], ==, sum);
                    MEMORIA_ASSERT(size_index[previous_level_start + i], ==, sizes_sum);
                }
            }
        }
        else {
            MEMORIA_ASSERT(this->element_size(block * SegmentsPerBlock + VALUE_INDEX + BlocksStart), ==, 0);
            MEMORIA_ASSERT(this->element_size(block * SegmentsPerBlock + SIZE_INDEX + BlocksStart), ==, 0);

            if (data_size > 0)
            {
                MEMORIA_ASSERT(offsets_size, ==, sizeof(OffsetsType));
                MEMORIA_ASSERT(this->offset(block, 0), ==, 0);
            }
            else {
                MEMORIA_ASSERT(offsets_size, ==, 0);
            }

            MEMORIA_ASSERT(this->data_size(block), <=, kValuesPerBranch);
        }
    }
};

}}