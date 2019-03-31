
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

#include <memoria/v1/core/packed/array/packed_vle_array_base.hpp>
#include <memoria/v1/core/packed/tree/vle/packed_vle_tools.hpp>
#include <memoria/v1/core/packed/buffer/packed_vle_input_buffer_ro.hpp>

#include <memoria/v1/core/iovector/io_substream_col_array_vlen.hpp>
#include <memoria/v1/core/iovector/io_substream_col_array_fixed_size_view.hpp>


namespace memoria {
namespace v1 {



template <
    typename ValueT,
    int32_t kBlocks,
    template <typename> class CodecT,
    int32_t kBranchingFactor,// = PkdVLETreeShapeProvider<CodecT<ValueT>>::BranchingFactor,
    int32_t kValuesPerBranch// = PkdVLETreeShapeProvider<CodecT<ValueT>>::ValuesPerBranch
>
struct PkdVLEArrayTypes {
    using Value = ValueT;

    template <typename T>
    using Codec = CodecT<T>;

    static constexpr int32_t Blocks = kBlocks;
    static constexpr int32_t BranchingFactor = kBranchingFactor;
    static constexpr int32_t ValuesPerBranch = kValuesPerBranch;
};



template <typename Types> class PkdVDArray;

template <
    typename ValueT,
    int32_t kBlocks = 1,
    template <typename> class CodecT = ValueCodec,

    int32_t kBranchingFactor = PkdVLETreeShapeProvider<CodecT<ValueT>>::BranchingFactor,
    int32_t kValuesPerBranch = PkdVLETreeShapeProvider<CodecT<ValueT>>::ValuesPerBranch
>
using PkdVDArrayT = PkdVDArray<PkdVLEArrayTypes<ValueT, kBlocks, CodecT, kBranchingFactor, kValuesPerBranch>>;


template <typename Types>
class PkdVDArray: public PkdVLEArrayBase<1, typename Types::Value, Types::template Codec, Types::BranchingFactor, Types::ValuesPerBranch> {

    using Base      = PkdVLEArrayBase<1, typename Types::Value, Types::template Codec, Types::BranchingFactor, Types::ValuesPerBranch>;
    using MyType    = PkdVDArray<Types>;

public:
    using Base::BlocksStart;
    using Base::SegmentsPerBlock;
    using Base::compute_tree_layout;

    using Base::metadata;
    using Base::locate;
    using Base::offsets_segment_size;

    using Base::METADATA;
    using Base::DATA_SIZES;

    using Base::VALUES;
    using Base::OFFSETS;
    using Base::SIZE_INDEX;

    using typename Base::Metadata;
    using typename Base::TreeLayout;
    using typename Base::OffsetsType;
    using typename Base::ValueData;



    using typename Base::Codec;

    static constexpr uint32_t VERSION = 1;
    static constexpr int32_t TreeBlocks = 1;
    static constexpr int32_t Blocks = Types::Blocks;

    static constexpr int32_t SafetyMargin = 128 / Codec::ElementSize;

    using FieldsList = MergeLists<
                typename Base::FieldsList,
                ConstValue<uint32_t, VERSION>,
                ConstValue<uint32_t, TreeBlocks>,
                ConstValue<uint32_t, Blocks>
    >;

    using Value      = typename Types::Value;

    using Values = core::StaticVector<Value, Blocks>;


    using InputBuffer   = PkdVLERowOrderInputBuffer<Types>;
    using InputType     = Values;


    using SizesT = core::StaticVector<int32_t, Blocks>;


    using ReadState = SizesT;

    using GrowableIOSubstream = io::IOColumnwiseVLenArraySubstreamImpl<Value, Blocks>;
    using IOSubstreamView     = io::IOColumnwiseFixedSizeArraySubstreamViewImpl<Value, Blocks>;

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
        return init_bs(empty_size(), total_capacity);
    }

    OpStatus init_bs(int32_t block_size)
    {
        return init_bs(block_size, 0);
    }

    OpStatus init_bs(int32_t block_size, int32_t total_capacity)
    {
        if(isFail(Base::init(block_size, TreeBlocks * SegmentsPerBlock + BlocksStart))) {
            return OpStatus::FAIL;
        }

        Metadata* meta = this->template allocate<Metadata>(METADATA);

        if(isFail(meta)) {
            return OpStatus::FAIL;
        }

        meta->size() = 0;

        for (int32_t block = 0; block < TreeBlocks; block++)
        {
            int32_t capacity        = total_capacity;
            int32_t offsets_size    = offsets_segment_size(capacity);
            int32_t index_size      = this->index_size(capacity);
            int32_t values_segment_length = this->value_segment_size(capacity);

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

    OpStatus init()
    {
        if(isFail(Base::init(empty_size(), TreeBlocks * SegmentsPerBlock + BlocksStart))) {
            return OpStatus::FAIL;
        }

        Metadata* meta = this->template allocate<Metadata>(METADATA);

        meta->size() = 0;
        int32_t offsets_size = offsets_segment_size(0);

        for (int32_t block = 0; block < TreeBlocks; block++)
        {
            if(isFail(this->template allocateArrayBySize<int32_t>(block * SegmentsPerBlock + SIZE_INDEX + BlocksStart, 0))) {
                return OpStatus::FAIL;
            }

            if(isFail(this->template allocateArrayBySize<int8_t>(block * SegmentsPerBlock + OFFSETS + BlocksStart, offsets_size))){
                return OpStatus::FAIL;
            }

            if(isFail(this->template allocateArrayBySize<int8_t>(block * SegmentsPerBlock + VALUES + BlocksStart, 0))) {
                return OpStatus::FAIL;
            }
        }

        return OpStatus::OK;
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

    int32_t size() const
    {
        return this->metadata()->size() / Blocks;
    }

//    SizesT data_size_v() const
//    {
//      SizesT sizes;
//
//      for (int32_t block = 0; block < Blocks; block++)
//      {
//          sizes[block] = this->data_size(block);
//      }
//
//      return sizes;
//    }

    static int32_t elements_for(int32_t block_size)
    {
        return Base::tree_size(TreeBlocks, block_size);
    }

    static int32_t expected_block_size(int32_t items_num)
    {
        return block_size(items_num);
    }

    int32_t locate(int32_t block, int32_t idx) const
    {
        auto values = this->values();
        auto data_size = this->data_size();

        TreeLayout layout = Base::compute_tree_layout(data_size);

        return locate(layout, values, block, idx * Blocks + block, data_size).idx;
    }


    Value value(int32_t block, int32_t idx) const
    {
        int32_t size = this->size();

        MEMORIA_V1_ASSERT(idx, >=, 0);
        MEMORIA_V1_ASSERT(idx, <, size);

        int32_t data_size     = this->data_size();
        auto values       = this->values();
        TreeLayout layout = this->compute_tree_layout(data_size);

        int32_t global_idx    = idx * Blocks + block;
        int32_t start_pos     = this->locate(layout, values, 0, global_idx, data_size).idx;

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

    OpStatus reindex()
    {
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



    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    void max(BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset <= Size, "Invalid balanced tree structure");
    }

    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    void sum(BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset <= Size, "Invalid balanced tree structure");
    }

    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    void sum(int32_t start, int32_t end, BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset <= Size, "Invalid balanced tree structure");
    }

    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    void sub(int32_t start, BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset <= Size, "Invalid balanced tree structure");
    }

    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    void sum(int32_t idx, BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset <= Size, "Invalid balanced tree structure");
    }

    Values get_values(int32_t idx) const
    {
        int32_t size = this->size();

        MEMORIA_V1_ASSERT(idx, >=, 0);
        MEMORIA_V1_ASSERT(idx, <, size);

        int32_t data_size     = this->data_size();
        auto values       = this->values();
        TreeLayout layout = this->compute_tree_layout(data_size);

        int32_t global_idx    = idx * size;
        int32_t start_pos     = this->locate(layout, values, 0, idx, global_idx).idx;

        MEMORIA_V1_ASSERT(start_pos, <, data_size);

        Codec codec;
        Values values_data;

        for (int32_t b = 0; b < Blocks; b++)
        {
            auto len = codec.decode(values, values_data[b], start_pos);
            start_pos += len;
        }

        return values_data;
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

        int32_t data_segment_size    = PackedAllocatable::roundUpBitsToAlignmentBlocks(new_data_size * Codec::ElementSize);
        int32_t index_size           = Base::index_size(new_data_size);
        int32_t offsets_segment_size = Base::offsets_segment_size(new_data_size);

        if (isFail(this->resizeBlock(block * SegmentsPerBlock + VALUES + BlocksStart, data_segment_size))) {
            return OpStatus::FAIL;
        }

        if (isFail(this->resizeBlock(block * SegmentsPerBlock + OFFSETS + BlocksStart, offsets_segment_size))) {
            return OpStatus::FAIL;
        }

        if (isFail(this->resizeBlock(block * SegmentsPerBlock + SIZE_INDEX + BlocksStart, index_size * sizeof(int32_t)))) {
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
            int32_t& data_size      = this->data_size();
            auto values         = this->values();
            TreeLayout layout   = compute_tree_layout(data_size);
            int32_t size            = this->size();

            Codec codec;

            int32_t start_pos[Blocks];
            int32_t lengths[Blocks];

            for (int32_t block = 0; block < Blocks; block++)
            {
                start_pos[block] = this->locate(layout, values, 0, start + size * block, data_size).idx;
                int32_t end_pos      = this->locate(layout, values, 0, end + size * block, data_size).idx;

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

            if (isFail(resize_segments(data_size))) {
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
        return this->_insert(idx, 1, [&](int32_t idx) -> const auto& {
            return values;
        });
    }

    template <typename Adaptor>
    OpStatus insert(int32_t pos, int32_t processed, Adaptor&& adaptor) {
        return _insert(pos, processed, std::forward<Adaptor>(adaptor));
    }


    template <typename Adaptor>
    OpStatus _insert(int32_t idx, int32_t inserted, Adaptor&& adaptor)
    {
        int32_t size      = this->size();
        MEMORIA_V1_ASSERT(idx, >=, 0);
        MEMORIA_V1_ASSERT(idx, <=, size);

        Codec codec;

        auto metadata = this->metadata();
        size_t data_size = metadata->data_size(0);

        TreeLayout layout = compute_tree_layout(data_size);

        auto values          = this->values();
        int32_t global_idx       = idx * Blocks;
        size_t insertion_pos = this->locate(layout, values, 0, global_idx, data_size).idx;

        size_t total_length = 0;

        for (int32_t c = 0; c < inserted; c++)
        {
            for (int32_t block = 0; block < Blocks; block++)
            {
                const auto& value = adaptor(block, c);
                auto len = codec.length(value);

                total_length += len;
            }
        }

        if(isFail(resize_segments(data_size + total_length))) {
            return OpStatus::FAIL;
        }

        values = this->values();

        codec.move(values, insertion_pos, insertion_pos + total_length, data_size - insertion_pos);

        for (int32_t c = 0; c < inserted; c++)
        {
            for (int32_t block = 0; block < Blocks; block++)
            {
                const auto& value = adaptor(block, c);
                int32_t len = codec.encode(values, value, insertion_pos);
                insertion_pos += len;
            }
        }

        metadata->data_size(0) += total_length;

        metadata->size() += (inserted * Blocks);

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
            pos[block] = this->locate(layout, values, 0, idx * Blocks + block, data_size).idx;
        }

        return pos;
    }

    SizesT capacities() const
    {
        return SizesT(0);
    }


    OpStatusT<int32_t> insert_buffer(int32_t at, const InputBuffer* buffer, int32_t start, int32_t size)
    {
        if (size > 0)
        {
            Codec codec;

            auto meta = this->metadata();

            size_t data_size = meta->data_size(0);

            int32_t buffer_start = buffer->locate(0, start);
            int32_t buffer_end = buffer->locate(Blocks - 1, start + size);

            int32_t total_length = buffer_end - buffer_start;

            if(isFail(resize_segments(data_size + total_length))) {
                return OpStatusT<int32_t>();
            }

            auto values         = this->values();
            auto buffer_values  = buffer->values(0);

            size_t insertion_pos = locate(0, at);
            codec.move(values, insertion_pos, insertion_pos + total_length, data_size - insertion_pos);

            codec.copy(buffer_values, buffer_start, values, insertion_pos, total_length);

            meta->data_size(0) += total_length;

            meta->size() += (size * Blocks);

            if(isFail(reindex())) {
                return OpStatusT<int32_t>();
            }
        }

        return OpStatusT<int32_t>(at + size);
    }


    OpStatusT<int32_t> insert_io_substream(int32_t at, io::IOSubstream& substream, int32_t start, int32_t size)
    {
        static_assert(Blocks == 1, "This Packed Array currently does not support multiple columns here");

        io::IOColumnwiseVLenArraySubstream& buffer = io::substream_cast<io::IOColumnwiseVLenArraySubstream>(substream);

        auto buffer_values_start = T2T<const uint8_t*>(buffer.select(0, start));
        auto buffer_values_end   = T2T<const uint8_t*>(buffer.select(0, start + size));

        ptrdiff_t total_length   = buffer_values_end - buffer_values_start;

        auto meta = this->metadata();
        size_t data_size = meta->data_size(0);

        if(isFail(resize_segments(data_size + total_length))) {
            return OpStatusT<int32_t>();
        }

        auto values         = this->values();
        auto buffer_values  = T2T<const ValueData*>(buffer_values_start);

        Codec codec;

        size_t insertion_pos = locate(0, at);
        codec.move(values, insertion_pos, insertion_pos + total_length, data_size - insertion_pos);

        codec.copy(buffer_values, 0, values, insertion_pos, total_length);

        meta->data_size(0) += total_length;

        meta->size() += (size * Blocks);

        if(isFail(reindex())) {
            return OpStatusT<int32_t>();
        }

        return OpStatusT<int32_t>(at + size);
    }

    void configure_io_substream(io::IOSubstream& substream)
    {
        static_assert(Blocks == 1, "This Packed Array currently does not support multiple columns here");
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
    }

    template <int32_t Offset, int32_t Size, typename T2, template <typename, int32_t> class BranchNodeEntryItem, typename Fn>
    OpStatus _insert_b(int32_t idx, BranchNodeEntryItem<T2, Size>& accum, Fn&& fn)
    {
        if(isFail(_insert(idx, 1, [&](int32_t block, int32_t c) -> const auto& {
            return fn(block);
        }))) {
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

    template <int32_t Offset, int32_t Size, typename T2, template <typename, int32_t> class BranchNodeEntryItem, typename Fn>
    OpStatus _update_b(int32_t idx, BranchNodeEntryItem<T2, Size>& accum, Fn&& fn)
    {
        sub<Offset>(idx, accum);

        if(isFail(update_values(idx, [&](int32_t block, auto old_value){
            return fn(block);
        }))) {
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

        if (isFail(remove(idx, idx + 1))) {
            return OpStatus::FAIL;
        }

        return OpStatus::OK;
    }

    template <typename UpdateFn>
    OpStatus update_values(int32_t start, int32_t end, UpdateFn&& update_fn)
    {
        auto values         = this->values();
        int32_t data_size   = this->data_size();
        TreeLayout layout   = compute_tree_layout(data_size);
        int32_t size        = this->size();

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

                    auto new_value = update_fn(block, c, old_value);

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
        }

        return reindex();
    }


    template <typename UpdateFn>
    OpStatus update_values(int32_t start, UpdateFn&& update_fn)
    {
        int32_t size       = this->size();

        MEMORIA_V1_ASSERT(start, >=, 0);
        MEMORIA_V1_ASSERT(start, <, size);

        int32_t global_idx = start * Blocks;

        Codec codec;

        int32_t data_size       = this->data_size();
        auto values         = this->values();
        TreeLayout layout   = compute_tree_layout(data_size);

        size_t insertion_pos = this->locate(layout, values, 0, global_idx, data_size).idx;

        size_t insertion_pos0 = insertion_pos;

        Value old_values[Blocks];
        Value new_values[Blocks];

        size_t new_length = 0;

        for (int32_t block = 0; block < Blocks; block++)
        {
            auto len = codec.decode(values, old_values[block], insertion_pos0);
            new_values[block] = update_fn(block, old_values[block]);
            insertion_pos0 += len;

            new_length += codec.length(new_values[block]);
        }

        size_t old_length = insertion_pos0 - insertion_pos;

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

        for (int32_t block = 0; block < Blocks; block++)
        {
            auto len = codec.encode(values, new_values[block], insertion_pos);
            insertion_pos += len;
        }

        return reindex();
    }


    template <typename UpdateFn>
    OpStatus update_value(int32_t block, int32_t start, UpdateFn&& update_fn)
    {
        int32_t size       = this->size();

        MEMORIA_V1_ASSERT(start, >=, 0);
        MEMORIA_V1_ASSERT(start, <, size);

        int32_t global_idx = start * Blocks + block;

        Codec codec;

        int32_t data_size       = this->data_size();
        auto values         = this->values();
        TreeLayout layout   = compute_tree_layout(data_size);

        size_t insertion_pos = this->locate(layout, values, 0, global_idx).idx;

        Value value;
        size_t old_length = codec.decode(values, value, insertion_pos);
        auto new_value    = update_fn(block, value);

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


    OpStatus addValue(int32_t block, int32_t idx, Value value)
    {
        return update_value(block, idx, [&](int32_t block, auto old_value){return value + old_value;});
    }

    template <typename T, int32_t Indexes>
    OpStatus addValues(int32_t idx, int32_t from, int32_t size, const core::StaticVector<T, Indexes>& values)
    {
        return update_values(idx, [&](int32_t block, auto old_value){return values[block + from] + old_value;});
    }

    void check() const {}

    OpStatus clear()
    {
        if (this->allocatable().has_allocator())
        {
            auto alloc = this->allocatable().allocator();
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
        handler->startGroup("VLD_ARRAY");

        auto meta = this->metadata();

        handler->value("SIZE",      &meta->size());
        handler->value("DATA_SIZE", &meta->data_size(0), TreeBlocks);

        handler->startGroup("INDEXES", TreeBlocks);

        for (int32_t block = 0; block < TreeBlocks; block++)
        {
            int32_t index_size = this->index_size(Base::data_size(block));

            handler->startGroup("BLOCK_INDEX", block);

            auto size_indexes  = this->size_index(block);

            for (int32_t c = 0; c < index_size; c++)
            {
                handler->value("INDEX", BlockValueProviderFactory::provider(size_indexes[c]));
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

            handler->value("ARRAY_ITEM", BlockValueProviderFactory::provider(Blocks, [&](int32_t idx) -> const Value& {
                return values_data[idx];
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

        auto meta = this->metadata();

        FieldFactory<int32_t>::serialize(buf, meta->size());

        FieldFactory<int32_t>::serialize(buf, meta->data_size(0), TreeBlocks);

        for (int32_t block = 0; block < TreeBlocks; block++)
        {
            Base::template serializeSegment<int32_t>(buf, block * SegmentsPerBlock + BlocksStart + SIZE_INDEX);
            Base::template serializeSegment<OffsetsType>(buf, block * SegmentsPerBlock + BlocksStart + OFFSETS);
            FieldFactory<ValueData>::serialize(buf, Base::values(block), Base::data_size(block));
        }
    }

    template <typename DeserializationData>
    void deserialize(DeserializationData& buf)
    {
        Base::deserialize(buf);

        auto meta = this->metadata();

        FieldFactory<int32_t>::deserialize(buf, meta->size());

        FieldFactory<int32_t>::deserialize(buf, meta->data_size(0), TreeBlocks);

        for (int32_t block = 0; block < TreeBlocks; block++)
        {
            Base::template deserializeSegment<int32_t>(buf, block * SegmentsPerBlock + BlocksStart + SIZE_INDEX);
            Base::template deserializeSegment<OffsetsType>(buf, block * SegmentsPerBlock + BlocksStart + OFFSETS);
            FieldFactory<ValueData>::deserialize(buf, Base::values(block), Base::data_size(block));
        }
    }


    template <typename IOBuffer>
    bool readTo(ReadState& state, IOBuffer& buffer) const
    {
        Codec codec;
        auto values = this->values();

        for (int32_t b = 0; b < Blocks; b++)
        {
            Value value;
            auto val = codec.describe(values, state[b]);

            if (buffer.put(val))
            {
                state[0] += val.length();
            }
            else {
                return false;
            }
        }

        return true;
    }



    template <typename Fn>
    void read(int32_t start, int32_t end, Fn&& fn) const
    {
        int32_t size = this->size();

        MEMORIA_V1_ASSERT(start, >=, 0);
        MEMORIA_V1_ASSERT(start, <=, end);
        MEMORIA_V1_ASSERT(end, <=, size);

        size_t positions[Blocks];

        for (int32_t b = 0; b < Blocks; b++)
        {
            int32_t global_idx = b * size + start;
            positions[b] = this->locate(0, global_idx);
        }

        Codec codec;

        auto values = this->values();

        for (int32_t c = start; c < end; c++)
        {
            for (int32_t b = 0; b < Blocks; b++)
            {
                Value value;
                auto len = codec.decode(values, value, positions[b]);

                fn(b, value);

                positions[b] += len;
            }

            fn.next();
        }
    }



    template <typename ConsumerFn>
    void read(int32_t block, int32_t start, int32_t end, ConsumerFn&& fn) const
    {
        int32_t size = this->size();

        MEMORIA_V1_ASSERT(start, >=, 0);
        MEMORIA_V1_ASSERT(start, <=, end);
        MEMORIA_V1_ASSERT(end, <=, size);

        int32_t global_idx = block * size + start;

        size_t pos = this->locate(0, global_idx);
        size_t data_size = this->data_size();

        Codec codec;

        auto values = this->values();

        for (int32_t c = start; c < end && pos < data_size; c++)
        {
            Value value;
            auto len = codec.decode(values, value, pos);

            fn(block, value);
            fn.next();

            pos += len;
        }
    }


    template <typename T>
    void read(int32_t block, int32_t start, int32_t end, T* values) const
    {
        read(block, start, end, [&](int32_t c, const auto& value){
            values[c - start] = value;
        });
    }


    void dump(std::ostream& out = std::cout) const
    {
        int32_t size = this->size();

        auto data_size  = this->data_size();

        out << "size_         = " << size << std::endl;
        out << "block_size_   = " << this->block_size() << std::endl;
        out << "data_size_    = " << data_size << std::endl;

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

        auto values = this->values();

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
};




template <typename Types>
struct PkdStructSizeType<PkdVDArray<Types>> {
    static const PackedSizeType Value = PackedSizeType::VARIABLE;
};


template <typename Types>
struct StructSizeProvider<PkdVDArray<Types>> {
    static const int32_t Value = 0;
};

template <typename Types>
struct IndexesSize<PkdVDArray<Types>> {
    static const int32_t Value = 0;
};

template <typename T>
struct PkdSearchKeyTypeProvider<PkdVDArray<T>> {
    using Type = typename PkdVDArray<T>::Value;
};



}}
