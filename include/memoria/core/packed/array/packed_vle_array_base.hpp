
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

#include <memoria/core/packed/array/packed_vle_array_base_base.hpp>


namespace memoria {

template <int32_t Blocks>
class PkdVDArrayMetadata {
    int32_t size_;

    int32_t data_size_[Blocks];

public:
    PkdVDArrayMetadata() = default;

    int32_t& size() {return size_;}
    const int32_t& size() const {return size_;}

    int32_t& data_size(int32_t block) {return data_size_[block];}
    const int32_t& data_size(int32_t block) const {return data_size_[block];}

    template <int32_t, typename, template <typename> class Codec, int32_t, int32_t, typename> friend class PkdVLEArrayBase;
};



template <
    int32_t Blocks,
    typename ValueT,
    template <typename> class CodecT,
    int32_t kBranchingFactor,
    int32_t kValuesPerBranch,
    typename MetadataT = PkdVDArrayMetadata<Blocks>
>
class PkdVLEArrayBase: public PkdVLEArrayBaseBase<kBranchingFactor, kValuesPerBranch, 3, MetadataT> {

    using Base      = PkdVLEArrayBaseBase<kBranchingFactor, kValuesPerBranch, 3, MetadataT>;
    using MyType    = PkdVLEArrayBase<Blocks, ValueT, CodecT, kBranchingFactor, kValuesPerBranch, MetadataT>;

public:
    static constexpr uint32_t VERSION = 1;

    using typename Base::Metadata;
    using typename Base::TreeLayout;

    using Value         = ValueT;
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

    static constexpr int32_t SegmentsPerBlock = 3;

    static constexpr int32_t BITS_PER_OFFSET        = Codec::BitsPerOffset;
    static constexpr int32_t BITS_PER_DATA_VALUE    = Codec::ElementSize;

    using Base::BlocksStart;

    using Base::METADATA;
    using Base::DATA_SIZES;

    using Base::VALUES;
    using Base::OFFSETS;
    using Base::SIZE_INDEX;

    using typename Base::LocateResult;

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

    PkdVLEArrayBase() = default;

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
        return values > 0 ? Base::divUpV(values) : 1;
    }

    static constexpr int32_t offsets_segment_size(int32_t values)
    {
        return PackedAllocatable::roundUpBitsToAlignmentBlocks(number_of_offsets(values) * BITS_PER_OFFSET);
    }

    static constexpr int32_t index2_segment_size(int32_t index_size) {
        return PackedAllocatable::roundUpBytesToAlignmentBlocks(index_size * sizeof(int));
    }

    VoidResult init_tl(int32_t data_block_size, int32_t blocks) noexcept
    {
        MEMORIA_TRY_VOID(Base::init(data_block_size, blocks * SegmentsPerBlock + BlocksStart));

        MEMORIA_TRY(meta, this->template allocate<Metadata>(METADATA));

        MEMORIA_TRY_VOID(this->template allocateArrayBySize<int32_t>(DATA_SIZES, blocks));

        meta->size() = 0;

        int32_t max_size        = 0;
        int32_t offsets_size    = offsets_segment_size(max_size);

        int32_t values_segment_length = this->value_segment_size(max_size);
        int32_t index_size      = this->index_size(max_size);

        for (int32_t block = 0; block < blocks; block++)
        {
            MEMORIA_TRY_VOID(this->template allocateArrayBySize<int32_t>(block * SegmentsPerBlock + SIZE_INDEX + BlocksStart, index_size));

            MEMORIA_TRY_VOID(this->template allocateArrayBySize<int8_t>(block * SegmentsPerBlock + OFFSETS + BlocksStart, offsets_size));

            MEMORIA_TRY_VOID(this->template allocateArrayBySize<int8_t>(block * SegmentsPerBlock + VALUES + BlocksStart, values_segment_length));
        }

        return VoidResult::of();
    }




    //FIXME: invalid block size calculation by capacity
    static int32_t block_size_equi(int32_t blocks, int32_t total_capacity)
    {
        int32_t metadata_length = PackedAllocatable::roundUpBytesToAlignmentBlocks(sizeof(Metadata));

        int32_t index_size      = MyType::index_size(total_capacity);
        int32_t sizes_length    = PackedAllocatable::roundUpBytesToAlignmentBlocks(index_size * sizeof(int32_t));

        int32_t values_length   = PackedAllocatable::roundUpBitsToAlignmentBlocks(total_capacity * BITS_PER_DATA_VALUE);

        int32_t offsets_length  = offsets_segment_size(total_capacity);

        int32_t  blocks_length  = sizes_length + values_length + offsets_length;

        return Base::block_size(
                metadata_length +
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
        return this->element_size(block * SegmentsPerBlock + SIZE_INDEX + BlocksStart) > 0;
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
        auto data_size = this->data_size(block);

        TreeLayout layout = Base::compute_tree_layout(data_size);

        return locate(layout, values, block, idx, data_size).idx;
    }

    LocateResult locate(TreeLayout& layout, const ValueData* values, int32_t block, int32_t idx, size_t data_size) const
    {
        if (data_size > 0) {

            LocateResult locate_result;

            if (layout.levels_max >= 0)
            {
                layout.valaue_block_size_prefix = this->size_index(block);
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


    VoidResult reindex(int32_t blocks) noexcept
    {
        for (int32_t block = 0; block < blocks; block++)
        {
            int32_t data_size = this->data_size(block);
            TreeLayout layout = this->compute_tree_layout(data_size);

            MEMORIA_TRY_VOID(this->reindex_block(block, layout, data_size));
        }

        return VoidResult::of();
    }


    void check(int32_t blocks) const
    {
        for (int32_t block = 0; block < blocks; block++)
        {
            int32_t data_size = this->data_size(block);
            TreeLayout layout = this->compute_tree_layout(data_size);

            this->check_block(block, layout, data_size);
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
                    out<<layout.level_sizes[c]<<" ";
                }
                out<<std::endl;

                out<<"Level starts: ";
                for (int32_t c = 0; c <= layout.levels_max; c++) {
                    out<<layout.level_starts[c]<<" ";
                }
                out<<std::endl;
                out<<std::endl;

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

        out<<std::endl;
    }






protected:
    VoidResult reindex_block(int32_t block) noexcept
    {
        auto data_size = this->data_size(block);
        TreeLayout layout = this->compute_tree_layout(data_size);
        return reindex_block(block, layout, data_size);
    }

    VoidResult reindex_block(int32_t block, TreeLayout& layout, int32_t data_size) noexcept
    {
        if (layout.levels_max >= 0)
        {
            auto values     = this->values(block);
            auto size_index = this->size_index(block);
            auto offsets    = this->offsets(block);

            this->clear(block * SegmentsPerBlock + Base::SIZE_INDEX + BlocksStart);
            this->clear(block * SegmentsPerBlock + Base::OFFSETS + BlocksStart);

            layout.valaue_block_size_prefix = size_index;

            int32_t levels = layout.levels_max + 1;

            int32_t level_start = layout.level_starts[levels - 1];

            Codec codec;

            size_t pos = 0;
            int32_t size_cnt = 0;
            size_t threshold = ValuesPerBranch;

            set_offset(offsets, 0, 0);

            int32_t idx = 0;
            while(pos < data_size)
            {
                if (pos >= threshold)
                {
                    set_offset(offsets, idx + 1, pos - threshold);

                    size_index[level_start + idx] = size_cnt;

                    threshold += ValuesPerBranch;

                    idx++;

                    size_cnt  = 0;
                }

                auto len = codec.length(values, pos, data_size);

                size_cnt++;

                pos += len;
            }

            size_index[level_start + idx] = size_cnt;

            for (int32_t level = levels - 1; level > 0; level--)
            {
                int32_t previous_level_start = layout.level_starts[level - 1];
                int32_t previous_level_size  = layout.level_sizes[level - 1];

                int32_t current_level_start  = layout.level_starts[level];

                int32_t current_level_size = layout.level_sizes[level];

                for (int i = 0; i < previous_level_size; i++)
                {
                    int32_t sizes_sum  = 0;

                    int32_t start       = (i << BranchingFactorLog2) + current_level_start;
                    int32_t window_end  = ((i + 1) << BranchingFactorLog2);

                    int32_t end = (window_end <= current_level_size ? window_end : current_level_size) + current_level_start;

                    for (int32_t c = start; c < end; c++) {
                        sizes_sum += size_index[c];
                    }

                    size_index[previous_level_start + i] = sizes_sum;
                }
            }
        }
        else {
            // FIXME resize segment to the proper size
            this->clear(block * SegmentsPerBlock + Base::OFFSETS + BlocksStart);
        }

        return VoidResult::of();
    }


    void check_block(int32_t block, TreeLayout& layout, int32_t data_size) const
    {
        int32_t offsets_size = this->element_size(block * SegmentsPerBlock + Base::OFFSETS + BlocksStart);

        if (layout.levels_max >= 0)
        {
            MEMORIA_V1_ASSERT(this->element_size(block * SegmentsPerBlock + SIZE_INDEX + BlocksStart), >, 0);

            auto values     = this->values(block);
            auto size_index = this->size_index(block);
            auto offsets    = this->offsets(block);

            layout.valaue_block_size_prefix = size_index;

            int32_t levels = layout.levels_max + 1;

            int32_t level_start = layout.level_starts[levels - 1];

            Codec codec;

            size_t pos = 0;
            int32_t size_cnt = 0;
            size_t threshold = ValuesPerBranch;

            MEMORIA_V1_ASSERT(offset(offsets, 0), ==, 0);

            int32_t idx = 0;
            while(pos < data_size)
            {
                if (pos >= threshold)
                {
                    MEMORIA_V1_ASSERT(offset(offsets, idx + 1), ==, pos - threshold);
                    MEMORIA_V1_ASSERT(size_index[level_start + idx], ==, size_cnt);

                    threshold += ValuesPerBranch;

                    idx++;
                    size_cnt  = 0;
                }

                auto len = codec.length(values, pos, -1ull);

                size_cnt++;

                pos += len;
            }

            MEMORIA_V1_ASSERT((int32_t)pos, ==, data_size);
            MEMORIA_V1_ASSERT(size_index[level_start + idx], ==, size_cnt);

            for (int32_t level = levels - 1; level > 0; level--)
            {
                int32_t previous_level_start = layout.level_starts[level - 1];
                int32_t previous_level_size  = layout.level_sizes[level - 1];

                int32_t current_level_start  = layout.level_starts[level];

                int32_t current_level_size = layout.level_sizes[level];

                for (int i = 0; i < previous_level_size; i++)
                {
                    int32_t sizes_sum  = 0;

                    int32_t start       = (i << BranchingFactorLog2) + current_level_start;
                    int32_t window_end  = ((i + 1) << BranchingFactorLog2);

                    int32_t end = (window_end <= current_level_size ? window_end : current_level_size) + current_level_start;

                    for (int32_t c = start; c < end; c++) {
                        sizes_sum += size_index[c];
                    }

                    MEMORIA_V1_ASSERT(size_index[previous_level_start + i], ==, sizes_sum);
                }
            }
        }
        else {
            MEMORIA_V1_ASSERT(this->element_size(block * SegmentsPerBlock + SIZE_INDEX + BlocksStart), ==, 0);

            if (data_size > 0)
            {
                MEMORIA_V1_ASSERT(offsets_size, ==, sizeof(OffsetsType));
                MEMORIA_V1_ASSERT(this->offset(block, 0), ==, 0);
            }
            else {
//              MEMORIA_V1_ASSERT(offsets_size, ==, 0);
            }

            MEMORIA_V1_ASSERT(this->data_size(block), <=, kValuesPerBranch);
        }
    }
};

}
