
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/core/packed/tree/vle/packed_vle_quick_tree_base.hpp>
#include <memoria/v1/core/packed/tree/vle/packed_vle_tools.hpp>
#include <memoria/v1/core/packed/buffer/packed_vle_input_buffer_co.hpp>

#include <memoria/v1/core/packed/array/packed_vle_array_base.hpp>

namespace memoria {
namespace v1 {




template <typename Types>
class PkdVLERowOrderInputBuffer: public PkdVLEArrayBase<
    1,
    typename Types::Value,
    Types::template Codec,
    Types::BranchingFactor,
    Types::ValuesPerBranch,
    PkdVLEInputBufferMetadata<1>
> {

    using Base = PkdVLEArrayBase<
            1,
            typename Types::Value,
            Types::template Codec,
            Types::BranchingFactor,
            Types::ValuesPerBranch,
            PkdVLEInputBufferMetadata<1>
    >;

    using MyType = PkdVLERowOrderInputBuffer<Types>;

public:
    using Base::BlocksStart;
    using Base::SegmentsPerBlock;
    using Base::compute_tree_layout;
    using Base::reindex_block;
    using Base::offsets_segment_size;
    using Base::locate;
    using Base::block_size;

    using Base::METADATA;
    using Base::DATA_SIZES;

    using Base::VALUES;
    using Base::OFFSETS;
    using Base::SIZE_INDEX;
    using Base::BITS_PER_DATA_VALUE;
    using Base::ValuesPerBranch;


    using typename Base::Metadata;
    using typename Base::TreeLayout;
    using typename Base::OffsetsType;
    using typename Base::ValueData;



    using typename Base::Codec;

    static constexpr UInt VERSION = 1;
    static constexpr Int TreeBlocks = 1;
    static constexpr Int Blocks = Types::Blocks;

    static constexpr Int SafetyMargin = (128 / Codec::ElementSize) * Blocks;

    using FieldsList = MergeLists<
                typename Base::FieldsList,
                ConstValue<UInt, VERSION>,
                ConstValue<UInt, Blocks>
    >;

    using Value      = typename Types::Value;

    using Values = core::StaticVector<Value, Blocks>;

    using SizesT = core::StaticVector<Int, Blocks>;

    void init(const SizesT& sizes)
    {
        Base::init(block_size(sizes), TreeBlocks * SegmentsPerBlock + BlocksStart);

        Metadata* meta = this->template allocate<Metadata>(METADATA);

        meta->size() = 0;

        Int capacity        = sizes.sum() + SafetyMargin * Blocks;
        Int offsets_size    = offsets_segment_size(capacity);
        Int index_size      = this->index_size(capacity);
        Int values_segment_length = this->value_segment_size(capacity);

        meta->max_data_size(0) = capacity;

        Int block = 0;
        this->resizeBlock(block * SegmentsPerBlock + SIZE_INDEX + BlocksStart, index_size * sizeof(Int));
        this->resizeBlock(block * SegmentsPerBlock + OFFSETS + BlocksStart, offsets_size);
        this->resizeBlock(block * SegmentsPerBlock + VALUES + BlocksStart, values_segment_length);
    }


    static Int block_size(Int capacity)
    {
        return Base::block_size_equi(TreeBlocks, (capacity + SafetyMargin) * Blocks);
    }


    static Int block_size(const SizesT& capacities)
    {
        Int capacity            = capacities.sum() + SafetyMargin * Blocks;
        Int metadata_length     = Base::roundUpBytesToAlignmentBlocks(sizeof(Metadata));
        Int data_sizes_length   = Base::roundUpBytesToAlignmentBlocks(TreeBlocks * sizeof(Int));

        Int index_size      = MyType::index_size(capacity);
        Int sizes_length    = Base::roundUpBytesToAlignmentBlocks(index_size * sizeof(Int));

        Int values_length   = Base::roundUpBitsToAlignmentBlocks(capacity * BITS_PER_DATA_VALUE);

        Int offsets_length  = offsets_segment_size(capacity);

        Int segments_length = values_length + offsets_length + sizes_length;

        return PackedAllocator::block_size (
                metadata_length +
                data_sizes_length +
                segments_length,
                TreeBlocks * SegmentsPerBlock + BlocksStart
        );
    }

    bool has_capacity_for(const SizesT& sizes) const
    {
        auto capacity = this->metadata()->meta->max_data_size(0);
        auto sum = sizes.sum();

        return sum <= capacity;
    }


    Int locate(Int block, Int idx) const
    {
        auto meta = this->metadata();

        MEMORIA_ASSERT(idx, >=, 0);
        MEMORIA_ASSERT(idx, <=, meta->size());

        auto values = this->values(0);
        auto data_size = meta->data_size(0);

        TreeLayout layout = Base::compute_tree_layout(meta->max_data_size(0));

        auto pos = locate(layout, values, 0, idx * Blocks, data_size).idx;

        if (idx < meta->size())
        {
            Codec codec;

            for (Int b = 0; b < block; b++)
            {
                pos += codec.length(values, pos, data_size);
            }
        }

        return pos;
    }




    static Int empty_size()
    {
        return block_size(0);
    }

    void reindex()
    {
        auto metadata = this->metadata();
        for (Int block = 0; block < TreeBlocks; block++)
        {
            auto data_size = metadata->data_size(block);
            TreeLayout layout = this->compute_tree_layout(metadata->max_data_size(block));
            Base::reindex_block(block, layout, data_size);
        }
    }



    bool check_capacity(Int size) const
    {
        MEMORIA_ASSERT_TRUE(size >= 0);

        auto alloc = this->allocator();

        Int total_size          = this->size() + size;
        Int total_block_size    = MyType::block_size(total_size);
        Int my_block_size       = alloc->element_size(this);
        Int delta               = total_block_size - my_block_size;

        return alloc->free_space() >= delta;
    }

    void reset() {
        auto meta = this->metadata();
        meta->data_size().clear();
        meta->size() = 0;
    }

    Value value(Int block, Int idx) const
    {
        auto start_pos = locate(block, idx);
        auto values = this->values(0);

        Codec codec;
        Value value;

        codec.decode(values, value, start_pos);

        return value;
    }

    Values get_values(Int idx) const
    {
        auto start_pos = locate(0, idx);
        auto values = this->values(0);

        Codec codec;
        Values data;

        for (Int b = 0; b < Blocks; b++)
        {
            start_pos += codec.decode(values, data[b], start_pos);
        }

        return data;
    }

    Value get_values(Int idx, Int index) const
    {
        return this->value(index, idx);
    }

    Value getValue(Int index, Int idx) const
    {
        return this->value(index, idx);
    }

    SizesT positions(Int idx) const
    {
        auto start_pos = locate(0, idx);
        auto values    = this->values(0);

        Codec codec;
        SizesT pos;
        for (Int block = 0; block < Blocks; block++)
        {
            pos[block] = start_pos;

            start_pos += codec.length(values, start_pos, -1);
        }

        return pos;
    }

    template <typename Adaptor>
    static SizesT calculate_size(Int size, Adaptor&& fn)
    {
        Codec codec;
        SizesT sizes;

        for (Int c = 0; c < size; c++) {
            for (Int b = 0; b < Blocks; b++)
            {
                sizes[b] += codec.length(fn(b, c));
            }
        }

        return sizes;
    }




    template <typename Adaptor>
    Int append(Int size, Adaptor&& adaptor)
    {
        Codec codec;

        auto metadata = this->metadata();

        auto values = this->values(0);

        size_t pos = metadata->data_size(0);

        size_t limit = metadata->max_data_size(0) - SafetyMargin * Blocks;

        Int c;
        for (c = 0; c < size && pos < limit; c++)
        {
            for (Int block = 0; block < Blocks; block++)
            {
                auto value = adaptor(block, c);
                pos += codec.encode(values, value, pos);
            }
        }

        metadata->data_size(0) = pos;

        metadata->size() += c;

        return c;
    }



    void check_indexless(Int block, Int data_size) const
    {
        Int offsets_size = this->element_size(block * SegmentsPerBlock + Base::OFFSETS + BlocksStart);
        MEMORIA_ASSERT(this->element_size(block * SegmentsPerBlock + SIZE_INDEX + BlocksStart), ==, 0);

        if (data_size > 0)
        {
            MEMORIA_ASSERT(offsets_size, ==, sizeof(OffsetsType));
            MEMORIA_ASSERT(this->offset(block, 0), ==, 0);
        }
        else {
            MEMORIA_ASSERT(offsets_size, ==, 0);
        }

        MEMORIA_ASSERT(data_size, <=, ValuesPerBranch);
    }


    void check() const
    {
        auto metadata = this->metadata();

        for (Int block = 0; block < TreeBlocks; block++)
        {
            Int data_size     = metadata->data_size(block);
            Int max_data_size = metadata->max_data_size(block);

            MEMORIA_ASSERT(data_size, <=, max_data_size);

            TreeLayout layout = this->compute_tree_layout(max_data_size);

            if (layout.levels_max >= 0)
            {
                Base::check_block(block, layout, data_size);
            }
            else {
                check_indexless(block, max_data_size);
            }
        }
    }

    template <typename ConsumerFn>
    SizesT scan(Int start, Int end, ConsumerFn&& fn) const
    {
        auto metadata = this->metadata();

        MEMORIA_ASSERT(start, >=, 0);
        MEMORIA_ASSERT(end, >=, start);
        MEMORIA_ASSERT(end, <=, metadata->size());

        Codec codec;

        auto values = this->values(0);
        size_t position = this->locate(0, start);

        for (Int c = start; c < end; c++)
        {
            Values values_data;

            for (Int block = 0; block < Blocks; block++)
            {
                position += codec.decode(values, values_data[block], position);
            }

            fn(values_data);
        }

        return SizesT(position);
    }

    void dump(std::ostream& out = cout) const
    {
        auto metadata = this->metadata();

        Int size = metadata->size();

        auto data_size      = metadata->data_size(0);
        auto max_data_size  = metadata->max_data_size(0);

        out<<"size_         = "<<size<<std::endl;
        out<<"block_size_   = "<<this->block_size()<<std::endl;
        out<<"data_size_    = "<<data_size<<std::endl;
        out<<"max_data_size_= "<<max_data_size<<std::endl;

        auto index_size = this->index_size(max_data_size);

        out<<"index_size_   = "<<index_size<<std::endl;

        TreeLayout layout = this->compute_tree_layout(max_data_size);

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

            auto size_indexes = this->size_index(0);

            out<<"Index:"<<endl;
            for (Int c = 0; c < index_size; c++)
            {
                out<<c<<": "<<size_indexes[c]<<std::endl;
            }
        }

        out<<endl;

        out<<"Offsets: ";
        for (Int c = 0; c <= this->divUpV(data_size); c++) {
            out<<this->offset(0, c)<<" ";
        }
        out<<endl;

        out<<"Values: "<<endl;

        auto values = this->values(0);

        Codec codec;

        size_t pos = 0;

        for (Int c = 0; c < size; c++)
        {
            out<<"c: "<<c<<" ";
            for (Int block = 0; block < Blocks; block++)
            {
                Value value;
                auto len = codec.decode(values, value, pos);
                out<<value<<" ";
                pos += len;
            }
            out<<endl;
        }
    }

    void generateDataEvents(IPageDataEventHandler* handler) const
    {
        Base::generateDataEvents(handler);

        handler->startStruct();
        handler->startGroup("VLE_COLUMS_ORDER_INPUT_BUFFER");

        auto meta = this->metadata();

        handler->value("SIZE",      &meta->size());
        handler->value("DATA_SIZE", &meta->data_size(0), TreeBlocks);

        handler->startGroup("INDEXES", TreeBlocks);

        for (Int block = 0; block < TreeBlocks; block++)
        {
            Int index_size = this->index_size(Base::data_size(block));

            handler->startGroup("BLOCK_INDEX", block);

            auto size_indexes = this->size_index(block);

            for (Int c = 0; c < index_size; c++)
            {
                BigInt indexes[] = {
                        size_indexes[c]
                };

                handler->value("INDEX", indexes, 1);
            }

            handler->endGroup();
        }

        handler->endGroup();


        handler->startGroup("DATA", meta->size());

        Int size = this->size();

        Codec codec;

        size_t positions[Blocks];
        for (Int block = 0; block < Blocks; block++) {
            positions[block] = this->locate(0, block * size);
        }

        auto values = this->values();

        for (Int idx = 0; idx < size; idx++)
        {
            Value values_data[Blocks];
            for (Int block = 0; block < Blocks; block++)
            {
                auto len = codec.decode(values, values_data[block], positions[block]);
                positions[block] += len;
            }

            handler->value("TREE_ITEM", values_data, Blocks);
        }

        handler->endGroup();

        handler->endGroup();

        handler->endStruct();
    }
};




}}