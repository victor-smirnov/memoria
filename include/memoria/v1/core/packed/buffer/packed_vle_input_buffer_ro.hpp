
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

    static constexpr uint32_t VERSION = 1;
    static constexpr int32_t TreeBlocks = 1;
    static constexpr int32_t Blocks = Types::Blocks;

    static constexpr int32_t SafetyMargin = (128 / Codec::ElementSize) * Blocks;

    using FieldsList = MergeLists<
                typename Base::FieldsList,
                ConstValue<uint32_t, VERSION>,
                ConstValue<uint32_t, Blocks>
    >;

    using Value  = typename Types::Value;

    using Values = core::StaticVector<Value, Blocks>;

    using SizesT = core::StaticVector<int32_t, Blocks>;

    class AppendState {
        int32_t pos_;
        int32_t size_ = 0;

        Metadata* meta_;

        using ValueDataP = ValueData*;

        ValueDataP values_;

    public:
        AppendState() {}
        AppendState(Metadata* meta): meta_(meta) {}

        int32_t& pos() {return pos_;}
        const int32_t& pos() const {return pos_;}

        int32_t& size() {return size_;}
        const int32_t& size() const {return size_;}

        ValueDataP& values() {return values_;}
        const ValueDataP& values() const {return values_;}

        Metadata* meta() {return meta_;}
    };

    OpStatus init(const SizesT& sizes) {
        return init_bs(block_size(sizes), sizes);
    }

    OpStatus init_bs(int32_t block_size, const SizesT& sizes)
    {
        if(isFail(Base::init(block_size, TreeBlocks * SegmentsPerBlock + BlocksStart))) {
            return OpStatus::FAIL;
        }

        Metadata* meta = this->template allocate<Metadata>(METADATA);
        if(isFail(meta)) {
            return OpStatus::FAIL;
        }

        meta->size() = 0;

        int32_t capacity        = sizes.sum() + SafetyMargin * Blocks;
        int32_t offsets_size    = offsets_segment_size(capacity);
        int32_t index_size      = this->index_size(capacity);
        int32_t values_segment_length = this->value_segment_size(capacity);

        meta->max_data_size(0) = capacity;

        int32_t block = 0;
        if(isFail(this->resizeBlock(block * SegmentsPerBlock + SIZE_INDEX + BlocksStart, index_size * sizeof(int32_t)))) {
            return OpStatus::FAIL;
        }

        if(isFail(this->resizeBlock(block * SegmentsPerBlock + OFFSETS + BlocksStart, offsets_size))) {
            return OpStatus::FAIL;
        }

        if(isFail(this->resizeBlock(block * SegmentsPerBlock + VALUES + BlocksStart, values_segment_length))) {
            return OpStatus::FAIL;
        }

        return OpStatus::OK;
    }


    static int32_t block_size(int32_t capacity)
    {
        return Base::block_size_equi(TreeBlocks, (capacity + SafetyMargin) * Blocks);
    }


    static int32_t block_size(const SizesT& capacities)
    {
        int32_t capacity            = capacities.sum() + SafetyMargin * Blocks;
        int32_t metadata_length     = Base::roundUpBytesToAlignmentBlocks(sizeof(Metadata));
        int32_t data_sizes_length   = Base::roundUpBytesToAlignmentBlocks(TreeBlocks * sizeof(int32_t));

        int32_t index_size      = MyType::index_size(capacity);
        int32_t sizes_length    = Base::roundUpBytesToAlignmentBlocks(index_size * sizeof(int32_t));

        int32_t values_length   = Base::roundUpBitsToAlignmentBlocks(capacity * BITS_PER_DATA_VALUE);

        int32_t offsets_length  = offsets_segment_size(capacity);

        int32_t segments_length = values_length + offsets_length + sizes_length;

        return PackedAllocator::block_size (
                metadata_length +
                data_sizes_length +
                segments_length,
                TreeBlocks * SegmentsPerBlock + BlocksStart
        );
    }


    SizesT data_capacity() const
    {
        return SizesT(this->metadata()->max_data_size()[0]);
    }

    OpStatus copyTo(MyType* other) const
    {
        auto meta = this->metadata();
        auto other_meta = other->metadata();

        other_meta->size()      = meta->size();
        other_meta->data_size() = meta->data_size();

        Codec codec;
        codec.copy(this->values(0), 0, other->values(0), 0, meta->data_size()[0]);

        return OpStatus::OK;
    }


    bool has_capacity_for(const SizesT& sizes) const
    {
        auto capacity = this->metadata()->meta->max_data_size(0);
        auto sum = sizes.sum();

        return sum <= capacity;
    }


    template <typename Buffer>
    bool has_capacity_for(const Buffer& buffer, int32_t start, int32_t length) const
    {
        auto meta = this->metadata();

        Codec codec;

        int32_t data_size = 0;

        for (int32_t c = start; c < start + length; c++)
        {
            Values entry = buffer[c];

            for (int32_t block = 0; block < Blocks; block++)
            {
                data_size += codec.length(entry[block]);
            }
        }

        return data_size <= this->metadata()->meta->max_data_size(0);
    }


    int32_t locate(int32_t block, int32_t idx) const
    {
        auto meta = this->metadata();

        MEMORIA_V1_ASSERT(idx, >=, 0);
        MEMORIA_V1_ASSERT(idx, <=, meta->size());

        auto values = this->values(0);
        auto data_size = meta->data_size(0);

        TreeLayout layout = Base::compute_tree_layout(meta->max_data_size(0));

        auto pos = locate(layout, values, 0, idx * Blocks, data_size).idx;

        if (idx < meta->size())
        {
            Codec codec;

            for (int32_t b = 0; b < block; b++)
            {
                pos += codec.length(values, pos, data_size);
            }
        }

        return pos;
    }




    static int32_t empty_size()
    {
        return block_size(0);
    }

    OpStatus reindex()
    {
        auto metadata = this->metadata();
        for (int32_t block = 0; block < TreeBlocks; block++)
        {
            auto data_size = metadata->data_size(block);
            TreeLayout layout = this->compute_tree_layout(metadata->max_data_size(block));
            if(isFail(Base::reindex_block(block, layout, data_size))) {
                return OpStatus::FAIL;
            }
        }

        return OpStatus::OK;
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

    OpStatus reset() {
        auto meta = this->metadata();
        meta->data_size().clear();
        meta->size() = 0;

        return OpStatus::OK;
    }

    Value value(int32_t block, int32_t idx) const
    {
        auto start_pos = locate(block, idx);
        auto values = this->values(0);

        Codec codec;
        Value value;

        codec.decode(values, value, start_pos);

        return value;
    }

    Values get_values(int32_t idx) const
    {
        auto start_pos = locate(0, idx);
        auto values = this->values(0);

        Codec codec;
        Values data;

        for (int32_t b = 0; b < Blocks; b++)
        {
            start_pos += codec.decode(values, data[b], start_pos);
        }

        return data;
    }

    Value get_values(int32_t idx, int32_t index) const
    {
        return this->value(index, idx);
    }

    Value getValue(int32_t index, int32_t idx) const
    {
        return this->value(index, idx);
    }

    SizesT positions(int32_t idx) const
    {
        auto start_pos = locate(0, idx);
        auto values    = this->values(0);

        Codec codec;
        SizesT pos;
        for (int32_t block = 0; block < Blocks; block++)
        {
            pos[block] = start_pos;

            start_pos += codec.length(values, start_pos, -1);
        }

        return pos;
    }

    template <typename Adaptor>
    static SizesT calculate_size(int32_t size, Adaptor&& fn)
    {
        Codec codec;
        SizesT sizes;

        for (int32_t c = 0; c < size; c++) {
            for (int32_t b = 0; b < Blocks; b++)
            {
                sizes[b] += codec.length(fn(b, c));
            }
        }

        return sizes;
    }



    AppendState append_state()
    {
        auto meta = this->metadata();
        AppendState state(meta);

        state.pos()  = meta->data_size()[0];
        state.size() = meta->size();

        state.values() = this->values(0);

        return state;
    }


    template <typename IOBuffer>
    bool append_entry_from_iobuffer(AppendState& state, IOBuffer& buffer)
    {
        Codec codec;

        auto meta = state.meta();

        size_t capacity = meta->capacity(0);

        for (int32_t block = 0; block < Blocks; block++)
        {
            auto ptr = buffer.array();
            auto pos = buffer.pos();

            size_t len = codec.length(ptr, pos, -1);

            if (len <= capacity)
            {
                int32_t& data_pos = meta->data_size(0);

                codec.copy(ptr, pos, state.values(), data_pos, len);

                data_pos += len;
                buffer.skip(len);
                capacity -= len;
            }
            else {
                return false;
            }
        }

        state.size()++;
        state.pos() = meta->data_size()[0];
        this->size()++;

        return true;
    }



    void restore(const AppendState& state)
    {
        auto meta = this->metadata();

        meta->size() = state.size();
        meta->data_size()[0] = state.pos();
    }



    template <typename Adaptor>
    int32_t append(int32_t size, Adaptor&& adaptor)
    {
        Codec codec;

        auto metadata = this->metadata();

        auto values = this->values(0);

        size_t pos = metadata->data_size(0);

        size_t limit = metadata->max_data_size(0) - SafetyMargin * Blocks;

        int32_t c;
        for (c = 0; c < size && pos < limit; c++)
        {
            for (int32_t block = 0; block < Blocks; block++)
            {
                const auto& value = adaptor(block, c);
                pos += codec.encode(values, value, pos);
            }
        }

        metadata->data_size(0) = pos;

        metadata->size() += c;

        return c;
    }



    void check_indexless(int32_t block, int32_t data_size) const
    {
        int32_t offsets_size = this->element_size(block * SegmentsPerBlock + Base::OFFSETS + BlocksStart);
        MEMORIA_V1_ASSERT(this->element_size(block * SegmentsPerBlock + SIZE_INDEX + BlocksStart), ==, 0);

        if (data_size > 0)
        {
            MEMORIA_V1_ASSERT(offsets_size, ==, sizeof(OffsetsType));
            MEMORIA_V1_ASSERT(this->offset(block, 0), ==, 0);
        }
        else {
            MEMORIA_V1_ASSERT(offsets_size, ==, 0);
        }

        MEMORIA_V1_ASSERT(data_size, <=, (int32_t)ValuesPerBranch);
    }


    void check() const
    {
        auto metadata = this->metadata();

        for (int32_t block = 0; block < TreeBlocks; block++)
        {
            int32_t data_size     = metadata->data_size(block);
            int32_t max_data_size = metadata->max_data_size(block);

            MEMORIA_V1_ASSERT(data_size, <=, max_data_size);

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
    SizesT scan(int32_t start, int32_t end, ConsumerFn&& fn) const
    {
        auto metadata = this->metadata();

        MEMORIA_V1_ASSERT(start, >=, 0);
        MEMORIA_V1_ASSERT(end, >=, start);
        MEMORIA_V1_ASSERT(end, <=, metadata->size());

        Codec codec;

        auto values = this->values(0);
        size_t position = this->locate(0, start);

        for (int32_t c = start; c < end; c++)
        {
            Values values_data;

            for (int32_t block = 0; block < Blocks; block++)
            {
                position += codec.decode(values, values_data[block], position);
            }

            fn(values_data);
        }

        return SizesT(position);
    }

    void dump(std::ostream& out = std::cout) const
    {
        auto metadata = this->metadata();

        int32_t size = metadata->size();

        auto data_size      = metadata->data_size(0);
        auto max_data_size  = metadata->max_data_size(0);

        out << "size_         = " << size << std::endl;
        out << "block_size_   = " << this->block_size() << std::endl;
        out << "data_size_    = " << data_size << std::endl;
        out << "max_data_size_= " << max_data_size << std::endl;

        auto index_size = this->index_size(max_data_size);

        out << "index_size_   = " << index_size << std::endl;

        TreeLayout layout = this->compute_tree_layout(max_data_size);

        if (layout.levels_max >= 0)
        {
            out << "TreeLayout: " << std::endl;

            out << "Level sizes: ";
            for (int32_t c = 0; c <= layout.levels_max; c++) {
                out << layout.level_sizes[c] << " ";
            }
            out << std::endl;

            out << "Level starts: ";
            for (int32_t c = 0; c <= layout.levels_max; c++) {
                out << layout.level_starts[c] << " ";
            }
            out << std::endl;

            auto size_indexes = this->size_index(0);

            out << "Index:" << std::endl;
            for (int32_t c = 0; c < index_size; c++)
            {
                out << c << ": " << size_indexes[c] << std::endl;
            }
        }

        out << std::endl;

        out << "Offsets: ";
        for (int32_t c = 0; c <= this->divUpV(data_size); c++) {
            out << this->offset(0, c) << " ";
        }
        out << std::endl;

        out << "Values: " << std::endl;

        auto values = this->values(0);

        Codec codec;

        size_t pos = 0;

        for (int32_t c = 0; c < size; c++)
        {
            out << "c: " << c << " ";
            for (int32_t block = 0; block < Blocks; block++)
            {
                Value value;
                auto len = codec.decode(values, value, pos);
                out << value << " ";
                pos += len;
            }
            out << std::endl;
        }
    }

    void generateDataEvents(IBlockDataEventHandler* handler) const
    {
        Base::generateDataEvents(handler);

        handler->startStruct();
        handler->startGroup("VLE_COLUMS_ORDER_INPUT_BUFFER");

        auto meta = this->metadata();

        handler->value("SIZE",      &meta->size());
        handler->value("DATA_SIZE", &meta->data_size(0), TreeBlocks);

        handler->startGroup("INDEXES", TreeBlocks);

        for (int32_t block = 0; block < TreeBlocks; block++)
        {
            int32_t index_size = this->index_size(Base::data_size(block));

            handler->startGroup("BLOCK_INDEX", block);

            auto size_indexes = this->size_index(block);

            for (int32_t c = 0; c < index_size; c++)
            {
                int64_t indexes[] = {
                        size_indexes[c]
                };

                handler->value("INDEX", indexes, 1);
            }

            handler->endGroup();
        }

        handler->endGroup();


        handler->startGroup("DATA", meta->size());

        int32_t size = this->size();

        Codec codec;

        size_t positions[Blocks];
        for (int32_t block = 0; block < Blocks; block++) {
            positions[block] = this->locate(0, block * size);
        }

        auto values = this->values();

        for (int32_t idx = 0; idx < size; idx++)
        {
            Value values_data[Blocks];
            for (int32_t block = 0; block < Blocks; block++)
            {
                auto len = codec.decode(values, values_data[block], positions[block]);
                positions[block] += len;
            }

            handler->value("ARRAY_ITEM", BlockValueProviderFactory::provider(Blocks, [&](int32_t idx) {
                return values_data[idx];
            }));
        }

        handler->endGroup();

        handler->endGroup();

        handler->endStruct();
    }
};




}}
