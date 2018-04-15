
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

#include <memoria/v1/core/packed/buffer/packed_vle_input_buffer_co.hpp>
#include <memoria/v1/core/packed/tree/vle/packed_vle_quick_tree_base.hpp>
#include <memoria/v1/core/packed/tree/vle/packed_vle_tools.hpp>

namespace memoria {
namespace v1 {


template <typename Types> class PkdVQTree;

template <
    typename IndexValueT,
    int32_t kBlocks = 1,
    template <typename> class CodecT = ValueCodec,
    typename ValueT = IndexValueT,
    int32_t kBranchingFactor = PkdVLETreeShapeProvider<CodecT<ValueT>>::BranchingFactor,
    int32_t kValuesPerBranch = PkdVLETreeShapeProvider<CodecT<ValueT>>::ValuesPerBranch
>
using PkdVQTreeT = PkdVQTree<PkdVLETreeTypes<IndexValueT, kBlocks, CodecT, ValueT, kBranchingFactor, kValuesPerBranch>>;


template <typename Types>
class PkdVQTree: public PkdVQTreeBase<typename Types::IndexValue, typename Types::Value, Types::template Codec, Types::BranchingFactor, Types::ValuesPerBranch> {

    using Base      = PkdVQTreeBase<typename Types::IndexValue, typename Types::Value, Types::template Codec, Types::BranchingFactor, Types::ValuesPerBranch>;
    using MyType    = PkdVQTree<Types>;

public:

    using Base::BlocksStart;
    using Base::SegmentsPerBlock;
    using Base::compute_tree_layout;
    using Base::gsum;
    using Base::find;
    using Base::walk_fw;
    using Base::walk_bw;
    using Base::reindex_block;
    using Base::offsets_segment_size;

    using Base::METADATA;
    using Base::DATA_SIZES;

    using Base::VALUES;
    using Base::VALUE_INDEX;
    using Base::OFFSETS;
    using Base::SIZE_INDEX;
    using Base::BITS_PER_DATA_VALUE;


    using typename Base::Metadata;
    using typename Base::TreeLayout;
    using typename Base::OffsetsType;
    using typename Base::ValueData;
    using typename Base::FindGEWalker;
    using typename Base::FindGTWalker;


    using typename Base::Codec;

    static constexpr uint32_t VERSION = 1;
    static constexpr int32_t Blocks = Types::Blocks;

    using FieldsList = MergeLists<
                typename Base::FieldsList,
                ConstValue<uint32_t, VERSION>,
                ConstValue<uint32_t, Blocks>
    >;

    using IndexValue = typename Types::IndexValue;
    using Value      = typename Types::Value;

    using Values = core::StaticVector<IndexValue, Blocks>;

    using InputBuffer   = PkdVLEColumnOrderInputBuffer<Types>;
    using InputType     = Values;

    using SizesT = core::StaticVector<int32_t, Blocks>;

    using ReadState = SizesT;

    static int32_t estimate_block_size(int32_t tree_capacity, int32_t density_hi = 1000, int32_t density_lo = 333)
    {
        int32_t max_tree_capacity = (tree_capacity * Blocks * density_hi) / density_lo;
        return block_size(max_tree_capacity);
    }


    void init_tl(int32_t data_block_size)
    {
        Base::init_tl(data_block_size, Blocks);
    }

    void init(const SizesT& sizes)
    {
        Base::init(empty_size(), Blocks * SegmentsPerBlock + BlocksStart);

        Metadata* meta = this->template allocate<Metadata>(METADATA);
        this->template allocateArrayBySize<int32_t>(DATA_SIZES, Blocks);

        meta->size()        = 0;

        for (int32_t block = 0; block < Blocks; block++)
        {
            int32_t capacity        = sizes[block];
            int32_t offsets_size    = offsets_segment_size(capacity);
            int32_t index_size      = this->index_size(capacity);
            int32_t values_segment_length = this->value_segment_size(capacity);

            this->resizeBlock(block * SegmentsPerBlock + VALUE_INDEX + BlocksStart, index_size * sizeof(IndexValue));
            this->resizeBlock(block * SegmentsPerBlock + SIZE_INDEX + BlocksStart, index_size * sizeof(int32_t));
            this->resizeBlock(block * SegmentsPerBlock + OFFSETS + BlocksStart, offsets_size);
            this->resizeBlock(block * SegmentsPerBlock + VALUES + BlocksStart, values_segment_length);
        }
    }

    void init() {
        init_bs(empty_size());
    }


    void init_bs(int32_t block_size)
    {
        Base::init(block_size, Blocks * SegmentsPerBlock + BlocksStart);

        Metadata* meta = this->template allocate<Metadata>(METADATA);
        this->template allocateArrayBySize<int32_t>(DATA_SIZES, Blocks);

        meta->size() = 0;
        int32_t offsets_size = offsets_segment_size(0);

        for (int32_t block = 0; block < Blocks; block++)
        {
            this->template allocateArrayBySize<IndexValue>(block * SegmentsPerBlock + VALUE_INDEX + BlocksStart, 0);
            this->template allocateArrayBySize<int32_t>(block * SegmentsPerBlock + SIZE_INDEX + BlocksStart, 0);
            this->template allocateArrayBySize<int8_t>(block * SegmentsPerBlock + OFFSETS + BlocksStart, offsets_size);
            this->template allocateArrayBySize<int8_t>(block * SegmentsPerBlock + VALUES + BlocksStart, 0);
        }
    }

    static int32_t block_size(int32_t capacity)
    {
        return Base::block_size_equi(Blocks, capacity);
    }


    static int32_t block_size(const SizesT& capacity)
    {
        int32_t metadata_length = Base::roundUpBytesToAlignmentBlocks(sizeof(Metadata));
        int32_t data_sizes_length = Base::roundUpBytesToAlignmentBlocks(Blocks * sizeof(int32_t));


        int32_t segments_length = 0;

        for (int32_t block = 0; block < Blocks; block++)
        {
            int32_t index_size      = MyType::index_size(capacity[block]);
            int32_t index_length    = Base::roundUpBytesToAlignmentBlocks(index_size * sizeof(IndexValue));
            int32_t sizes_length    = Base::roundUpBytesToAlignmentBlocks(index_size * sizeof(int32_t));

            int32_t values_length   = Base::roundUpBitsToAlignmentBlocks(capacity[block] * BITS_PER_DATA_VALUE);

            int32_t offsets_length  = offsets_segment_size(capacity[block]);

            segments_length += index_length + values_length + offsets_length + sizes_length;
        }

        return PackedAllocator::block_size(
                metadata_length +
                data_sizes_length +
                segments_length,
                Blocks * SegmentsPerBlock + BlocksStart
        );
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
        return block_size(this->data_size_v() + other->data_size_v());
    }


    SizesT data_size_v() const
    {
        SizesT sizes;

        for (int32_t block = 0; block < Blocks; block++)
        {
            sizes[block] = this->data_size(block);
        }

        return sizes;
    }

    int32_t data_block_size(int32_t block) const
    {
        int32_t size = this->element_size(block * SegmentsPerBlock + BlocksStart + VALUES);
        return PackedAllocatable::roundUpBytesToAlignmentBlocks(size) / sizeof(ValueData);
    }

    static int32_t elements_for(int32_t block_size)
    {
        return Base::tree_size(Blocks, block_size);
    }

    static int32_t expected_block_size(int32_t items_num)
    {
        return block_size(items_num);
    }

    Value value(int32_t block, int32_t idx) const
    {
        MEMORIA_V1_ASSERT(idx, >=, 0);
        MEMORIA_V1_ASSERT(idx, <, this->size());

        int32_t data_size     = this->data_size(block);
        auto values       = this->values(block);
        TreeLayout layout = this->compute_tree_layout(data_size);

        int32_t start_pos     = this->locate(layout, values, block, idx).idx;

        MEMORIA_V1_ASSERT(start_pos, <, data_size);

        Codec codec;
        Value value;

        codec.decode(values, value, start_pos);

        return value;
    }


    static int32_t empty_size()
    {
        return block_size(0);
    }

    void reindex() {
        Base::reindex(Blocks);
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





    // ========================================= Insert/Remove/Resize ============================================== //

protected:
    void resize(int32_t block, int32_t data_size, int32_t start, int32_t length)
    {
        int32_t new_data_size = data_size + length;

        int32_t data_segment_size    = PackedAllocator::roundUpBitsToAlignmentBlocks(new_data_size * Codec::ElementSize);
        int32_t index_size           = Base::index_size(new_data_size);
        int32_t offsets_segment_size = Base::offsets_segment_size(new_data_size);

        this->resizeBlock(block * SegmentsPerBlock + VALUES + BlocksStart, data_segment_size);
        this->resizeBlock(block * SegmentsPerBlock + OFFSETS + BlocksStart, offsets_segment_size);
        this->resizeBlock(block * SegmentsPerBlock + SIZE_INDEX + BlocksStart, index_size * sizeof(int32_t));
        this->resizeBlock(block * SegmentsPerBlock + VALUE_INDEX + BlocksStart, index_size * sizeof(IndexValue));
    }


    void insert_space(int32_t block, int32_t start, int32_t length)
    {
        int32_t data_size = this->data_size(block);
        resize(block, data_size, start, length);

        auto values = this->values(block);

        Codec codec;
        codec.move(values, start, start + length, data_size - start);

        this->data_size(block) += length;
    }

    void dump_values(int32_t block, std::ostream& out = std::cout)
    {
        out << "Dump values" << std::endl;
        Codec codec;
        size_t pos = 0;

        auto values     = this->values(block);
        auto data_size  = this->data_size(block);

        for(int32_t c = 0; pos < data_size; c++)
        {
            Value value;
            auto len = codec.decode(values, value, pos);

            out << c << ": " << pos << " " << value << std::endl;

            pos += len;
        }

        out << std::endl;
    }


    void remove_space(int32_t block, int32_t start, int32_t length)
    {
        int32_t data_size = this->data_size(block);
        auto values = this->values(block);

        Codec codec;
        int32_t end = start + length;

        if (data_size < end) {
            std::cout << "RemoeSpace: " << this->size() << std::endl;
        }

        MEMORIA_V1_ASSERT(data_size, >=, end);

        codec.move(values, end, start, data_size - end);

        resize(block, data_size, start, -(end - start));

        this->data_size(block) -= (end - start);
    }



public:
    void splitTo(MyType* other, int32_t idx)
    {
        Codec codec;
        for (int32_t block = 0; block < Blocks; block++)
        {
            int32_t start = this->locate(block, idx);
            int32_t size  = this->data_size(block) - start;

            other->insert_space(block, 0, size);
            codec.copy(this->values(block), start, other->values(block), 0, size);
        }

        int32_t size = this->size();
        other->size() += size - idx;

        other->reindex();

        remove(idx, size);
    }


    void mergeWith(MyType* other)
    {
        Codec codec;

        for (int32_t block = 0; block < Blocks; block++)
        {
            int32_t data_size = this->data_size(block);
            int32_t other_data_size = other->data_size(block);
            int32_t start = other_data_size;
            other->insert_space(block, other_data_size, data_size);

            codec.copy(this->values(block), 0, other->values(block), start, data_size);
        }

        other->size() += this->size();

        other->reindex();

        this->clear();
    }



    template <typename TreeType>
    void transferDataTo(TreeType* other) const
    {
        Codec codec;

        for (int32_t block = 0; block < Blocks; block++)
        {
            int32_t data_size = this->data_size(block);
            other->insertSpace(block, 0, data_size);
            codec.copy(this->values(block), 0, other->values(block), 0, data_size);
        }

        other->reindex();
    }


    void remove_space(int32_t start, int32_t end)
    {
        remove(start, end);
    }

    void removeSpace(int32_t start, int32_t end) {
        remove(start, end);
    }

    void remove(int32_t start, int32_t end)
    {
        for (int32_t block = 0; block < Blocks; block++)
        {
            const int32_t data_size = this->data_size(block);
            auto values         = this->values(block);
            TreeLayout layout   = compute_tree_layout(data_size);

            int32_t start_pos = this->locate(layout, values, block, start).idx;
            int32_t end_pos   = this->locate(layout, values, block, end).idx;

            this->remove_space(block, start_pos, end_pos - start_pos);
        }

        this->size() -= end - start;

        reindex();
    }




    template <typename T>
    void insert(int32_t idx, const core::StaticVector<T, Blocks>& values)
    {
        this->_insert(idx, 1, [&](int32_t block, int32_t idx) -> const auto& {
            return values[block];
        });
    }

    template <typename Adaptor>
    void insert(int32_t pos, int32_t processed, Adaptor&& adaptor) {
        _insert(pos, processed, std::forward<Adaptor>(adaptor));
    }


    template <typename Adaptor>
    void _insert(int32_t pos, int32_t processed, Adaptor&& adaptor)
    {
        int32_t size = this->size();

        MEMORIA_V1_ASSERT(pos, >=, 0);
        MEMORIA_V1_ASSERT(pos, <=, size);
        MEMORIA_V1_ASSERT(processed, >=, 0);

        Codec codec;

        SizesT total_lengths;

        for (SizeT c = 0; c < processed; c++)
        {
            for (int32_t block = 0; block < Blocks; block++)
            {
                const auto& value = adaptor(block, c);
                auto len = codec.length(value);
                total_lengths[block] += len;
            }
        }


        for (int32_t block = 0; block < Blocks; block++)
        {
            int32_t data_size       = this->data_size(block);
            auto values         = this->values(block);
            TreeLayout layout   = compute_tree_layout(data_size);

            auto lr = this->locate(layout, values, block, pos);

            size_t insertion_pos = lr.idx;

            this->insert_space(block, insertion_pos, total_lengths[block]);

            values = this->values(block);

            for (int32_t c = 0; c < processed; c++)
            {
                const auto& value = adaptor(block, c);
                auto len = codec.encode(values, value, insertion_pos);
                insertion_pos += len;
            }
        }

        this->size() += processed;

        reindex();
    }



    ReadState positions(int32_t idx) const
    {
        MEMORIA_V1_ASSERT(idx, >=, 0);

        if(idx > this->size()) {
            this->dump();
        }

        MEMORIA_V1_ASSERT(idx, <=, this->size());

        SizesT pos;
        for (int32_t block = 0; block < Blocks; block++)
        {
            int32_t data_size       = this->data_size(block);
            auto values         = this->values(block);
            TreeLayout layout   = compute_tree_layout(data_size);

            pos[block] = this->locate(layout, values, block, idx).idx;
        }

        return pos;
    }


    SizesT insert_buffer(SizesT at, const InputBuffer* buffer, SizesT starts, SizesT ends, int32_t size)
    {
        Codec codec;

        SizesT total_lengths = ends - starts;

        for (int32_t block = 0; block < Blocks; block++)
        {
            auto values = this->values(block);

            size_t insertion_pos = at[block];

            this->insert_space(block, insertion_pos, total_lengths[block]);

            values = this->values(block);

            codec.copy(buffer->values(block), starts[block], values, insertion_pos, total_lengths[block]);
        }

        this->size() += size;

        reindex();

        return at + total_lengths;
    }

    void insert_buffer(int32_t pos, const InputBuffer* buffer, int32_t start, int32_t size)
    {
        Codec codec;

        SizesT starts = buffer->positions(start);
        SizesT ends   = buffer->positions(start + size);

        SizesT at     = this->positions(pos);

        SizesT total_lengths = ends - starts;

        for (int32_t block = 0; block < Blocks; block++)
        {
            auto values = this->values(block);

            size_t insertion_pos = at[block];

            this->insert_space(block, insertion_pos, total_lengths[block]);

            values = this->values(block);

            codec.copy(buffer->values(block), starts[block], values, insertion_pos, total_lengths[block]);
        }

        this->size() += size;

        reindex();
    }



    template <typename Adaptor>
    SizesT populate(const SizesT& at, int32_t size, Adaptor&& adaptor)
    {
        Codec codec;

        SizesT total_lengths;

        for (int32_t c = 0; c < size; c++)
        {
            for (int32_t block = 0; block < Blocks; block++)
            {
                total_lengths[block] += codec.length(adaptor(block, c));
            }
        }

        for (int32_t block = 0; block < Blocks; block++)
        {
            size_t insertion_pos = at[block];

            auto values = this->values(block);

            for (int32_t c = 0; c < size; c++)
            {
                const auto& value = adaptor(block, c);
                auto len = codec.encode(values, value, insertion_pos);
                insertion_pos += len;
            }

            this->data_size(block) += total_lengths[block];
        }

        this->size() += size;

        return at + total_lengths;
    }


    template <typename T>
    void update(int32_t idx, const core::StaticVector<T, Blocks>& values)
    {
        setValues(idx, values);
    }



    template <int32_t Offset, int32_t Size, typename T1, typename T2, template <typename, int32_t> class BranchNodeEntryItem>
    void _insert(int32_t idx, const core::StaticVector<T1, Blocks>& values, BranchNodeEntryItem<T2, Size>& accum)
    {
        insert(idx, values);

        sum<Offset>(idx, accum);
    }

    template <int32_t Offset, int32_t Size, typename T1, typename T2, template <typename, int32_t> class BranchNodeEntryItem>
    void _update(int32_t idx, const core::StaticVector<T1, Blocks>& values, BranchNodeEntryItem<T2, Size>& accum)
    {
        sub<Offset>(idx, accum);

        update(idx, values);

        sum<Offset>(idx, accum);
    }


    template <int32_t Offset, int32_t Size, typename T1, typename T2, typename I, template <typename, int32_t> class BranchNodeEntryItem>
    void _update(int32_t idx, const std::pair<T1, I>& values, BranchNodeEntryItem<T2, Size>& accum)
    {
        sub<Offset>(idx, accum);

        this->setValue(values.first, idx, values.second);

        sum<Offset>(idx, accum);
    }

    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    void _remove(int32_t idx, BranchNodeEntryItem<T, Size>& accum)
    {
        sub<Offset>(idx, accum);
        remove(idx, idx + 1);
    }

    template <typename UpdateFn>
    void update_values(int32_t start, int32_t end, UpdateFn&& update_fn)
    {
        Codec codec;

        for (int32_t block = 0; block < Blocks; block++)
        {
            auto values         = this->values(block);
            int32_t data_size       = this->data_size(block);
            TreeLayout layout   = compute_tree_layout(data_size);
            size_t data_start   = this->locate(layout, values, block, start);

            for (int32_t window_start = start; window_start < end; window_start += 32)
            {
                int32_t window_end = (window_start + 32) < end ? window_start + 32 : end;

                int32_t old_length = 0;
                int32_t new_length = 0;

                auto values = this->values(block);

                size_t data_start_tmp = data_start;

                Value buffer[32];

                for (int32_t c = window_start; c < window_end; c++)
                {
                    Value old_value;
                    auto len = codec.decode(values, old_value, data_start_tmp, data_size);

                    auto new_value = update_fn(block, c, old_value);

                    buffer[c - window_start] = new_value;

                    old_length += len;
                    new_length += codec.length(new_value);

                    data_start_tmp += len;
                }

                if (new_length > old_length)
                {
                    auto delta = new_length - old_length;
                    this->insert_space(block, data_start, delta);

                    values = this->values(block);
                }
                else if (new_length < old_length)
                {
                    auto delta = old_length - new_length;
                    this->remove_space(block, data_start, delta);

                    values = this->values(block);
                }

                for (int32_t c = window_start; c < window_end; c++)
                {
                    data_start += codec.encode(values, buffer[c], data_start);
                }
            }

            reindex_block(block);
        }
    }


    template <typename UpdateFn>
    void update_values(int32_t start, UpdateFn&& update_fn)
    {
        for (int32_t block = 0; block < Blocks; block++)
        {
            update_value(block, start, std::forward<UpdateFn>(update_fn));
        }
    }


    template <typename UpdateFn>
    void update_value(int32_t block, int32_t start, UpdateFn&& update_fn)
    {
        MEMORIA_V1_ASSERT(start, <, this->size());
        MEMORIA_V1_ASSERT(start, >=, 0);
        MEMORIA_V1_ASSERT(block, >=, 0);
        MEMORIA_V1_ASSERT(block, <=, (int32_t)Blocks);

        Codec codec;

        int32_t data_size       = this->data_size(block);

        auto values         = this->values(block);
        TreeLayout layout   = compute_tree_layout(data_size);
        size_t insertion_pos = this->locate(layout, values, block, start).idx;

        Value value;
        size_t old_length = codec.decode(values, value, insertion_pos, data_size);
        auto new_value = update_fn(block, value);

        if (new_value != value)
        {
            size_t new_length = codec.length(new_value);

            if (new_length > old_length)
            {
                this->insert_space(block, insertion_pos, new_length - old_length);
                values = this->values(block);

            }
            else if (old_length > new_length)
            {
                this->remove_space(block, insertion_pos, old_length - new_length);
                values = this->values(block);
            }

            codec.encode(values, new_value, insertion_pos);

            reindex_block(block);
        }
    }



    template <typename T>
    void setValues(int32_t idx, const core::StaticVector<T, Blocks>& values)
    {
        update_values(idx, [&](int32_t block, auto old_value){return values[block];});
    }

    template <typename T>
    void addValues(int32_t idx, const core::StaticVector<T, Blocks>& values)
    {
        update_values(idx, [&](int32_t block, auto old_value){return values[block] + old_value;});
    }

    template <typename T>
    void subValues(int32_t idx, const core::StaticVector<T, Blocks>& values)
    {
        update_values(idx, [&](int32_t block, auto old_value){return values[block] + old_value;});
    }


    void addValue(int32_t block, int32_t idx, Value value)
    {
        update_value(block, idx, [&](int32_t block, auto old_value){return value + old_value;});
    }

    template <typename T, int32_t Indexes>
    void addValues(int32_t idx, int32_t from, int32_t size, const core::StaticVector<T, Indexes>& values)
    {
        for (int32_t block = 0; block < size; block++)
        {
            update_value(block, idx, [&](int32_t block, auto old_value){return values[block + from] + old_value;});
        }

        reindex();
    }



    void check() const {
        Base::check(Blocks);
    }

    void clear()
    {
        if (Base::has_allocator())
        {
            auto alloc = this->allocator();
            int32_t empty_size = MyType::empty_size();
            alloc->resizeBlock(this, empty_size);
        }

        init();
    }

    void clear(int32_t start, int32_t end)
    {
    }

    void generateDataEvents(IPageDataEventHandler* handler) const
    {
        Base::generateDataEvents(handler);

        handler->startStruct();
        handler->startGroup("VLQ_TREE");

        auto meta = this->metadata();

        handler->value("SIZE",      &meta->size());
        handler->value("DATA_SIZE", this->data_sizes(), Blocks);


        handler->startGroup("INDEXES", Blocks);

        for (int32_t block = 0; block < Blocks; block++)
        {
            int32_t index_size = this->index_size(this->data_size(block));

            handler->startGroup("BLOCK_INDEX", block);

            auto value_indexes = this->value_index(block);
            auto size_indexes  = this->size_index(block);

            for (int32_t c = 0; c < index_size; c++)
            {
                int64_t indexes[] = {
                    value_indexes[c],
                    size_indexes[c]
                };

                handler->value("INDEX", PageValueProviderFactory::provider(2, [&](int32_t idx) {
                    return indexes[idx];
                }));
            }

            handler->endGroup();
        }

        handler->endGroup();


        handler->startGroup("DATA", meta->size());

        const ValueData* values[Blocks];
        for (int32_t b = 0; b < Blocks; b++) {
            values[b] = this->values(b);
        }

        size_t positions[Blocks];
        for (auto& p: positions) p = 0;

        int32_t size = this->size();

        Codec codec;

        for (int32_t idx = 0; idx < size; idx++)
        {
            Value values_data[Blocks];
            for (int32_t block = 0; block < Blocks; block++)
            {
                auto len = codec.decode(values[block], values_data[block], positions[block]);
                positions[block] += len;
            }

            handler->value("TREE_ITEM", PageValueProviderFactory::provider(Blocks, [&](int32_t b) {
                return values_data[b];
            }));
        }

        handler->endGroup();

        handler->endGroup();

        handler->endStruct();
    }

    void serialize(SerializationData& buf) const
    {
        Base::serialize(buf);

        auto meta = this->metadata();

        FieldFactory<int32_t>::serialize(buf, meta->size());

        FieldFactory<int32_t>::serialize(buf, this->data_sizes(), Blocks);

        for (int32_t block = 0; block < Blocks; block++)
        {
            Base::template serializeSegment<IndexValue>(buf, block * SegmentsPerBlock + BlocksStart + VALUE_INDEX);
            Base::template serializeSegment<int32_t>(buf, block * SegmentsPerBlock + BlocksStart + SIZE_INDEX);
            Base::template serializeSegment<OffsetsType>(buf, block * SegmentsPerBlock + BlocksStart + OFFSETS);

            int32_t data_block_size = this->data_block_size(block);

            FieldFactory<ValueData>::serialize(buf, this->values(block), data_block_size);
        }
    }

    void deserialize(DeserializationData& buf)
    {
        Base::deserialize(buf);

        auto meta = this->metadata();

        FieldFactory<int32_t>::deserialize(buf, meta->size());

        FieldFactory<int32_t>::deserialize(buf, this->data_sizes(), Blocks);

        for (int32_t block = 0; block < Blocks; block++)
        {
            Base::template deserializeSegment<IndexValue>(buf, block * SegmentsPerBlock + BlocksStart + VALUE_INDEX);
            Base::template deserializeSegment<int32_t>(buf, block * SegmentsPerBlock + BlocksStart + SIZE_INDEX);
            Base::template deserializeSegment<OffsetsType>(buf, block * SegmentsPerBlock + BlocksStart + OFFSETS);

            int32_t data_block_size = this->data_block_size(block);

            FieldFactory<ValueData>::deserialize(buf, this->values(block), data_block_size);
        }
    }


    auto find_ge(int32_t block, IndexValue value) const
    {
        return find(block, FindGEWalker(value));
    }

    auto find_gt(int32_t block, IndexValue value) const
    {
        return find(block, FindGTWalker(value));
    }

    auto find_ge_fw(int32_t block, int32_t start, IndexValue value) const
    {
        return walk_fw(block, start, this->size(), FindGEWalker(value));
    }

    auto find_gt_fw(int32_t block, int32_t start, IndexValue value) const
    {
        return walk_fw(block, start, this->size(), FindGTWalker(value));
    }


    auto find_ge_bw(int32_t block, int32_t start, IndexValue value) const
    {
        return walk_bw(block, start, FindGEWalker(value));
    }

    auto find_gt_bw(int32_t block, int32_t start, IndexValue value) const
    {
        return walk_bw(block, start, FindGTWalker(value));
    }


    IndexValue sum(int32_t block) const
    {
        return gsum(block);
    }



    IndexValue sum(int32_t block, int32_t end) const
    {
        return gsum(block, end);
    }

    IndexValue plain_sum(int32_t block, int32_t end) const
    {
        return this->plain_gsum(block, end);
    }

    IndexValue sum(int32_t block, int32_t start, int32_t end) const
    {
        return gsum(block, start, end);
    }



    auto findGTForward(int32_t block, int32_t start, IndexValue val) const
    {
        return this->find_gt_fw(block, start, val);
    }

    auto findGTForward(int32_t block, IndexValue val) const
    {
        return this->find_gt(block, val);
    }



    auto findGTBackward(int32_t block, int32_t start, IndexValue val) const
    {
        return this->find_gt_bw(block, start, val);
    }

    auto findGTBackward(int32_t block, IndexValue val) const
    {
        return this->find_gt_bw(block, this->size() - 1, val);
    }



    auto findGEForward(int32_t block, int32_t start, IndexValue val) const
    {
        return this->find_ge_fw(block, start, val);
    }

    auto findGEForward(int32_t block, IndexValue val) const
    {
        return this->find_ge(block, val);
    }

    auto findGEBackward(int32_t block, int32_t start, IndexValue val) const
    {
        return this->find_ge_bw(block, start, val);
    }

    auto findGEBackward(int32_t block, IndexValue val) const
    {
        return this->find_ge_bw(block, this->size() - 1, val);
    }


    class FindResult {
        IndexValue prefix_;
        int32_t idx_;
    public:
        template <typename Fn>
        FindResult(Fn&& fn): prefix_(fn.prefix()), idx_(fn.idx()) {}

        IndexValue prefix() {return prefix_;}
        int32_t idx() const {return idx_;}
    };

    auto findForward(SearchType search_type, int32_t block, int32_t start, IndexValue val) const
    {
        if (search_type == SearchType::GT)
        {
            return FindResult(findGTForward(block, start, val));
        }
        else {
            return FindResult(findGEForward(block, start, val));
        }
    }

    auto findForward(SearchType search_type, int32_t block, IndexValue val) const
    {
        if (search_type == SearchType::GT)
        {
            return FindResult(findGTForward(block, val));
        }
        else {
            return FindResult(findGEForward(block, val));
        }
    }


    auto findBackward(SearchType search_type, int32_t block, int32_t start, IndexValue val) const
    {
        if (search_type == SearchType::GT)
        {
            return FindResult(findGTBackward(block, start, val));
        }
        else {
            return FindResult(findGEBackward(block, start, val));
        }
    }

    auto findBackward(SearchType search_type, int32_t block, IndexValue val) const
    {
        if (search_type == SearchType::GT)
        {
            return FindResult(findGTBackward(block, val));
        }
        else {
            return FindResult(findGEBackward(block, val));
        }
    }







    template <typename ConsumerFn>
    void read(int32_t block, int32_t start, int32_t end, ConsumerFn&& fn) const
    {
        MEMORIA_V1_ASSERT(start, >=, 0);
        MEMORIA_V1_ASSERT(start, <=, end);
        MEMORIA_V1_ASSERT(end, <=, this->size());

        auto values = this->values(block);
        TreeLayout layout = this->compute_tree_layout(this->data_size(block));
        size_t pos = this->locate(layout, values, block, start).idx;

        Codec codec;

        int32_t c;
        for (c = start; c < end; c++)
        {
            Value value;
            auto len = codec.decode(values, value, pos);
            fn(block, value);
            fn.next();
            pos += len;
        }
    }

    template <typename ConsumerFn>
    void read(int32_t start, int32_t end, ConsumerFn&& fn) const
    {
        MEMORIA_V1_ASSERT(start, >=, 0);
        MEMORIA_V1_ASSERT(start, <=, end);
        MEMORIA_V1_ASSERT(end, <=, this->size());

        const ValueData* values[Blocks];
        size_t positions[Blocks];

        for (int32_t b = 0; b < Blocks; b++)
        {
            values[b] = this->values(b);

            TreeLayout layout   = this->compute_tree_layout(this->data_size(b));
            positions[b]        = this->locate(layout, values[b], b, start).idx;
        }

        Codec codec;

        for (int32_t c = start; c < end; c++)
        {
            for (int32_t b = 0; b < Blocks; b++)
            {
                Value value;
                auto len = codec.decode(values[b], value, positions[b]);

                fn(b, value);
                fn.next();

                positions[b] += len;
            }
        }
    }


    template <typename T>
    void read(int32_t block, int32_t start, int32_t end, T* values) const
    {
        MEMORIA_V1_ASSERT(start, >=, 0);
        MEMORIA_V1_ASSERT(start, <=, end);
        MEMORIA_V1_ASSERT(end, <=, this->size());

        int32_t c = 0;
        read(block, start, end, make_fn_with_next([&](int32_t c, auto&& value){
            values[c] = value;
        }, [&]{c++;}));
    }

    void dump_block_values(std::ostream& out = std::cout) const
    {
        for (int32_t b = 0; b < Blocks; b++) {
            Base::dump_block(b, out);
        }
    }

    void dump(std::ostream& out = std::cout) const
    {
        auto meta = this->metadata();
        auto size = meta->size();

        out << "size_         = " << size << std::endl;
        out << "block_size_   = " << this->block_size() << std::endl;

        for (int32_t block = 0; block < Blocks; block++) {
            out << "data_size_[" << block << "] = " << this->data_size(block) << std::endl;
        }

        for (int32_t block = 0; block < Blocks; block++)
        {
            out << "++++++++++++++++++ Block: " << block << " ++++++++++++++++++" << std::endl;

            auto data_size  = this->data_size(block);
            auto index_size = this->index_size(data_size);

            out << "index_size_   = " << index_size << std::endl;

            TreeLayout layout = this->compute_tree_layout(data_size);

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

                auto value_indexes = this->value_index(block);
                auto size_indexes = this->size_index(block);

                out << "Index:" << std::endl;
                for (int32_t c = 0; c < index_size; c++)
                {
                    out << c << ": " << value_indexes[c] << " " << size_indexes[c] << std::endl;
                }
            }

            out << std::endl;

            out << "Offsets: ";
            for (int32_t c = 0; c <= this->divUpV(data_size); c++) {
                out << this->offset(block, c) << " ";
            }
            out << std::endl;
        }




        out << "Values: " << std::endl;

        const ValueData* values[Blocks];
        size_t block_pos[Blocks];

        for (int32_t block = 0; block < Blocks; block++) {
            values[block] = this->values(block);
            block_pos[block] = 0;
        }


        Codec codec;
        for (int32_t c = 0; c < size; c++)
        {
            out << c << ": " << c << " ";
            for (int32_t block = 0; block < Blocks; block++)
            {
                Value value;
                auto len = codec.decode(values[block], value, block_pos[block]);

                out << "  (" << block_pos[block] << ") " << value;
                block_pos[block] += len;
            }
            out << std::endl;
        }
    }

};




template <typename Types>
struct PkdStructSizeType<PkdVQTree<Types>> {
    static const PackedSizeType Value = PackedSizeType::VARIABLE;
};


template <typename Types>
struct StructSizeProvider<PkdVQTree<Types>> {
    static const int32_t Value = Types::Blocks;
};

template <typename Types>
struct IndexesSize<PkdVQTree<Types>> {
    static const int32_t Value = Types::Blocks;
};


}}
