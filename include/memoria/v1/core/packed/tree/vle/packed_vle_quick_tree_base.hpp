
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

#include <memoria/v1/core/packed/tree/vle/packed_vle_quick_tree_base_base.hpp>
#include <memoria/v1/core/tools/assert.hpp>

namespace memoria {
namespace v1 {

class PkdVQTreeMetadata {
    int32_t size_;

public:
    PkdVQTreeMetadata() = default;

    int32_t& size() {return size_;}
    const int32_t& size() const {return size_;}

    template <typename, int32_t, int32_t, int32_t, typename> friend class PkdVQTreeBase1;
    template <typename, typename, template <typename> class Codec, int32_t, int32_t> friend class PkdVQTreeBase;
};



template <
    typename IndexValueT,
    typename ValueT,
    template <typename> class CodecT,
    int32_t kBranchingFactor,
    int32_t kValuesPerBranch
>
class PkdVQTreeBase: public PkdVQTreeBaseBase<IndexValueT, kBranchingFactor, kValuesPerBranch, 4, PkdVQTreeMetadata> {

    using Base      = PkdVQTreeBaseBase<IndexValueT, kBranchingFactor, kValuesPerBranch, 4, PkdVQTreeMetadata>;
    using MyType    = PkdVQTreeBase<IndexValueT, ValueT, CodecT, kBranchingFactor, kValuesPerBranch>;

public:
    static constexpr uint32_t VERSION = 1;

    using IndexValue    = IndexValueT;
    using Value         = ValueT;

    using typename Base::Metadata;
    using typename Base::TreeLayout;

    using OffsetsType   = uint64_t;

    using Codec         = CodecT<Value>;

    using ValueData = typename Codec::BufferType;

    static const int32_t BranchingFactor        = kBranchingFactor;
    static const int32_t ValuesPerBranch        = kValuesPerBranch;

    static const bool FixedSizeElement      = false;

    static constexpr int32_t ValuesPerBranchMask    = ValuesPerBranch - 1;
    static constexpr int32_t BranchingFactorMask    = BranchingFactor - 1;

    static constexpr int32_t ValuesPerBranchLog2    = Log2(ValuesPerBranch) - 1;
    static constexpr int32_t BranchingFactorLog2    = Log2(BranchingFactor) - 1;

    static constexpr int32_t SegmentsPerBlock = 4;


    static constexpr int32_t BITS_PER_OFFSET        = Codec::BitsPerOffset;
    static constexpr int32_t BITS_PER_DATA_VALUE    = Codec::ElementSize;

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
        int32_t blocks_;

        InitFn(int32_t blocks): blocks_(blocks) {}

        int32_t block_size(int32_t items_number) const {
            return MyType::block_size_equi(blocks_, items_number);
        }

        int32_t max_elements(int32_t block_size)
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
                ConstValue<uint32_t, VERSION>,
                Value
    >;

    static constexpr int32_t value_segment_size(int32_t values)
    {
        return PackedAllocatable::roundUpBitsToAlignmentBlocks(values * BITS_PER_DATA_VALUE);
    }

    static constexpr int32_t number_of_offsets(int32_t values)
    {
        //FIXME: Number of offsets cant be zero
        return Base::divUpV(values);
    }

    static constexpr int32_t offsets_segment_size(int32_t values)
    {
        return PackedAllocator::roundUpBitsToAlignmentBlocks(number_of_offsets(values) * BITS_PER_OFFSET);
    }

    static constexpr int32_t index1_segment_size(int32_t index_size) {
        return PackedAllocator::roundUpBytesToAlignmentBlocks(index_size * sizeof(IndexValue));
    }

    static constexpr int32_t index2_segment_size(int32_t index_size) {
        return PackedAllocator::roundUpBytesToAlignmentBlocks(index_size * sizeof(int));
    }


//    void init(int32_t data_block_size, int32_t blocks)
//    {
//      Base::init(data_block_size, blocks * SegmentsPerBlock + BlocksStart);
//
//      Metadata* meta = this->template allocate<Metadata>(METADATA);
//      this->template allocateArrayBySize<int32_t>(DATA_SIZES, blocks);
//
//      meta->size()        = 0;
//
//      int32_t max_size        = FindTotalElementsNumber3(data_block_size, InitFn(blocks));
//      int32_t offsets_size    = offsets_segment_size(max_size);
//
//      int32_t values_segment_length = this->value_segment_size(max_size);
//      int32_t index_size      = this->index_size(max_size);
//
//      for (int32_t block = 0; block < blocks; block++)
//      {
//          this->template allocateArrayBySize<IndexValue>(block * SegmentsPerBlock + VALUE_INDEX + BlocksStart, index_size);
//          this->template allocateArrayBySize<int32_t>(block * SegmentsPerBlock + SIZE_INDEX + BlocksStart, index_size);
//          this->template allocateArrayBySize<int8_t>(block * SegmentsPerBlock + OFFSETS + BlocksStart, offsets_size);
//          this->template allocateArrayBySize<int8_t>(block * SegmentsPerBlock + VALUES + BlocksStart, values_segment_length);
//      }
//    }

    void init_tl(int32_t data_block_size, int32_t blocks)
    {
        Base::init(data_block_size, blocks * SegmentsPerBlock + BlocksStart);

        Metadata* meta = this->template allocate<Metadata>(METADATA);
        this->template allocateArrayBySize<int32_t>(DATA_SIZES, blocks);

        meta->size()        = 0;

        int32_t max_size        = 0;
        int32_t offsets_size    = offsets_segment_size(max_size);

        int32_t values_segment_length = this->value_segment_size(max_size);
        int32_t index_size      = this->index_size(max_size);

        for (int32_t block = 0; block < blocks; block++)
        {
            this->template allocateArrayBySize<IndexValue>(block * SegmentsPerBlock + VALUE_INDEX + BlocksStart, index_size);
            this->template allocateArrayBySize<int32_t>(block * SegmentsPerBlock + SIZE_INDEX + BlocksStart, index_size);
            this->template allocateArrayBySize<int8_t>(block * SegmentsPerBlock + OFFSETS + BlocksStart, offsets_size);
            this->template allocateArrayBySize<int8_t>(block * SegmentsPerBlock + VALUES + BlocksStart, values_segment_length);
        }
    }

//    void init(int32_t blocks)
//    {
//      int32_t block_size = MyType::tree_size(blocks, 0);
//      Base::init(block_size, blocks * SegmentsPerBlock + BlocksStart);
//
//      Metadata* meta = this->template allocate<Metadata>(Base::METADATA);
//      this->template allocateArrayBySize<int32_t>(Base::DATA_SIZES, blocks);
//
//      meta->size() = 0;
//
//      for (int32_t block = 0; block < blocks; block++)
//      {
//          this->template allocateArrayBySize<IndexValue>(block * SegmentsPerBlock + VALUE_INDEX + BlocksStart, 0);
//          this->template allocateArrayBySize<Value>(block * SegmentsPerBlock + SIZE_INDEX + BlocksStart, 0);
//          this->template allocateArrayBySize<int8_t>(block * SegmentsPerBlock + OFFSETS + BlocksStart, 0);
//          this->template allocateArrayBySize<ValueData>(block * SegmentsPerBlock + VALUES + BlocksStart, 0);
//      }
//    }



    //FIXME: invalid block size calculation by capacity
    static int32_t block_size_equi(int32_t blocks, int32_t capacity)
    {
        int32_t metadata_length = Base::roundUpBytesToAlignmentBlocks(sizeof(Metadata));

        int32_t data_sizes_length = Base::roundUpBytesToAlignmentBlocks(blocks * sizeof(int32_t));

        int32_t index_size      = MyType::index_size(capacity);
        int32_t index_length    = Base::roundUpBytesToAlignmentBlocks(index_size * sizeof(IndexValue));
        int32_t sizes_length    = Base::roundUpBytesToAlignmentBlocks(index_size * sizeof(int32_t));

        int32_t values_length   = Base::roundUpBitsToAlignmentBlocks(capacity * BITS_PER_DATA_VALUE);

        int32_t offsets_length  = offsets_segment_size(capacity);

        int32_t  blocks_length  = index_length + values_length + offsets_length + sizes_length;

        return Base::block_size(
                metadata_length +
                data_sizes_length +
                blocks_length * blocks,
                blocks * SegmentsPerBlock + BlocksStart
        );
    }




    int32_t block_size() const
    {
        return Base::block_size();
    }

    static int32_t index_size(int32_t capacity)
    {
        TreeLayout layout;
        Base::compute_tree_layout(capacity, layout);
        return layout.index_size;
    }

    static int32_t tree_size(int32_t blocks, int32_t block_size)
    {
        return block_size >= (int32_t)sizeof(Value) ? FindTotalElementsNumber3(block_size, InitFn(blocks)) : 0;
    }


    int32_t offset(int32_t block, int32_t idx) const
    {
        return GetBits(offsets(block), idx * BITS_PER_OFFSET, BITS_PER_OFFSET);
    }

    int32_t offset(const OffsetsType* block, int32_t idx) const
    {
        return GetBits(block, idx * BITS_PER_OFFSET, BITS_PER_OFFSET);
    }

    void set_offset(int32_t block, int32_t idx, int32_t value)
    {
        SetBits(offsets(block), idx * BITS_PER_OFFSET, value, BITS_PER_OFFSET);
    }

    void set_offset(OffsetsType* block, int32_t idx, int32_t value)
    {
        SetBits(block, idx * BITS_PER_OFFSET, value, BITS_PER_OFFSET);
    }

    OffsetsType* offsets(int32_t block) {
        return this->template get<OffsetsType>(block * SegmentsPerBlock + OFFSETS + BlocksStart);
    }

    const OffsetsType* offsets(int32_t block) const {
        return this->template get<OffsetsType>(block * SegmentsPerBlock + OFFSETS + BlocksStart);
    }

    ValueData* values(int32_t block) {
        return this->template get<ValueData>(block * SegmentsPerBlock + VALUES + BlocksStart);
    }
    const ValueData* values(int32_t block) const {
        return this->template get<ValueData>(block * SegmentsPerBlock + VALUES + BlocksStart);
    }



    bool has_index(int32_t block) const {
        return this->element_size(block * SegmentsPerBlock + VALUE_INDEX + BlocksStart) > 0;
    }


    struct FindGEWalker {
        IndexValue sum_ = 0;
        IndexValue target_;

        IndexValue next_;

        int32_t idx_;

    public:
        FindGEWalker(const IndexValue& target): target_(target) {}

        template <typename T>
        bool compare(T value)
        {
            next_ = value;
            return sum_ + next_ >= target_;
        }

        void next() {
            sum_ += next_;
        }

        int32_t& idx() {return idx_;}
        const int32_t& idx() const {return idx_;}

        FindGEWalker& idx(int32_t idx) {
            idx_ = idx;
            return *this;
        }

        IndexValue prefix() const {
            return sum_;
        }

        FindGEWalker& adjust(int32_t base, int32_t size) {
            idx_ -= base;

            if (idx_ > size) {
                idx_ = size;
            }

            return *this;
        }

        FindGEWalker& adjust(int32_t base, int32_t size, IndexValue sum) {
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


        FindGEWalker& adjust_s(int32_t base, int32_t size, IndexValue sum) {
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

        int32_t idx_;
    public:
        FindGTWalker(const IndexValue& target): target_(target) {}

        template <typename T>
        bool compare(T value)
        {
            next_ = value;
            return sum_ + next_ > target_;
        }

        void next() {
            sum_ += next_;
        }

        int32_t& idx() {return idx_;}
        const int32_t& idx() const {return idx_;}

        FindGTWalker& idx(int32_t idx) {
            idx_ = idx;
            return *this;
        }

        IndexValue prefix() const {
            return sum_;
        }

        FindGTWalker& adjust(int32_t base, int32_t size) {
            idx_ -= base;

            if (idx_ > size) {
                idx_ = size;
            }

            return *this;
        }

        FindGTWalker& adjust(int32_t base, int32_t size, IndexValue sum) {
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

        FindGTWalker& adjust_s(int32_t base, int32_t size, IndexValue sum) {
            idx_ -= base;

            if (idx_ > size) {
                idx_ = size;
            }

            this->sum_ -= sum;

            return *this;
        }
    };



    auto gfind_ge(int32_t block, const IndexValue& value) const
    {
        return find(block, FindGEWalker(value));
    }

    auto gfind_gt(int32_t block, const IndexValue& value) const
    {
        return find(block, FindGTWalker(value));
    }

    auto gfind_ge_fw(int32_t block, int32_t start, const IndexValue& value) const
    {
        return walk_fw(block, start, FindGEWalker(value));
    }

    auto gfind_gt_fw(int32_t block, int32_t start, const IndexValue& value) const
    {
        return walk_fw(block, start, FindGTWalker(value));
    }


    auto gfind_ge_bw(int32_t block, int32_t start, const IndexValue& value) const
    {
        return walk_bw(block, start, FindGEWalker(value));
    }

    auto gfind_gt_bw(int32_t block, int32_t start, const IndexValue& value) const
    {
        return walk_bw(block, start, FindGTWalker(value));
    }

    class Location {
        int32_t window_start_;
        int32_t size_prefix_;
        int32_t pos_;
    public:
        Location(int32_t window_start, int32_t size_prefix, int32_t pos):
            window_start_(window_start), size_prefix_(size_prefix), pos_(pos)
        {}

        int32_t window_start() const {return window_start_;}
        int32_t size_prefix() const {return size_prefix_;}
        int32_t pos() const {return pos_;}
    };

    int32_t locate(int32_t block, int32_t idx) const
    {
        auto values = this->values(block);
        TreeLayout layout = Base::compute_tree_layout(this->data_size(block));

        return locate(layout, values, block, idx).idx;
    }

    LocateResult locate(TreeLayout& layout, const ValueData* values, int32_t block, int32_t idx) const
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

            int32_t window_num = locate_result.idx;

            int32_t window_start = window_num << ValuesPerBranchLog2;
            if (window_start >= 0)
            {
                Codec codec;

                size_t offset = this->offset(block, window_num);

                int32_t c = 0;
                int32_t local_idx = idx - locate_result.index_cnt;
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


    LocateResult locate_with_sum(TreeLayout& layout, const ValueData* values, int32_t block, int32_t idx) const
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

            int32_t window_num = locate_result.idx;

            int32_t window_start = (window_num << ValuesPerBranchLog2);
            if (window_start >= 0)
            {
                Codec codec;

                size_t offset = this->offset(block, window_num);

                int32_t c = 0;
                int32_t local_idx = idx - locate_result.index_cnt;
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
    auto find(int32_t block, Walker&& walker) const
    {
        auto metadata = this->metadata();
        auto values = this->values(block);

        size_t data_size = this->data_size(block);
        int32_t size = metadata->size();

        Codec codec;

        if (!this->has_index(block))
        {
            size_t pos = 0;
            for (int32_t c = 0; pos < data_size; c++)
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

            int32_t idx = this->find_index(data, state, walker);

            if (idx >= 0)
            {
                size_t local_pos = (idx << ValuesPerBranchLog2) + this->offset(block, idx);

                for (int32_t local_idx = state.size_sum; local_pos < data_size; local_idx++)
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
    auto walk_fw(int32_t block, int32_t start, int32_t size, Walker&& walker) const
    {
        auto values = this->values(block);

        int32_t data_size = this->data_size(block);
        TreeLayout layout = this->compute_tree_layout(data_size);

        auto lr = this->locate(layout, values, block, start);

        size_t pos = lr.idx;

        if (pos < data_size)
        {
            Codec codec;

            if (layout.levels_max < 0 || data_size - pos  < ValuesPerBranch)
            {
                for (int32_t c = start; pos < data_size; c++)
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

                int32_t c = start;
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

                int32_t idx = this->walk_index_fw(
                        layout,
                        state,
                        window_end >> ValuesPerBranchLog2,
                        layout.levels_max,
                        std::forward<Walker>(walker)
                );

                if (idx >= 0)
                {
                    size_t local_pos = (idx << ValuesPerBranchLog2) + this->offset(block, idx);

                    for (int32_t local_idx = state.size_sum; local_pos < data_size; local_idx++)
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
    auto walk_bw(int32_t block, int32_t start, Walker&& walker) const
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

            for (int32_t c = 0; c <= start; c++)
            {
                Value value;
                local_pos += codec.decode(values, value, local_pos, data_size);
                value_data[c] = value;
            }

            for (int32_t c = start; c >= 0; c--)
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

            int32_t local_c;
            for (local_c = 0; local_pos <= pos && local_c <= start; local_c++)
            {
                Value value;
                local_pos += codec.decode(values, value, local_pos, data_size);
                value_data[local_c] = value;
            }

            int32_t window_size_prefix = lr.index_cnt;

            for (int32_t c = local_c - 1; c >= 0; c--)
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

            int32_t idx = this->walk_index_bw(
                    layout,
                    state,
                    (window_start >> ValuesPerBranchLog2) - 1,
                    layout.levels_max,
                    std::forward<Walker>(walker)
            );

            if (idx >= 0)
            {
                int32_t pos2 = idx << ValuesPerBranchLog2;

                size_t window_start = (pos2 & ~ValuesPerBranchMask) + this->offset(offsets, pos2 >> ValuesPerBranchLog2);
                size_t window_end   = (pos2 | ValuesPerBranchMask) + 1;

                size_t local_pos = window_start;

                int32_t c;
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





    IndexValue gsum(int32_t block) const
    {
        auto meta = this->metadata();
        TreeLayout layout = this->compute_tree_layout(this->data_size(block));

        return gsum(layout, meta, block, meta->size());
    }



    IndexValue gsum(int32_t block, int32_t end) const
    {
        auto meta = this->metadata();
        TreeLayout layout = this->compute_tree_layout(this->data_size(block));

        return gsum(layout, meta, block, end);
    }

    IndexValue plain_gsum(int32_t block, int32_t end) const
    {
        IndexValue sum = 0;

        int32_t size = this->size();

        int32_t limit = end <= size ? end : size;

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

    IndexValue gsum(int32_t block, int32_t start, int32_t end) const
    {
        TreeLayout layout = this->compute_tree_layout(this->data_size(block));

        return gsum(layout, this->metadata(), block, start, end);
    }

    IndexValue gsum(TreeLayout& layout, const Metadata* meta, int32_t block, int32_t end) const
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

    IndexValue gsum(TreeLayout& layout, const Metadata* meta, int32_t block, int32_t start, int32_t end) const
    {
        auto end_sum = gsum(layout, meta, block, end);
        auto start_sum = gsum(layout, meta, block, start);

        return end_sum - start_sum;
    }


    void reindex(int32_t blocks)
    {
        for (int32_t block = 0; block < blocks; block++)
        {
            int32_t data_size = this->data_size(block);
            TreeLayout layout = this->compute_tree_layout(data_size);

            this->reindex_block(block, layout);
        }
    }


    void check(int32_t blocks) const
    {
        for (int32_t block = 0; block < blocks; block++)
        {
            int32_t data_size = this->data_size(block);
            TreeLayout layout = this->compute_tree_layout(data_size);

            this->check_block(block, layout);
        }
    }


    void dump_index(int32_t blocks, std::ostream& out = std::cout) const
    {
        auto meta = this->metadata();

        out << "size_         = " << meta->size() << std::endl;

        for (int32_t block = 0; block < blocks; block++)
        {
            out << "++++++++++++++++++ Block: " << block << " ++++++++++++++++++" << std::endl;

            auto data_size  = this->data_size(block);
            auto index_size = this->index_size(data_size);

            out << "index_size_   = " << index_size << std::endl;

            TreeLayout layout = this->compute_tree_layout(data_size);

            if (layout.levels_max >= 0)
            {
                out << "TreeLayout: " << std::endl;

                out<<"Level sizes: ";
                for (int32_t c = 0; c <= layout.levels_max; c++) {
                    out << layout.level_sizes[c] << " ";
                }
                out << std::endl;

                out << "Level starts: ";
                for (int32_t c = 0; c <= layout.levels_max; c++) {
                    out << layout.level_starts[c] << " ";
                }
                out << std::endl;
                out << std::endl;

                auto value_indexes = this->value_index(block);
                auto size_indexes = this->size_index(block);

                out << "Index:" << std::endl;
                for (int32_t c = 0; c < index_size; c++)
                {
                    out << c << ": " << value_indexes[c] << " " << size_indexes[c] << std::endl;
                }
            }
        }
    }

    void dump_block(int32_t block, std::ostream& out = std::cout) const
    {
        out << "Dump values" << std::endl;
        Codec codec;
        size_t pos = 0;

        auto values     = this->values(block);
        auto data_size  = this->data_size(block);

        for(int32_t c = 0; pos < data_size; c++)
        {
            ValueT value;
            auto len = codec.decode(values, value, pos);

            out << c << ": " << pos << " " << value << std::endl;

            pos += len;
        }

        out << std::endl;
    }






protected:
    void reindex_block(int32_t block)
    {
        TreeLayout layout = this->compute_tree_layout(this->data_size(block));
        reindex_block(block, layout);
    }

    void reindex_block(int32_t block, TreeLayout& layout)
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

            int32_t levels = layout.levels_max + 1;

            int32_t level_start = layout.level_starts[levels - 1];

            int32_t data_size = this->data_size(block);

            Codec codec;

            size_t pos = 0;
            IndexValueT value_sum = 0;
            int32_t size_cnt = 0;
            size_t threshold = ValuesPerBranch;

            set_offset(offsets, 0, 0);

            int32_t idx = 0;
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

            for (int32_t level = levels - 1; level > 0; level--)
            {
                int32_t previous_level_start = layout.level_starts[level - 1];
                int32_t previous_level_size  = layout.level_sizes[level - 1];

                int32_t current_level_start  = layout.level_starts[level];

                int32_t current_level_size = layout.level_sizes[level];

                for (int i = 0; i < previous_level_size; i++)
                {
                    IndexValue sum = 0;
                    int32_t sizes_sum  = 0;

                    int32_t start       = (i << BranchingFactorLog2) + current_level_start;
                    int32_t window_end  = ((i + 1) << BranchingFactorLog2);

                    int32_t end = (window_end <= current_level_size ? window_end : current_level_size) + current_level_start;

                    for (int32_t c = start; c < end; c++) {
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


    void check_block(int32_t block, TreeLayout& layout) const
    {
        int32_t data_size    = this->data_size(block);
        int32_t offsets_size = this->element_size(block * SegmentsPerBlock + Base::OFFSETS + BlocksStart);



        if (layout.levels_max >= 0)
        {
            MEMORIA_V1_ASSERT(this->element_size(block * SegmentsPerBlock + VALUE_INDEX + BlocksStart), >, 0);
            MEMORIA_V1_ASSERT(this->element_size(block * SegmentsPerBlock + SIZE_INDEX + BlocksStart), >, 0);

            auto values     = this->values(block);
            auto indexes    = this->value_index(block);
            auto size_index = this->size_index(block);
            auto offsets    = this->offsets(block);

            layout.indexes = indexes;
            layout.valaue_block_size_prefix = size_index;

            int32_t levels = layout.levels_max + 1;

            int32_t level_start = layout.level_starts[levels - 1];

            Codec codec;

            size_t pos = 0;
            IndexValueT value_sum = 0;
            int32_t size_cnt = 0;
            size_t threshold = ValuesPerBranch;
            int32_t total_size = 0;

            MEMORIA_V1_ASSERT(offset(offsets, 0), ==, 0);

            int32_t idx = 0;
            while(pos < data_size)
            {
                if (pos >= threshold)
                {
                    MEMORIA_V1_ASSERT(offset(offsets, idx + 1), ==, pos - threshold);

                    MEMORIA_V1_ASSERT(indexes[level_start + idx], ==, value_sum);
                    MEMORIA_V1_ASSERT(size_index[level_start + idx], ==, size_cnt);

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

            MEMORIA_V1_ASSERT((int32_t)pos, ==, data_size);

            MEMORIA_V1_ASSERT(indexes[level_start + idx], ==, value_sum);
            MEMORIA_V1_ASSERT(size_index[level_start + idx], ==, size_cnt);

            MEMORIA_V1_ASSERT(this->size(), ==, size_cnt + total_size);

            for (int32_t level = levels - 1; level > 0; level--)
            {
                int32_t previous_level_start = layout.level_starts[level - 1];
                int32_t previous_level_size  = layout.level_sizes[level - 1];

                int32_t current_level_start  = layout.level_starts[level];

                int32_t current_level_size = layout.level_sizes[level];

                for (int i = 0; i < previous_level_size; i++)
                {
                    IndexValue sum = 0;
                    int32_t sizes_sum  = 0;

                    int32_t start       = (i << BranchingFactorLog2) + current_level_start;
                    int32_t window_end  = ((i + 1) << BranchingFactorLog2);

                    int32_t end = (window_end <= current_level_size ? window_end : current_level_size) + current_level_start;

                    for (int32_t c = start; c < end; c++) {
                        sum += indexes[c];
                        sizes_sum += size_index[c];
                    }

                    MEMORIA_V1_ASSERT(indexes[previous_level_start + i], ==, sum);
                    MEMORIA_V1_ASSERT(size_index[previous_level_start + i], ==, sizes_sum);
                }
            }
        }
        else {
            MEMORIA_V1_ASSERT(this->element_size(block * SegmentsPerBlock + VALUE_INDEX + BlocksStart), ==, 0);
            MEMORIA_V1_ASSERT(this->element_size(block * SegmentsPerBlock + SIZE_INDEX + BlocksStart), ==, 0);

            if (data_size > 0)
            {
                MEMORIA_V1_ASSERT(offsets_size, ==, sizeof(OffsetsType));
                MEMORIA_V1_ASSERT(this->offset(block, 0), ==, 0);
            }
            else {
                MEMORIA_V1_ASSERT(offsets_size, ==, 0);
            }

            MEMORIA_V1_ASSERT(this->data_size(block), <=, kValuesPerBranch);
        }
    }
};

}}
