
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/core/packed/array/packed_vle_array_base_base.hpp>


namespace memoria {
namespace v1 {

template <Int Blocks>
class PkdVDArrayMetadata {
    Int size_;

    Int data_size_[Blocks];

public:
    PkdVDArrayMetadata() = default;

    Int& size() {return size_;}
    const Int& size() const {return size_;}

    Int& data_size(Int block) {return data_size_[block];}
    const Int& data_size(Int block) const {return data_size_[block];}

    template <Int, typename, template <typename> class Codec, Int, Int, typename> friend class PkdVLEArrayBase;
};



template <
    Int Blocks,
    typename ValueT,
    template <typename> class CodecT,
    Int kBranchingFactor,
    Int kValuesPerBranch,
    typename MetadataT = PkdVDArrayMetadata<Blocks>
>
class PkdVLEArrayBase: public PkdVLEArrayBaseBase<kBranchingFactor, kValuesPerBranch, 3, MetadataT> {

    using Base      = PkdVLEArrayBaseBase<kBranchingFactor, kValuesPerBranch, 3, MetadataT>;
    using MyType    = PkdVLEArrayBase<Blocks, ValueT, CodecT, kBranchingFactor, kValuesPerBranch, MetadataT>;

public:
    static constexpr UInt VERSION = 1;

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

    static constexpr Int SegmentsPerBlock = 3;


    static constexpr Int BITS_PER_OFFSET        = Codec::BitsPerOffset;
    static constexpr Int BITS_PER_DATA_VALUE    = Codec::ElementSize;

    using Base::BlocksStart;

    using Base::METADATA;
    using Base::DATA_SIZES;

    using Base::VALUES;
    using Base::OFFSETS;
    using Base::SIZE_INDEX;

    using typename Base::LocateResult;


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

    PkdVLEArrayBase() = default;

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
        return values > 0 ? Base::divUpV(values) : 1;
    }

    static constexpr Int offsets_segment_size(Int values)
    {
        return PackedAllocator::roundUpBitsToAlignmentBlocks(number_of_offsets(values) * BITS_PER_OFFSET);
    }

    static constexpr Int index2_segment_size(Int index_size) {
        return PackedAllocator::roundUpBytesToAlignmentBlocks(index_size * sizeof(int));
    }



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
            this->template allocateArrayBySize<Int>(block * SegmentsPerBlock + SIZE_INDEX + BlocksStart, index_size);
            this->template allocateArrayBySize<Byte>(block * SegmentsPerBlock + OFFSETS + BlocksStart, offsets_size);
            this->template allocateArrayBySize<Byte>(block * SegmentsPerBlock + VALUES + BlocksStart, values_segment_length);
        }
    }




    //FIXME: invalid block size calculation by capacity
    static Int block_size_equi(Int blocks, Int total_capacity)
    {
        Int metadata_length = Base::roundUpBytesToAlignmentBlocks(sizeof(Metadata));

        Int index_size      = MyType::index_size(total_capacity);
        Int sizes_length    = Base::roundUpBytesToAlignmentBlocks(index_size * sizeof(Int));

        Int values_length   = Base::roundUpBitsToAlignmentBlocks(total_capacity * BITS_PER_DATA_VALUE);

        Int offsets_length  = offsets_segment_size(total_capacity);

        Int  blocks_length  = sizes_length + values_length + offsets_length;

        return Base::block_size(
                metadata_length +
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
        return this->element_size(block * SegmentsPerBlock + SIZE_INDEX + BlocksStart) > 0;
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
        auto data_size = this->data_size(block);

        TreeLayout layout = Base::compute_tree_layout(data_size);

        return locate(layout, values, block, idx, data_size).idx;
    }

    LocateResult locate(TreeLayout& layout, const ValueData* values, Int block, Int idx, size_t data_size) const
    {
        if (data_size > 0) {

            LocateResult locate_result;

            if (layout.levels_max >= 0)
            {
                layout.valaue_block_size_prefix = this->size_index(block);
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


    void reindex(Int blocks)
    {
        for (Int block = 0; block < blocks; block++)
        {
            Int data_size = this->data_size(block);
            TreeLayout layout = this->compute_tree_layout(data_size);

            this->reindex_block(block, layout, data_size);
        }
    }


    void check(Int blocks) const
    {
        for (Int block = 0; block < blocks; block++)
        {
            Int data_size = this->data_size(block);
            TreeLayout layout = this->compute_tree_layout(data_size);

            this->check_block(block, layout, data_size);
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
        auto data_size = this->data_size(block);
        TreeLayout layout = this->compute_tree_layout(data_size);
        reindex_block(block, layout, data_size);
    }

    void reindex_block(Int block, TreeLayout& layout, Int data_size)
    {
        if (layout.levels_max >= 0)
        {
            auto values     = this->values(block);
            auto size_index = this->size_index(block);
            auto offsets    = this->offsets(block);

            this->clear(block * SegmentsPerBlock + Base::SIZE_INDEX + BlocksStart);
            this->clear(block * SegmentsPerBlock + Base::OFFSETS + BlocksStart);

            layout.valaue_block_size_prefix = size_index;

            Int levels = layout.levels_max + 1;

            Int level_start = layout.level_starts[levels - 1];

            Codec codec;

            size_t pos = 0;
            Int size_cnt = 0;
            size_t threshold = ValuesPerBranch;

            set_offset(offsets, 0, 0);

            Int idx = 0;
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

            for (Int level = levels - 1; level > 0; level--)
            {
                Int previous_level_start = layout.level_starts[level - 1];
                Int previous_level_size  = layout.level_sizes[level - 1];

                Int current_level_start  = layout.level_starts[level];

                Int current_level_size = layout.level_sizes[level];

                for (int i = 0; i < previous_level_size; i++)
                {
                    Int sizes_sum  = 0;

                    Int start       = (i << BranchingFactorLog2) + current_level_start;
                    Int window_end  = ((i + 1) << BranchingFactorLog2);

                    Int end = (window_end <= current_level_size ? window_end : current_level_size) + current_level_start;

                    for (Int c = start; c < end; c++) {
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
    }


    void check_block(Int block, TreeLayout& layout, Int data_size) const
    {
        Int offsets_size = this->element_size(block * SegmentsPerBlock + Base::OFFSETS + BlocksStart);

        if (layout.levels_max >= 0)
        {
            MEMORIA_ASSERT(this->element_size(block * SegmentsPerBlock + SIZE_INDEX + BlocksStart), >, 0);

            auto values     = this->values(block);
            auto size_index = this->size_index(block);
            auto offsets    = this->offsets(block);

            layout.valaue_block_size_prefix = size_index;

            Int levels = layout.levels_max + 1;

            Int level_start = layout.level_starts[levels - 1];

            Codec codec;

            size_t pos = 0;
            Int size_cnt = 0;
            size_t threshold = ValuesPerBranch;

            MEMORIA_ASSERT(offset(offsets, 0), ==, 0);

            Int idx = 0;
            while(pos < data_size)
            {
                if (pos >= threshold)
                {
                    MEMORIA_ASSERT(offset(offsets, idx + 1), ==, pos - threshold);
                    MEMORIA_ASSERT(size_index[level_start + idx], ==, size_cnt);

                    threshold += ValuesPerBranch;

                    idx++;
                    size_cnt  = 0;
                }

                auto len = codec.length(values, pos, -1ull);

                size_cnt++;

                pos += len;
            }

            MEMORIA_ASSERT((Int)pos, ==, data_size);
            MEMORIA_ASSERT(size_index[level_start + idx], ==, size_cnt);

            for (Int level = levels - 1; level > 0; level--)
            {
                Int previous_level_start = layout.level_starts[level - 1];
                Int previous_level_size  = layout.level_sizes[level - 1];

                Int current_level_start  = layout.level_starts[level];

                Int current_level_size = layout.level_sizes[level];

                for (int i = 0; i < previous_level_size; i++)
                {
                    Int sizes_sum  = 0;

                    Int start       = (i << BranchingFactorLog2) + current_level_start;
                    Int window_end  = ((i + 1) << BranchingFactorLog2);

                    Int end = (window_end <= current_level_size ? window_end : current_level_size) + current_level_start;

                    for (Int c = start; c < end; c++) {
                        sizes_sum += size_index[c];
                    }

                    MEMORIA_ASSERT(size_index[previous_level_start + i], ==, sizes_sum);
                }
            }
        }
        else {
            MEMORIA_ASSERT(this->element_size(block * SegmentsPerBlock + SIZE_INDEX + BlocksStart), ==, 0);

            if (data_size > 0)
            {
                MEMORIA_ASSERT(offsets_size, ==, sizeof(OffsetsType));
                MEMORIA_ASSERT(this->offset(block, 0), ==, 0);
            }
            else {
//              MEMORIA_ASSERT(offsets_size, ==, 0);
            }

            MEMORIA_ASSERT(this->data_size(block), <=, kValuesPerBranch);
        }
    }
};

}}