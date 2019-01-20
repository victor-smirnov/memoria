
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

template <typename Types> class PkdVDTree;

template <
    typename IndexValueT,
    int32_t kBlocks,
    template <typename> class CodecT,
    typename ValueT = int64_t,
    int32_t kBranchingFactor = PkdVLETreeShapeProvider<CodecT<ValueT>>::BranchingFactor,
    int32_t kValuesPerBranch = PkdVLETreeShapeProvider<CodecT<ValueT>>::ValuesPerBranch
>
using PkdVDTreeT = PkdVDTree<PkdVLETreeTypes<IndexValueT, kBlocks, CodecT, ValueT, kBranchingFactor, kValuesPerBranch>>;


template <typename Types>
class PkdVDTree: public PkdVQTreeBase<typename Types::IndexValue, typename Types::Value, Types::template Codec, Types::BranchingFactor, Types::ValuesPerBranch> {

    using Base      = PkdVQTreeBase<typename Types::IndexValue, typename Types::Value, Types::template Codec, Types::BranchingFactor, Types::ValuesPerBranch>;
    using MyType    = PkdVDTree<Types>;

public:

    using Base::BlocksStart;
    using Base::SegmentsPerBlock;
    using Base::compute_tree_layout;
    using Base::gsum;
    using Base::find;
    using Base::walk_fw;
    using Base::walk_bw;
    using Base::metadata;
    using Base::offsets_segment_size;

    using Base::METADATA;
    using Base::DATA_SIZES;

    using Base::VALUES;
    using Base::VALUE_INDEX;
    using Base::OFFSETS;
    using Base::SIZE_INDEX;

    using typename Base::Metadata;
    using typename Base::TreeLayout;
    using typename Base::OffsetsType;
    using typename Base::ValueData;
    using typename Base::FindGEWalker;
    using typename Base::FindGTWalker;


    using typename Base::Codec;

    static constexpr uint32_t VERSION   = 1;
    static constexpr int32_t TreeBlocks = 1;
    static constexpr int32_t Blocks = Types::Blocks;

    using FieldsList = MergeLists<
                typename Base::FieldsList,
                ConstValue<uint32_t, VERSION>,
                ConstValue<uint32_t, TreeBlocks>,
                ConstValue<uint32_t, Blocks>
    >;

    using IndexValue = typename Types::IndexValue;
    using Value      = typename Types::Value;

    using Values = core::StaticVector<IndexValue, Blocks>;
    using DataValues = core::StaticVector<Value, Blocks>;

    using InputBuffer   = PkdVLEColumnOrderInputBuffer<Types>;
    using InputType     = Values;

    using SizesT = core::StaticVector<int32_t, Blocks>;

    using ReadState = SizesT;

    static int32_t estimate_block_size(int32_t tree_capacity, int32_t density_hi = 1000, int32_t density_lo = 333)
    {
        int32_t max_tree_capacity = (tree_capacity * Blocks * density_hi) / density_lo;
        return block_size(max_tree_capacity);
    }



    OpStatus init_tl(int32_t data_block_size)
    {
        return Base::init_tl(data_block_size, TreeBlocks);
    }

    OpStatus init(const SizesT& sizes) {
        return MyType::init(sizes.sum());
    }

    OpStatus init(int32_t total_capacity)
    {
        if(isFail(Base::init(empty_size(), TreeBlocks * SegmentsPerBlock + BlocksStart))) {
            return OpStatus::FAIL;
        }

        Metadata* meta = this->template allocate<Metadata>(METADATA);
        if(isFail(meta)) {
            return OpStatus::FAIL;
        }

        if(isFail(this->template allocateArrayBySize<int32_t>(DATA_SIZES, TreeBlocks))) {
            return OpStatus::FAIL;
        }

        meta->size() = 0;

        for (int32_t block = 0; block < TreeBlocks; block++)
        {
            int32_t capacity        = total_capacity;
            int32_t offsets_size    = offsets_segment_size(capacity);
            int32_t index_size      = this->index_size(capacity);
            int32_t values_segment_length = this->value_segment_size(capacity);

            if(isFail(this->resizeBlock(block * SegmentsPerBlock + VALUE_INDEX + BlocksStart, index_size * sizeof(IndexValue)))){
                return OpStatus::FAIL;
            }
            if(isFail(this->resizeBlock(block * SegmentsPerBlock + SIZE_INDEX + BlocksStart, index_size * sizeof(int32_t)))) {
                return OpStatus::FAIL;
            }
            if(isFail(this->resizeBlock(block * SegmentsPerBlock + OFFSETS + BlocksStart, offsets_size))) {
                return OpStatus::FAIL;
            }
            if(isFail(this->resizeBlock(block * SegmentsPerBlock + VALUES + BlocksStart, values_segment_length))) {
                return OpStatus::FAIL;
            }
        }

        return OpStatus::OK;
    }

    OpStatus init_bs(int32_t block_size)
    {
        if(isFail(Base::init(block_size, TreeBlocks * SegmentsPerBlock + BlocksStart))) {
            return OpStatus::FAIL;
        }

        Metadata* meta = this->template allocate<Metadata>(METADATA);
        if(isFail(meta)) {
            return OpStatus::FAIL;
        }

        if(isFail(this->template allocateArrayBySize<int32_t>(DATA_SIZES, TreeBlocks))) {
            return OpStatus::FAIL;
        }

        meta->size() = 0;
        int32_t offsets_size = offsets_segment_size(0);

        for (int32_t block = 0; block < TreeBlocks; block++)
        {
            if(isFail(this->template allocateArrayBySize<IndexValue>(block * SegmentsPerBlock + VALUE_INDEX + BlocksStart, 0))) {
                return OpStatus::FAIL;
            }
            if(isFail(this->template allocateArrayBySize<int32_t>(block * SegmentsPerBlock + SIZE_INDEX + BlocksStart, 0))) {
                return OpStatus::FAIL;
            }
            if(isFail(this->template allocateArrayBySize<int8_t>(block * SegmentsPerBlock + OFFSETS + BlocksStart, offsets_size))) {
                return OpStatus::FAIL;
            }
            if(isFail(this->template allocateArrayBySize<int8_t>(block * SegmentsPerBlock + VALUES + BlocksStart, 0))) {
                return OpStatus::FAIL;
            }
        }

        return OpStatus::OK;
    }

    OpStatus init()
    {
        return init_bs(empty_size());
    }


    static int32_t block_size(int32_t capacity)
    {
        return Base::block_size_equi(TreeBlocks, capacity * Blocks);
    }

    static int32_t block_size(const SizesT& capacity)
    {
        return Base::block_size_equi(TreeBlocks, capacity.sum());
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
        return block_size(this->data_size() + other->data_size());
    }

    ValueData* values() {
        return Base::values(0);
    }

    const ValueData* values() const {
        return Base::values(0);
    }

    const int32_t& data_size() const {
        return Base::data_size(0);
    }

    int32_t& data_size() {
        return Base::data_size(0);
    }

    int32_t data_block_size(int32_t block) const
    {
        int32_t size = this->element_size(block * SegmentsPerBlock + VALUES + BlocksStart);
        return PackedAllocatable::roundUpBytesToAlignmentBlocks(size) / sizeof(ValueData);
    }

    int32_t size() const
    {
        return this->metadata()->size() / Blocks;
    }


    static int32_t elements_for(int32_t block_size)
    {
        return Base::tree_size(TreeBlocks, block_size);
    }

    static int32_t expected_block_size(int32_t items_num)
    {
        return block_size(items_num);
    }



    Value value(int32_t block, int32_t idx) const
    {
        int32_t size = this->size();

        if (idx >= size) {
            this->dump();
        }

        MEMORIA_V1_ASSERT(idx, >=, 0);
        MEMORIA_V1_ASSERT(idx, <, size);

        int32_t data_size     = this->data_size();
        auto values       = this->values();
        TreeLayout layout = this->compute_tree_layout(data_size);

        int32_t global_idx = idx + size * block;
        int32_t start_pos     = this->locate(layout, values, 0, global_idx).idx;

        MEMORIA_V1_ASSERT(start_pos, <, data_size);

        Codec codec;
        Value value;

        codec.decode(values, value, start_pos);

        return value;
    }

    static int32_t empty_size()
    {
        return block_size(SizesT());
    }

    OpStatus reindex() {
        return Base::reindex(TreeBlocks);
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
    OpStatus resize_segments(int32_t new_data_size)
    {
        int32_t block = 0;

        int32_t data_segment_size    = PackedAllocator::roundUpBitsToAlignmentBlocks(new_data_size * Codec::ElementSize);
        int32_t index_size           = Base::index_size(new_data_size);
        int32_t offsets_segment_size = Base::offsets_segment_size(new_data_size);

        if(isFail(this->resizeBlock(block * SegmentsPerBlock + VALUES + BlocksStart, data_segment_size))) {
            return OpStatus::FAIL;
        }

        if(isFail(this->resizeBlock(block * SegmentsPerBlock + OFFSETS + BlocksStart, offsets_segment_size))) {
            return OpStatus::FAIL;
        }

        if(isFail(this->resizeBlock(block * SegmentsPerBlock + SIZE_INDEX + BlocksStart, index_size * sizeof(int32_t)))) {
            return OpStatus::FAIL;
        }

        if(isFail(this->resizeBlock(block * SegmentsPerBlock + VALUE_INDEX + BlocksStart, index_size * sizeof(IndexValue)))) {
            return OpStatus::FAIL;
        }

        return OpStatus::OK;
    }

    OpStatus insert_space(int32_t start, int32_t length)
    {
        int32_t& data_size = this->data_size();
        if(isFail(resize_segments(data_size + length))) {
            return OpStatus::FAIL;
        }

        Codec codec;
        codec.move(this->values(), start, start + length, data_size - start);

        data_size += length;

        return OpStatus::OK;
    }

    OpStatus remove_space(int32_t start, int32_t length)
    {
        int32_t& data_size = this->data_size();

        Codec codec;
        codec.move(this->values(), start + length, start, data_size - (start + length));

        if(isFail(resize_segments(data_size - length))) {
            return OpStatus::FAIL;
        }

        data_size -= length;

        return OpStatus::OK;
    }




//    void copyTo(MyType* other, int32_t copy_from, int32_t count, int32_t copy_to) const
//    {
//      MEMORIA_V1_ASSERT_TRUE(copy_from >= 0);
//      MEMORIA_V1_ASSERT_TRUE(count >= 0);
//
//      for (int32_t block = 0; block < Blocks; block++)
//      {
//          auto my_values    = this->values(block);
//          auto other_values = other->values(block);
//
//          CopyBuffer(
//                  my_values + copy_from,
//                  other_values + copy_to,
//                  count
//          );
//      }
//    }

public:
    OpStatus splitTo(MyType* other, int32_t idx)
    {
        int32_t size = this->size();
        int32_t other_size = other->size();

        int32_t other_lengths[Blocks];

        for (int32_t block = 0; block < Blocks; block++)
        {
            int32_t start = other->locate(0, block * other_size + 0);
            int32_t end = other->locate(0, block * other_size + other_size);

            other_lengths[block] = end - start;
        }

        Codec codec;
        int32_t insertion_pos = 0;
        for (int32_t block = 0; block < Blocks; block++)
        {
            int32_t start = this->locate(0, block * size + idx);
            int32_t end   = this->locate(0, block * size + size);

            int32_t length = end - start;

            if(isFail(other->insert_space(insertion_pos, length))) {
                return OpStatus::FAIL;
            }
            codec.copy(this->values(), start, other->values(), insertion_pos, length);

            insertion_pos += length + other_lengths[block];
        }

        other->metadata()->size() += (size - idx) * Blocks;

        if(isFail(other->reindex())) {
            return OpStatus::FAIL;
        }

        return remove(idx, size);
    }


    OpStatus mergeWith(MyType* other)
    {
        int32_t size = this->size();
        int32_t other_size = other->size();

        int32_t other_lengths[Blocks];

        for (int32_t block = 0; block < Blocks; block++)
        {
            int32_t start = other->locate(0, block * other_size + 0);
            int32_t end = other->locate(0, block * other_size + other_size);

            other_lengths[block] = end - start;
        }

        Codec codec;
        int32_t insertion_pos = 0;
        for (int32_t block = 0; block < Blocks; block++)
        {
            insertion_pos += other_lengths[block];

            int32_t start = this->locate(0, block * size);
            int32_t end   = this->locate(0, block * size + size);

            int32_t length = end - start;

            if(isFail(other->insert_space(insertion_pos, length))) {
                return OpStatus::FAIL;
            }
            codec.copy(this->values(), start, other->values(), insertion_pos, length);

            insertion_pos += length;
        }

        other->metadata()->size() += size * Blocks;

        if(isFail(other->reindex())) {
            return OpStatus::FAIL;
        }

        return this->clear();
    }


    template <typename TreeType>
    OpStatus transferDataTo(TreeType* other) const
    {
        Codec codec;

        int32_t data_size = this->data_size();

        if(isFail(other->insertSpace(0, data_size))) {
            return OpStatus::FAIL;
        }

        codec.copy(this->values(), 0, other->values(), 0, data_size);

        return other->reindex();
    }


    OpStatus removeSpace(int32_t start, int32_t end) {
        return remove(start, end);
    }

    OpStatus remove(int32_t start, int32_t end)
    {
        if (end > start)
        {
            int32_t& data_size  = this->data_size();
            auto values         = this->values();
            TreeLayout layout   = compute_tree_layout(data_size);
            int32_t size        = this->size();

            Codec codec;

            int32_t start_pos[Blocks];
            int32_t lengths[Blocks];

            for (int32_t block = 0; block < Blocks; block++)
            {
                start_pos[block] = this->locate(layout, values, 0, start + size * block).idx;
                int32_t end_pos      = this->locate(layout, values, 0, end + size * block).idx;

                lengths[block] = end_pos - start_pos[block];
            }

            int32_t total_length = 0;

            for (int32_t block = Blocks - 1; block >= 0; block--)
            {
                int32_t length = lengths[block];
                int32_t end = start_pos[block] + length;
                int32_t start = start_pos[block];

                codec.move(values, end, start, data_size - end);

                total_length += length;
                data_size -= length;
            }

            if(isFail(resize_segments(data_size))) {
                return OpStatus::FAIL;
            }

            metadata()->size() -= (end - start) * Blocks;

            return reindex();
        }

        return OpStatus::OK;
    }




    template <typename T>
    OpStatus insert(int32_t idx, const core::StaticVector<T, Blocks>& values)
    {
        return this->_insert(idx, 1, [&](int32_t block, int32_t idx) -> const auto& {
            return values[block];
        });
    }

    template <typename Adaptor>
    OpStatus insert(int32_t pos, int32_t processed, Adaptor&& adaptor) {
        return _insert(pos, processed, std::forward<Adaptor>(adaptor));
    }


    template <typename Adaptor>
    OpStatus _insert(int32_t idx, int32_t inserted, Adaptor&& adaptor)
    {
        Codec codec;

        SizesT total_lengths;
        int32_t total_length = 0;

        int32_t positions[Blocks];
        int32_t size      = this->size();
        int32_t& data_size = this->data_size();

        TreeLayout layout = compute_tree_layout(data_size);

        auto values = this->values();

        for (int32_t block = 0; block < Blocks; block++)
        {
            int32_t global_idx   = size * block + idx;
            positions[block] = this->locate(layout, values, 0, global_idx).idx;

            for (SizeT c = 0; c < inserted; c++)
            {
                const auto& value = adaptor(block, c);
                auto len = codec.length(value);

                total_lengths[block] += len;
            }

            total_length += total_lengths[block];
        }


        if(isFail(resize_segments(data_size + total_length))) {
            return OpStatus::FAIL;
        }

        values = this->values();

        int32_t shift = 0;

        for (int32_t block = 0; block < Blocks; block++)
        {
            size_t insertion_pos = positions[block] + shift;
            codec.move(values, insertion_pos, insertion_pos + total_lengths[block], data_size - insertion_pos);

            for (int32_t c = 0; c < inserted; c++)
            {
                const auto& value = adaptor(block, c);
                auto len = codec.encode(values, value, insertion_pos);
                insertion_pos += len;
            }

            shift += total_lengths[block];

            data_size += total_lengths[block];
        }

        metadata()->size() += (inserted * Blocks);

        return reindex();
    }


    ReadState positions(int32_t idx) const
    {
        int32_t size = this->size();

        MEMORIA_V1_ASSERT(idx, >=, 0);
        MEMORIA_V1_ASSERT(idx, <=, size);

        int32_t data_size       = this->data_size();
        auto values         = this->values();
        TreeLayout layout   = compute_tree_layout(data_size);

        SizesT pos;
        for (int32_t block = 0; block < Blocks; block++)
        {
            pos[block] = this->locate(layout, values, 0, size * block + idx).idx;
        }

        return pos;
    }

    OpStatusT<SizesT> insert_buffer(SizesT at, const InputBuffer* buffer, SizesT starts, SizesT ends, int32_t inserted)
    {
        Codec codec;

        size_t data_size = this->data_size();

        SizesT total_lengths = ends - starts;
        int32_t total_length = total_lengths.sum();

        if(isFail(resize_segments(data_size + total_length))) {
            return OpStatus::FAIL;
        }

        auto values = this->values();

        int32_t shift = 0;

        for (int32_t block = 0; block < Blocks; block++)
        {
            size_t insertion_pos = at[block] + shift;
            codec.move(values, insertion_pos, insertion_pos + total_lengths[block], data_size - insertion_pos);

            codec.copy(buffer->values(block), starts[block], values, insertion_pos, total_lengths[block]);

            shift += total_lengths[block];

            data_size += total_lengths[block];

            at[block] = insertion_pos + total_lengths[block];
        }

        this->data_size() += total_length;

        metadata()->size() += (inserted * Blocks);

        if(isFail(reindex())) {
            return OpStatus::FAIL;
        }

        return OpStatusT<SizesT>(at);
    }


    OpStatus insert_buffer(int32_t pos, const InputBuffer* buffer, int32_t start, int32_t inserted)
    {
        Codec codec;

        size_t data_size = this->data_size();

        SizesT starts = buffer->positions(start);
        SizesT ends   = buffer->positions(start + inserted);
        SizesT at     = this->positions(pos);

        SizesT total_lengths = ends - starts;
        int32_t total_length = total_lengths.sum();

        if(isFail(resize_segments(data_size + total_length))) {
            return OpStatus::FAIL;
        }

        auto values = this->values();

        int32_t shift = 0;

        for (int32_t block = 0; block < Blocks; block++)
        {
            size_t insertion_pos = at[block] + shift;
            codec.move(values, insertion_pos, insertion_pos + total_lengths[block], data_size - insertion_pos);

            codec.copy(buffer->values(block), starts[block], values, insertion_pos, total_lengths[block]);

            shift += total_lengths[block];

            data_size += total_lengths[block];

            at[block] = insertion_pos + total_lengths[block];
        }

        this->data_size() += total_length;

        metadata()->size() += (inserted * Blocks);

        return reindex();
    }




    template <typename Adaptor>
    OpStatusT<SizesT> populate(SizesT at, int32_t size, Adaptor&& adaptor)
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

        size_t data_size = this->data_size();
        auto values = this->values();

        int32_t shift = 0;

        for (int32_t block = 0; block < Blocks; block++)
        {
            size_t insertion_pos = at[block] + shift;

            codec.move(values, insertion_pos, insertion_pos + total_lengths[block], data_size - insertion_pos);

            for (int32_t c = 0; c < size; c++)
            {
                const auto& value = adaptor(block, c);
                auto len = codec.encode(values, value, insertion_pos);
                insertion_pos += len;
            }

            at[block] = insertion_pos;

            shift += total_lengths[block];

            data_size += total_lengths[block];
        }

        this->data_size() = data_size;

        metadata()->size() += (size * Blocks);

        return OpStatusT<SizesT>(at);
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

    template <int32_t Offset, int32_t Size, typename T1, typename T2, template <typename, int32_t> class BranchNodeEntryItem>
    OpStatus _update(int32_t idx, const core::StaticVector<T1, Blocks>& values, BranchNodeEntryItem<T2, Size>& accum)
    {
        sub<Offset>(idx, accum);

        if(isFail(update(idx, values))) {
            return OpStatus::FAIL;
        }

        sum<Offset>(idx, accum);

        return OpStatus::OK;
    }


    template <int32_t Offset, int32_t Size, typename T1, typename T2, typename I, template <typename, int32_t> class BranchNodeEntryItem>
    OpStatus _update(int32_t idx, const std::pair<T1, I>& values, BranchNodeEntryItem<T2, Size>& accum)
    {
        sub<Offset>(idx, accum);

        if(isFail(this->setValue(values.first, idx, values.second))) {
            return OpStatus::FAIL;
        }

        sum<Offset>(idx, accum);

        return OpStatus::OK;
    }

    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    OpStatus _remove(int32_t idx, BranchNodeEntryItem<T, Size>& accum)
    {
        sub<Offset>(idx, accum);
        remove(idx, idx + 1);

        return OpStatus::OK;
    }

    template <typename UpdateFn>
    OpStatus update_values(int32_t start, int32_t end, UpdateFn&& update_fn)
    {
        auto values         = this->values();
        int32_t data_size       = this->data_size();
        TreeLayout layout   = compute_tree_layout(data_size);
        int32_t size            = this->size();

        Codec codec;

        int32_t starts[Blocks];

        for (int32_t block = 0; block < Blocks; block++)
        {
            starts[block] = this->locate(layout, values, 0, block * size + start);
        }

        int32_t shift = 0;

        for (int32_t block = 0; block < Blocks; block++)
        {
            int32_t total_delta = 0;

            for (int32_t window_start = start; window_start < end; window_start += 32)
            {
                int32_t window_end = (window_start + 32) < end ? window_start + 32 : end;

                int32_t old_length = 0;
                int32_t new_length = 0;

                size_t data_start_tmp = starts[block];

                Value buffer[32];

                for (int32_t c = window_start; c < window_end; c++)
                {
                    Value old_value;
                    auto len = codec.decode(values, old_value, data_start_tmp);

                    const auto& new_value = update_fn(block, c, old_value);

                    buffer[c - window_start] = new_value;

                    old_length += len;
                    new_length += codec.length(new_value);

                    data_start_tmp += len;
                }

                size_t data_start = starts[block] + shift;

                if (new_length > old_length)
                {
                    auto delta = new_length - old_length;

                    insert_space(data_start, delta);

                    values = this->values(block);
                    total_delta += delta;
                }
                else if (new_length < old_length)
                {
                    auto delta = old_length - new_length;

                    remove_space(data_start, delta);

                    values = this->values(block);
                    total_delta -= delta;
                }

                for (int32_t c = window_start; c < window_end; c++)
                {
                    data_start += codec.encode(values, buffer[c], data_start);
                }
            }

            shift += total_delta;

//          for (int32_t b1 = block; b1 < Blocks; b1++) {
//              starts[b1] += total_delta;
//          }
        }

        return reindex();
    }


    template <typename UpdateFn>
    OpStatus update_values(int32_t start, UpdateFn&& update_fn)
    {
        for (int32_t block = 0; block < Blocks; block++)
        {
            if(isFail(update_value(block, start, std::forward<UpdateFn>(update_fn)))) {
                return OpStatus::FAIL;
            }
        }

        return OpStatus::OK;
    }


    template <typename UpdateFn>
    OpStatus update_value(int32_t block, int32_t start, UpdateFn&& update_fn)
    {
        int32_t size       = this->size();

        MEMORIA_V1_ASSERT(start, >=, 0);
        MEMORIA_V1_ASSERT(start, <, size);

        int32_t global_idx = block * size + start;

        Codec codec;

        int32_t data_size       = this->data_size();
        auto values         = this->values();
        TreeLayout layout   = compute_tree_layout(data_size);

        size_t insertion_pos = this->locate(layout, values, 0, global_idx).idx;

        Value value;
        size_t old_length = codec.decode(values, value, insertion_pos);
        const auto& new_value    = update_fn(block, value);

        if (new_value != value)
        {
            size_t new_length = codec.length(new_value);

            if (new_length > old_length)
            {
                if(isFail(insert_space(insertion_pos, new_length - old_length))) {
                    return OpStatus::FAIL;
                }

                values = this->values();
            }
            else if (old_length > new_length)
            {
                if(isFail(remove_space(insertion_pos, old_length - new_length))) {
                    return OpStatus::FAIL;
                }

                values = this->values();
            }

            codec.encode(values, new_value, insertion_pos);

            return reindex();
        }

        return OpStatus::OK;
    }




    template <typename T>
    OpStatus setValues(int32_t idx, const core::StaticVector<T, Blocks>& values)
    {
        return update_values(idx, [&](int32_t block, auto old_value){return values[block];});
    }

    template <typename T>
    OpStatus addValues(int32_t idx, const core::StaticVector<T, Blocks>& values)
    {
        return update_values(idx, [&](int32_t block, auto old_value){return values[block] + old_value;});
    }

    template <typename T>
    OpStatus subValues(int32_t idx, const core::StaticVector<T, Blocks>& values)
    {
        return update_values(idx, [&](int32_t block, auto old_value){return values[block] + old_value;});
    }


    OpStatus addValue(int32_t block, int32_t idx, const Value& value)
    {
        return update_value(block, idx, [&](int32_t block, auto old_value){return value + old_value;});
    }

    template <typename T, int32_t Indexes>
    OpStatus addValues(int32_t idx, int32_t from, int32_t size, const core::StaticVector<T, Indexes>& values)
    {
        for (int32_t block = 0; block < size; block++)
        {
            if(isFail(update_value(block, idx, [&](int32_t block, auto old_value){return values[block + from] + old_value;}))) {
                return OpStatus::FAIL;
            }
        }

        return OpStatus::OK;
    }




    void check() const {}

    OpStatus clear()
    {
        if (Base::has_allocator())
        {
            auto alloc = this->allocator();
            int32_t empty_size = MyType::empty_size();
            if(isFail(alloc->resizeBlock(this, empty_size))) {
                return OpStatus::FAIL;
            }
        }

        return init();
    }

    OpStatus clear(int32_t start, int32_t end)
    {
        return OpStatus::OK;
    }

    void generateDataEvents(IBlockDataEventHandler* handler) const
    {
        Base::generateDataEvents(handler);

        handler->startStruct();
        handler->startGroup("VLD_TREE");

        auto meta = this->metadata();

        handler->value("SIZE",      &meta->size());
        handler->value("DATA_SIZE", this->data_sizes(), TreeBlocks);

        handler->startGroup("INDEXES", TreeBlocks);

        for (int32_t block = 0; block < TreeBlocks; block++)
        {
            int32_t index_size = this->index_size(Base::data_size(block));

            handler->startGroup("BLOCK_INDEX", block);

            auto value_indexes = this->value_index(block);
            auto size_indexes  = this->size_index(block);

            for (int32_t c = 0; c < index_size; c++)
            {
                int64_t indexes[] = {
                    value_indexes[c],
                    size_indexes[c]
                };

                handler->value("INDEX", BlockValueProviderFactory::provider(2, [&](int32_t idx) {
                    return indexes[idx];
                }));
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

            handler->value("TREE_ITEM", BlockValueProviderFactory::provider(Blocks, [&](int32_t b) {
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

        FieldFactory<int32_t>::serialize(buf, this->data_sizes(), TreeBlocks);

        for (int32_t block = 0; block < TreeBlocks; block++)
        {
            Base::template serializeSegment<IndexValue>(buf, block * SegmentsPerBlock + BlocksStart + VALUE_INDEX);
            Base::template serializeSegment<int32_t>(buf, block * SegmentsPerBlock + BlocksStart + SIZE_INDEX);
            Base::template serializeSegment<OffsetsType>(buf, block * SegmentsPerBlock + BlocksStart + OFFSETS);

            int32_t data_size = this->data_block_size(block);

            FieldFactory<ValueData>::serialize(buf, Base::values(block), data_size);
        }
    }

    void deserialize(DeserializationData& buf)
    {
        Base::deserialize(buf);

        auto meta = this->metadata();

        FieldFactory<int32_t>::deserialize(buf, meta->size());

        FieldFactory<int32_t>::deserialize(buf, this->data_sizes(), TreeBlocks);

        for (int32_t block = 0; block < TreeBlocks; block++)
        {
            Base::template deserializeSegment<IndexValue>(buf, block * SegmentsPerBlock + BlocksStart + VALUE_INDEX);
            Base::template deserializeSegment<int32_t>(buf, block * SegmentsPerBlock + BlocksStart + SIZE_INDEX);
            Base::template deserializeSegment<OffsetsType>(buf, block * SegmentsPerBlock + BlocksStart + OFFSETS);

            int32_t data_size = this->data_block_size(block);

            FieldFactory<ValueData>::deserialize(buf, Base::values(block), data_size);
        }
    }


    auto find_ge(int32_t block, const IndexValue& value) const
    {
        int32_t size = this->size();
        int32_t block_start = block * size;

        auto sum = this->gsum(0, block_start);

        auto result = find(0, FindGEWalker(value + sum));

        if (result.local_pos() < block_start + size)
        {
            return result.adjust_s(block_start, size, sum);
        }
        else {
            auto block_sum = this->gsum(0, block_start + size) - sum;
            return result.adjust(block_start, size, block_sum);
        }
    }

    auto find_gt(int32_t block, const IndexValue& value) const
    {
        int32_t size = this->size();
        int32_t block_start = block * size;

        auto sum = this->gsum(0, block_start);

        auto result = find(0, FindGTWalker(value + sum));

        if (result.local_pos() < block_start + size)
        {
            return result.adjust_s(block_start, size, sum);
        }
        else {
            auto block_sum = this->gsum(0, block_start + size) - sum;
            return result.adjust(block_start, size, block_sum);
        }
    }

    auto find_ge_fw(int32_t block, int32_t start, const IndexValue& value) const
    {
        int32_t size = this->size();
        int32_t block_start = block * size;
        auto result = walk_fw(0, block_start + start, block_start + size, FindGEWalker(value));

        if (result.local_pos() < block_start + size)
        {
            return result.adjust(block_start, size);
        }
        else {
            auto sum = this->gsum(0, block_start + start, block_start + size);
            return result.adjust(block_start, size, sum);
        }
    }

    auto find_gt_fw(int32_t block, int32_t start, const IndexValue& value) const
    {
        int32_t size = this->size();
        int32_t block_start = block * size;
        auto result = walk_fw(0, block_start + start, block_start + size, FindGTWalker(value));


        if (result.local_pos() < block_start + size)
        {
            return result.adjust(block_start, size);
        }
        else {
            auto sum = this->gsum(0, block_start + start, block_start + size);
            return result.adjust(block_start, size, sum);
        }
    }


    auto find_ge_bw(int32_t block, int32_t start, const IndexValue& value) const
    {
        int32_t size = this->size();
        int32_t block_start = block * size;
        auto result = walk_bw(0, block_start + start, FindGEWalker(value));

        if (result.local_pos() >= block_start)
        {
            return result.adjust(block_start, size);
        }
        else {
            auto sum = this->gsum(0, block_start, block_start + start + 1);
            return result.adjust_bw(sum);
        }
    }

    auto find_gt_bw(int32_t block, int32_t start, const IndexValue& value) const
    {
        int32_t size = this->size();
        int32_t block_start = block * size;
        auto result = walk_bw(0, block_start + start, FindGTWalker(value));

        if (result.local_pos() >= block_start)
        {
            return result.adjust(block_start, size);
        }
        else {
            auto sum = this->gsum(0, block_start, block_start + start + 1);
            return result.adjust_bw(sum);
        }
    }


    IndexValue sum(int32_t block) const
    {
        int32_t size = this->size();
        return gsum(0, size * block, size * block + size);
    }



    IndexValue sum(int32_t block, int32_t end) const
    {
        int32_t size = this->size();
        return gsum(0, size * block, size * block + end);
    }

    IndexValue plain_sum(int32_t block, int32_t end) const
    {
        int32_t size = this->size();
        return this->plain_gsum(0, size * block + end) - this->plain_gsum(0, size * block);
    }

    IndexValue sum(int32_t block, int32_t start, int32_t end) const
    {
        int32_t size = this->size();
        return gsum(0, size * block + start, size * block + end);
    }



    auto findGTForward(int32_t block, int32_t start, const IndexValue& val) const
    {
        return this->find_gt_fw(block, start, val);
    }

    auto findGTForward(int32_t block, const IndexValue& val) const
    {
        return this->find_gt(block, val);
    }



    auto findGTBackward(int32_t block, int32_t start, const IndexValue& val) const
    {
        return this->find_gt_bw(block, start, val);
    }

    auto findGTBackward(int32_t block, const IndexValue& val) const
    {
        return this->find_gt_bw(block, this->size() - 1, val);
    }



    auto findGEForward(int32_t block, int32_t start, const IndexValue& val) const
    {
        return this->find_ge_fw(block, start, val);
    }

    auto findGEForward(int32_t block, const IndexValue& val) const
    {
        return this->find_ge(block, val);
    }

    auto findGEBackward(int32_t block, int32_t start, const IndexValue& val) const
    {
        return this->find_ge_bw(block, start, val);
    }

    auto findGEBackward(int32_t block, const IndexValue& val) const
    {
        return this->find_ge_bw(block, this->size() - 1, val);
    }

    class FindResult {
        IndexValue prefix_;
        int32_t idx_;
    public:
        template <typename Fn>
        FindResult(Fn&& fn): prefix_(fn.prefix()), idx_(fn.local_pos()) {}

        IndexValue prefix() {return prefix_;}
        int32_t local_pos() const {return idx_;}
    };

    auto findForward(SearchType search_type, int32_t block, int32_t start, const IndexValue& val) const
    {
        if (search_type == SearchType::GT)
        {
            return FindResult(findGTForward(block, start, val));
        }
        else {
            return FindResult(findGEForward(block, start, val));
        }
    }

    auto findBackward(SearchType search_type, int32_t block, int32_t start, const IndexValue& val) const
    {
        if (search_type == SearchType::GT)
        {
            return FindResult(findGTBackward(block, start, val));
        }
        else {
            return FindResult(findGEBackward(block, start, val));
        }
    }



    template <typename ConsumerFn>
    int32_t read(int32_t block, int32_t start, int32_t end, ConsumerFn&& fn) const
    {
        int32_t size = this->size();
        int32_t global_idx = block * size + start;

        size_t pos = this->locate(0, global_idx);

        Codec codec;

        auto values = this->values();

        int32_t c;
        for (c = start; c < end; c++)
        {
            Value value;
            auto len = codec.decode(values, value, pos);
            fn(block, value);
            pos += len;
            fn.next();
        }

        return c;
    }

    template <typename ConsumerFn>
    int32_t read(int32_t start, int32_t end, ConsumerFn&& fn) const
    {
        int32_t size = this->size();

        Codec codec;

        auto values = this->values();

        size_t pos[Blocks];

        for (int32_t b = 0; b < Blocks; b++)
        {
            int32_t global_idx = b * size + start;
            pos[b] = this->locate(0, global_idx);
        }

        int32_t c;
        for (c = start; c < end; c++)
        {
            for (int32_t b = 0; b  < Blocks; b++)
            {
                Value value;
                auto len = codec.decode(values, value, pos[b]);
                fn(b, value);
                pos[b] += len;
            }

            fn.next();
        }

        return c;
    }



    template <typename T>
    void read(int32_t block, int32_t start, int32_t end, T* values) const
    {
        MEMORIA_V1_ASSERT(start, >=, 0);
        MEMORIA_V1_ASSERT(start, <=, end);
        MEMORIA_V1_ASSERT(end, <=, size());

        int32_t idx = 0;
        scan(block, start, end, make_fn_with_next([&](int32_t block, auto value){
            values[idx - start] = value;
        }), [&]{idx++;});
    }


    void dump(std::ostream& out = std::cout) const
    {
        int32_t size = this->size();

        auto data_size  = this->data_size();

        out << "size_         = " << size << std::endl;
        out << "block_size_   = " << this->block_size() << std::endl;
        out << "data_size_    = " << data_size << std::endl;

        int32_t block_starts[Blocks];

        for (int32_t block = 0; block < Blocks; block++)
        {
            block_starts[block] = this->locate(0, block * size);
        }

        for (int32_t block = 0; block < Blocks - 1; block++)
        {
            out << "block_data_size_[" << block << "] = " << block_starts[block + 1] - block_starts[block] << std::endl;
        }

        out << "block_data_size_[" << (Blocks - 1) << "] = " << data_size - block_starts[Blocks - 1] << std::endl;

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

            out<<"Level starts: ";
            for (int32_t c = 0; c <= layout.levels_max; c++) {
                out << layout.level_starts[c] << " ";
            }
            out << std::endl;

            auto value_indexes = this->value_index(0);
            auto size_indexes = this->size_index(0);

            out << "Index:" << std::endl;
            for (int32_t c = 0; c < index_size; c++)
            {
                out << c << ": " << value_indexes[c] << " " << size_indexes[c] << std::endl;
            }
        }

        out << std::endl;

        out<<"Offsets: ";
        for (int32_t c = 0; c <= this->divUpV(data_size); c++) {
            out<<this->offset(0, c)<<" ";
        }
        out << std::endl;

        out << "Values: " << std::endl;

        auto values = this->values();

        Codec codec;

        for (int32_t c = 0; c < size; c++)
        {
            out << "c: " << c << " ";
            for (int32_t block = 0; block < Blocks; block++)
            {
                Value value;
                auto len = codec.decode(values, value, block_starts[block]);
                out<<" ("<<block_starts[block]<<") "<<value<<" ";
                block_starts[block] += len;
            }
            out << std::endl;
        }
    }
};




template <typename Types>
struct PkdStructSizeType<PkdVDTree<Types>> {
    static const PackedSizeType Value = PackedSizeType::VARIABLE;
};


template <typename Types>
struct StructSizeProvider<PkdVDTree<Types>> {
    static const int32_t Value = Types::Blocks;
};

template <typename Types>
struct IndexesSize<PkdVDTree<Types>> {
    static const int32_t Value = Types::Blocks;
};


}}
