
// Copyright 2016 Victor Smirnov
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

#include <memoria/v1/core/strings/string_codec.hpp>
#include <memoria/v1/core/tools/optional.hpp>

#include <memoria/v1/core/iovector/io_substream_col_array_vlen.hpp>
#include <memoria/v1/core/iovector/io_substream_col_array_vlen_view.hpp>

#ifdef HAVE_BOOST
#include <memoria/v1/core/bignum/cppint_codec.hpp>
#endif

#include <memoria/v1/core/bignum/int64_codec.hpp>
#include <memory>

namespace memoria {
namespace v1 {


class TextBlockDumper;



template <typename> class ValueCodec;

template <
    typename ValueT,
    template <typename> class CodecT,
    int32_t kBranchingFactor
>
struct PkdVBMTreeTypes {
    using Value = ValueT;
    using IndexValue = ValueT;

    template <typename T>
    using Codec = CodecT<T>;
    static constexpr int32_t ValueBranchingFactor = kBranchingFactor;

    static constexpr int32_t Blocks = 1;

    static constexpr int32_t BranchingFactor = 32;
    static constexpr int32_t ValuesPerBranch = kBranchingFactor;
};

template <typename Types> class PkdVBMTree;

template <
    typename ValueT,
    template <typename> class CodecT = ValueCodec,
    int32_t kBranchingFactor = 1024
>
using PkdVBMTreeT = PkdVBMTree<PkdVBMTreeTypes<ValueT, CodecT, kBranchingFactor>>;


template <typename Types>
class PkdVBMTree: public PackedAllocator {

    using Base      = PackedAllocator;
    using MyType    = PkdVBMTree<Types>;

public:
    static constexpr uint32_t VERSION   = 1;


    static constexpr int32_t Blocks     = 1;

    static constexpr PkdSearchType KeySearchType = PkdSearchType::MAX;

    static const int32_t BranchingFactorI           = PackedTreeBranchingFactor;
    static const int32_t BranchingFactorV           = Types::ValueBranchingFactor;

    static constexpr int32_t BranchingFactorVMask   = BranchingFactorV - 1;
    static constexpr int32_t BranchingFactorVLog2   = Log2(BranchingFactorV) - 1;

    static constexpr int32_t BranchingFactorIMask   = BranchingFactorI - 1;
    static constexpr int32_t BranchingFactorILog2   = Log2(BranchingFactorI) - 1;

    enum {METADATA, INDEX, SIZE_INDEX, OFFSETS, VALUES, TOTAL_BLOCKS};


    class Metadata {
        int32_t size_;
        int32_t data_size_;
    public:
        int32_t& size(){return size_;}
        const int32_t& size() const {return size_;}

        int32_t& data_size() {return data_size_;}
        const int32_t& data_size() const {return data_size_;}
    };

    using FieldsList = MergeLists<>;

    using OffsetsType   = uint16_t;
    using SizesValue    = int32_t;
    using Value         = typename Types::Value;
    using IndexValue    = typename Types::Value;

    using Values        = core::StaticVector<Value, Blocks>;
    using Codec         = typename Types::template Codec<Value>;

    using ValueData     = typename Codec::BufferType;
    using InputBuffer   = PkdVLEColumnOrderInputBuffer<Types>;
    using InputType     = Values;

    using SizesT        = core::StaticVector<int32_t, Blocks>;
    using PtrsT         = core::StaticVector<ValueData*, Blocks>;
    using ConstPtrsT    = core::StaticVector<const ValueData*, Blocks>;

    using GrowableIOSubstream = io::IOColumnwiseVLenArraySubstreamImpl<Value, Blocks>;
    using IOSubstreamView     = io::IOColumnwiseVLenArraySubstreamViewImpl<Value, Blocks>;

    class ReadState {
        ConstPtrsT values_;
        SizesT data_pos_;
    public:

        int32_t& data_pos(int32_t idx) {return data_pos_[idx];}
        const int32_t& data_pos(int32_t idx) const {return data_pos_[idx];}

        ConstPtrsT& values() {return values_;}
        SizesT& data_pos() {return data_pos_;}

        const ConstPtrsT& values() const {return values_;}
        const SizesT& data_pos() const {return data_pos_;}
    };

    struct TreeLayout {
        int32_t level_starts[8];
        int32_t level_sizes[8];
        int32_t levels_max = 0;
        int32_t index_size = 0;

        const int32_t* valaue_block_size_prefix;
    };

    struct LocateResult {
        int32_t idx = 0;
        int32_t index_cnt = 0;

        LocateResult(int32_t idx_, int32_t index_cnt_ = 0) :
            idx(idx_), index_cnt(index_cnt_)
        {}

        LocateResult() {}

        int32_t local_cnt() const {return idx - index_cnt;}
    };

    OpStatus init_bs(int32_t block_size) {
        return init();
    }

    OpStatus init()
    {
        if(isFail(Base::init(empty_size(), TOTAL_BLOCKS))) {
            return OpStatus::FAIL;
        }

        Metadata* meta = this->template allocate<Metadata>(METADATA);
        if(isFail(meta)) {
            return OpStatus::FAIL;
        }

        meta->size() = 0;
        meta->data_size() = 0;

        if(isFail(this->template allocateArrayBySize<int32_t>(SIZE_INDEX, 0))) {
            return OpStatus::FAIL;
        }

        if(isFail(this->template allocateArrayBySize<OffsetsType>(OFFSETS, number_of_offsets(0)))) {
            return OpStatus::FAIL;
        }

        if(isFail(this->template allocateArrayBySize<ValueData>(VALUES, 0))) {
            return OpStatus::FAIL;
        }

        return OpStatus::OK;
    }

    int32_t block_size() const
    {
        return Base::block_size();
    }



    Metadata* metadata() {
        return this->template get<Metadata>(METADATA);
    }

    const Metadata* metadata() const {
        return this->template get<Metadata>(METADATA);
    }


    const int32_t& size() const {
        return metadata()->size();
    }

    int32_t& size() {
        return metadata()->size();
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



    Value value(int32_t block, int32_t idx) const
    {
        return value(idx);
    }



    Value value(int32_t idx) const
    {
        MEMORIA_V1_ASSERT(idx, >=, 0);
        MEMORIA_V1_ASSERT(idx, <, this->size());

        auto meta         = this->metadata();

        int32_t data_size     = meta->data_size();
        auto values       = this->values();
        TreeLayout layout = compute_tree_layout(data_size);

        int32_t start_pos     = locate(layout, values, idx).idx;

        MEMORIA_V1_ASSERT(start_pos, <, data_size);

        Codec codec;
        Value value;
        codec.decode(values, value, start_pos);

        return value;
    }


//    int64_t setValue(int32_t block, int32_t idx, Value value)
//    {
//      if (value != 0)
//      {
//          Value val = this->value(block, idx);
//          this->value(block, idx) = value;
//
//          return val - value;
//      }
//      else {
//          return 0;
//      }
//    }
//


    template <typename T>
    OpStatus setValues(int32_t idx, const core::StaticVector<T, Blocks>& values)
    {
        return update_values(idx, [&](int32_t block, const auto& old_value) -> const auto& {return values[block];});
    }



    static int32_t empty_size()
    {
        int32_t metadata_length = PackedAllocatable::roundUpBytesToAlignmentBlocks(sizeof(Metadata));

        int32_t sizes_length    = 0;

        int32_t values_length   = 0;
        int32_t offsets_length  = offsets_segment_size(0);

        int32_t segments_length = values_length + offsets_length + sizes_length;


        return PackedAllocator::block_size(
                metadata_length +
                segments_length,
                TOTAL_BLOCKS
        );
    }






//    bool check_capacity(int32_t size) const
//    {
//      MEMORIA_V1_ASSERT_TRUE(size >= 0);
//
//      auto alloc = this->allocator();
//
//      int32_t total_size          = this->size() + size;
//      int32_t total_block_size    = MyType::block_size(total_size);
//      int32_t my_block_size       = alloc->element_size(this);
//      int32_t delta               = total_block_size - my_block_size;
//
//      return alloc->free_space() >= delta;
//    }


    // ================================ Container API =========================================== //


    Value max(int32_t block) const
    {
        auto size = this->size();

        if (size > 0) {
            return this->value(block, size - 1);
        }
        else {
            return Value();
        }
    }

    template <typename T>
    OpStatus addValues(int32_t idx, const core::StaticVector<T, Blocks>& values)
    {
        for (int32_t b = 0; b < Blocks; b++) {
            this->values(b)[idx] = values[b];
        }

        return reindex();
    }

    void sums(Values& values) const
    {
        for (int32_t c = 0; c < Blocks; c++) {
            values[c] = this->max(c);
        }
    }

    void sums(int32_t start, int32_t end, Values& values) const
    {
        if (end - 1 > start)
        {
            for (int32_t c = 0; c < Blocks; c++)
            {
                values[c] = this->values(c)[end - 1];
            }
        }
    }

    template <typename T>
    void max(core::StaticVector<T, Blocks>& accum) const
    {
        for (int32_t block = 0; block < Blocks; block++)
        {
            accum[block] = this->max(block);
        }
    }

    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    void max(BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset <= Size - Blocks, "Invalid balanced tree structure");

        for (int32_t block = 0; block < Blocks; block++)
        {
            accum[block + Offset] = this->max(block);
        }
    }


    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    void sum(BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset <= Size - Blocks, "Invalid balanced tree structure");

        for (int32_t block = 0; block < Blocks; block++)
        {
            accum[block + Offset] = this->max(block);
        }
    }

    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    void sum(int32_t start, int32_t end, BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset <= Size - Blocks, "Invalid balanced tree structure");

        for (int32_t block = 0; block < Blocks; block++)
        {
            accum[block + Offset] = this->value(block, end);
        }
    }

    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    void sum(int32_t idx, BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset <= Size - Blocks, "Invalid balanced tree structure");

        for (int32_t block = 0; block < Blocks; block++)
        {
            accum[block + Offset] = this->value(block, idx);
        }
    }

    template <int32_t Offset, int32_t From, int32_t To, typename T, template <typename, int32_t, int32_t> class BranchNodeEntryItem>
    void sum(int32_t start, int32_t end, BranchNodeEntryItem<T, From, To>& accum) const
    {
        for (int32_t block = 0; block < Blocks; block++)
        {
            accum[block + Offset] = this->value(block, end);
        }
    }

//    template <typename T>
//    void update(int32_t idx, const core::StaticVector<T, Blocks>& values)
//    {
//        setValues(idx, values);
//    }
//
//
//
//    template <int32_t Offset, int32_t Size, typename T1, typename T2, template <typename, int32_t> class BranchNodeEntryItem>
//    void _insert(int32_t idx, const core::StaticVector<T1, Blocks>& values, BranchNodeEntryItem<T2, Size>& accum)
//    {
//      insert(idx, values);
//
//      sum<Offset>(idx, accum);
//    }
//
//    template <int32_t Offset, int32_t Size, typename T1, typename T2, template <typename, int32_t> class BranchNodeEntryItem>
//    void _update(int32_t idx, const core::StaticVector<T1, Blocks>& values, BranchNodeEntryItem<T2, Size>& accum)
//    {
//      sub<Offset>(idx, accum);
//
//      update(idx, values);
//
//      sum<Offset>(idx, accum);
//    }
//
//
//    template <int32_t Offset, int32_t Size, typename T1, typename T2, typename I, template <typename, int32_t> class BranchNodeEntryItem>
//    void _update(int32_t idx, const std::pair<T1, I>& values, BranchNodeEntryItem<T2, Size>& accum)
//    {
//      sub<Offset>(idx, accum);
//
//      this->setValue(values.first, idx, values.second);
//
//      sum<Offset>(idx, accum);
//    }
//
//    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
//    void _remove(int32_t idx, BranchNodeEntryItem<T, Size>& accum)
//    {
//      sub<Offset>(idx, accum);
//      remove(idx, idx + 1);
//    }

    template <int32_t Offset, int32_t Size, typename T1, typename T2, template <typename, int32_t> class BranchNodeEntryItem>
    OpStatus _insert(int32_t idx, const core::StaticVector<T1, Blocks>& values, BranchNodeEntryItem<T2, Size>& accum)
    {
        if (isFail(insert(idx, values))) {
            return OpStatus::FAIL;
        }

        sum<Offset>(this->size() - 1, accum);

        return OpStatus::OK;
    }

    template <int32_t Offset, int32_t Size, typename T2, template <typename, int32_t> class BranchNodeEntryItem, typename AccessorFn>
    OpStatus _insert_b(int32_t idx, BranchNodeEntryItem<T2, Size>& accum, AccessorFn&& values)
    {
        if(isFail(this->_insert(idx, 1, [&](int32_t block, int32_t idx) -> const auto& {
            return values(block);
        }))) {
            return OpStatus::FAIL;
        }

        sum<Offset>(this->size() - 1, accum);

        return OpStatus::OK;
    }


//    template <int32_t Offset, typename T1>
//    void _insert(int32_t idx, const core::StaticVector<T1, Blocks>& values)
//    {
//      insert(idx, values);
//    }

    template <int32_t Offset, int32_t Size, typename T1, typename T2, template <typename, int32_t> class BranchNodeEntryItem>
    OpStatus _update(int32_t idx, const core::StaticVector<T1, Blocks>& values, BranchNodeEntryItem<T2, Size>& accum)
    {
        if(isFail(update(idx, values))) {
            return OpStatus::FAIL;
        }

        sum<Offset>(this->size() - 1, accum);

        return OpStatus::OK;
    }

    template <int32_t Offset, int32_t Size, typename T2, template <typename, int32_t> class BranchNodeEntryItem, typename AccessorFn>
    OpStatus _update_b(int32_t idx, BranchNodeEntryItem<T2, Size>& accum, AccessorFn&& values)
    {
        if(isFail(update(idx, idx + 1, [&](int32_t block, int32_t idx){
            return values(block);
        }))) {
            return OpStatus::FAIL;
        }

        sum<Offset>(this->size() - 1, accum);

        return OpStatus::OK;
    }


    template <int32_t Offset, int32_t Size, typename T1, typename T2, typename I, template <typename, int32_t> class BranchNodeEntryItem>
    OpStatus _update(int32_t idx, const std::pair<T1, I>& values, BranchNodeEntryItem<T2, Size>& accum)
    {
        if(isFail(this->setValue(values.first, idx, values.second))) {
            return OpStatus::FAIL;
        }

        sum<Offset>(this->size() - 1, accum);

        return OpStatus::OK;
    }

    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    OpStatus _remove(int32_t idx, BranchNodeEntryItem<T, Size>& accum)
    {
        if (isFail(remove(idx, idx + 1))) {
            return OpStatus::FAIL;
        }

        auto size = this->size();
        if (size > 0)
        {
            sum<Offset>(size - 1, accum);
        }

        return OpStatus::OK;
    }



    // ========================================= Insert/Remove/Resize ============================================== //





public:
    OpStatus splitTo(MyType* other, int32_t idx)
    {
        auto meta = this->metadata();

        TreeLayout layout   = compute_tree_layout(meta->data_size());
        auto values         = this->values();

        Codec codec;

        int32_t start       = locate(layout, values, idx).idx;
        int32_t data_size   = meta->data_size() - start;

        if (isFail(other->insert_space(0, data_size))) {
            return OpStatus::FAIL;
        }

        codec.copy(values, start, other->values(), 0, data_size);

        int32_t size        = meta->size();
        other->size()   += size - idx;

        if (isFail(other->reindex())) {
            return OpStatus::FAIL;
        }

        return remove(idx, size);
    }


    OpStatus mergeWith(MyType* other)
    {
        auto meta       = this->metadata();
        auto other_meta = other->metadata();

        int32_t data_size       = meta->data_size();
        int32_t other_data_size = other_meta->data_size();
        int32_t start           = other_data_size;

        if (isFail(other->insert_space(other_data_size, data_size))) {
            return OpStatus::FAIL;
        }

        Codec codec;
        codec.copy(this->values(), 0, other->values(), start, data_size);

        other->size() += meta->size();

        if (isFail(other->reindex())) {
            return OpStatus::FAIL;
        }

        return clear();
    }


    OpStatus removeSpace(int32_t start, int32_t end) {
        return remove(start, end);
    }

    OpStatus remove(int32_t start, int32_t end)
    {
        auto meta           = this->metadata();

        auto values         = this->values();
        TreeLayout layout   = compute_tree_layout(meta->data_size());

        int32_t start_pos = locate(layout, values, start).idx;
        int32_t end_pos   = locate(layout, values, end).idx;

        if (isFail(this->remove_space(start_pos, end_pos - start_pos))) {
            return OpStatus::FAIL;
        }

        meta->size() -= end - start;

        if (isFail(reindex())){
            return OpStatus::FAIL;
        }

        return OpStatus::OK;
    }




    template <typename T>
    OpStatus insert(int32_t idx, const core::StaticVector<T, 1>& values)
    {
        return this->_insert(idx, 1, [&](int32_t block, int32_t idx) -> const T& {
            return values[block];
        });
    }



    template <typename Adaptor>
    OpStatus insert(int32_t pos, int32_t processed, Adaptor&& adaptor) {
        return _insert(pos, processed, std::forward<Adaptor>(adaptor));
    }


    template <typename Adaptor>
    OpStatus _insert(int32_t pos, int32_t processed, Adaptor&& adaptor)
    {
        auto meta = this->metadata();

        int32_t size = meta->size();

        MEMORIA_V1_ASSERT(pos, >=, 0);
        MEMORIA_V1_ASSERT(pos, <=, size);
        MEMORIA_V1_ASSERT(processed, >=, 0);

        Codec codec;

        SizeT total_lengths = 0;

        for (SizeT c = 0; c < processed; c++)
        {
            const auto& value  = adaptor(0, c);
            auto len    = codec.length(value);
            total_lengths += len;
        }

        int32_t data_size   = meta->data_size();
        auto values         = this->values();
        TreeLayout layout   = compute_tree_layout(data_size);

        auto lr = locate(layout, values, pos);

        size_t insertion_pos = lr.idx;

        if(isFail(insert_space(insertion_pos, total_lengths))) {
            return OpStatus::FAIL;
        }

        values = this->values();

        for (int32_t c = 0; c < processed; c++)
        {
            const auto& value = adaptor(0, c);
            auto len = codec.encode(values, value, insertion_pos);
            insertion_pos += len;
        }

        meta->size() += processed;

        return reindex();
    }


    ReadState positions(int32_t idx) const
    {
        auto meta = this->metadata();

        MEMORIA_V1_ASSERT(idx, >=, 0);
        MEMORIA_V1_ASSERT(idx, <=, meta->size());

        int32_t data_size       = meta->data_size();
        auto values         = this->values();
        TreeLayout layout   = compute_tree_layout(data_size);

        ReadState read_state;

        read_state.values()[0] = values;
        read_state.data_pos()[0] = locate(layout, values, idx).idx;

        return read_state;
    }


    OpStatusT<SizesT> insert_buffer(SizesT at, const InputBuffer* buffer, SizesT starts, SizesT ends, int32_t size)
    {
        auto meta = this->metadata();

        Codec codec;

        SizesT total_lengths = ends - starts;

        auto values = this->values();

        size_t insertion_pos = at[0];

        insert_space(insertion_pos, total_lengths[0]);

        values = this->values();

        codec.copy(buffer->values(), starts[0], values, insertion_pos, total_lengths[0]);

        meta->size() += size;

        if(isFail(reindex())) {
            return OpStatus::FAIL;
        }

        return OpStatusT<SizesT>(at + total_lengths);
    }

    OpStatus insert_buffer(int32_t pos, const InputBuffer* buffer, int32_t start, int32_t size)
    {
        Codec codec;

        SizesT starts   = buffer->positions(start);
        SizesT ends     = buffer->positions(start + size);

        auto at         = this->positions(pos);

        SizesT total_lengths = ends - starts;

        auto values = this->values();

        size_t insertion_pos = at.data_pos(0);

        if(isFail(insert_space(insertion_pos, total_lengths[0]))) {
            return OpStatus::FAIL;
        }

        values = this->values();

        codec.copy(buffer->values(0), starts[0], values, insertion_pos, total_lengths[0]);

        this->size() += size;

        return reindex();
    }


    OpStatus insert_io_substream(int32_t pos, io::IOSubstream& substream, int32_t start, int32_t size)
    {
        static_assert(Blocks == 1, "This Packed Array currently does not support multiple columns here");

        io::IOColumnwiseVLenArraySubstream& buffer = io::substream_cast<io::IOColumnwiseVLenArraySubstream>(substream);

        auto buffer_values_start = T2T<const uint8_t*>(buffer.select(0, start));
        auto buffer_values_end   = T2T<const uint8_t*>(buffer.select(0, start + size));

        ptrdiff_t total_length   = buffer_values_end - buffer_values_start;

        Codec codec;

        auto at = this->positions(pos);

        auto values = this->values();

        size_t insertion_pos = at.data_pos(0);

        if(isFail(insert_space(insertion_pos, total_length))) {
            return OpStatus::FAIL;
        }

        values = this->values();

        codec.copy(buffer_values_start, 0, values, insertion_pos, total_length);

        this->size() += size;

        return reindex();
    }

    void configure_io_substream(io::IOSubstream& substream)
    {
    }



    template <typename Adaptor>
    OpStatusT<SizesT> populate(const SizesT& at, int32_t size, Adaptor&& adaptor)
    {
        auto meta = this->metadata();

        Codec codec;

        SizesT total_lengths;

        for (int32_t c = 0; c < size; c++)
        {
            total_lengths[0] += codec.length(adaptor(0, c));
        }

        size_t insertion_pos = at[0];

        auto values = this->values();

        for (int32_t c = 0; c < size; c++)
        {
            auto value = adaptor(0, c);
            auto len = codec.encode(values, value, insertion_pos);
            insertion_pos += len;
        }

        meta->data_size() += total_lengths[0];

        meta->size() += size;

        return OpStatusT<SizesT>(at + total_lengths);
    }



    template <typename UpdateFn>
    OpStatus update_values1(int32_t start, int32_t end, UpdateFn&& update_fn)
    {
        auto meta = this->metadata();

        Codec codec;

        auto values         = this->values();
        int32_t data_size       = meta->data_size();
        TreeLayout layout   = compute_tree_layout(data_size);
        size_t data_start   = locate(layout, values, start);

        for (int32_t window_start = start; window_start < end; window_start += 32)
        {
            int32_t window_end = (window_start + 32) < end ? window_start + 32 : end;

            int32_t old_length = 0;
            int32_t new_length = 0;

            auto values = this->values();

            size_t data_start_tmp = data_start;

            Value buffer[32];

            for (int32_t c = window_start; c < window_end; c++)
            {
                Value old_value;
                auto len = codec.decode(values, old_value, data_start_tmp, data_size);

                const auto& new_value = update_fn(0, c, old_value);

                buffer[c - window_start] = new_value;

                old_length += len;
                new_length += codec.length(new_value);

                data_start_tmp += len;
            }

            if (new_length > old_length)
            {
                auto delta = new_length - old_length;
                if (isFail(insert_space(data_start, delta))) {
                    return OpStatus::FAIL;
                }

                values = this->values();
            }
            else if (new_length < old_length)
            {
                auto delta = old_length - new_length;
                if (isFail(remove_space(data_start, delta))) {
                    return OpStatus::FAIL;
                }

                values = this->values();
            }

            for (int32_t c = window_start; c < window_end; c++)
            {
                data_start += codec.encode(values, buffer[c], data_start);
            }
        }

        return reindex();
    }


    template <typename UpdateFn>
    OpStatus update_values(int32_t start, UpdateFn&& update_fn)
    {
        return update_value(start, std::forward<UpdateFn>(update_fn));
    }


    template <typename UpdateFn>
    OpStatus update_value(int32_t start, UpdateFn&& update_fn)
    {
        auto meta = this->metadata();

        MEMORIA_V1_ASSERT(start, <=, meta->size());
        MEMORIA_V1_ASSERT(start, >=, 0);

        Codec codec;

        int32_t data_size       = meta->data_size();

        auto values             = this->values();
        TreeLayout layout       = compute_tree_layout(data_size);
        size_t insertion_pos    = locate(layout, values, start).idx;

        Value value;
        size_t old_length = codec.decode(values, value, insertion_pos, data_size);
        const auto& new_value = update_fn(0, value);

        if (new_value != value)
        {
            size_t new_length = codec.length(new_value);

            if (new_length > old_length)
            {
                if (isFail(insert_space(insertion_pos, new_length - old_length))) {
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





    OpStatus clear()
    {
        if (allocatable().has_allocator())
        {
            auto alloc = this->allocatable().allocator();
            int32_t empty_size = MyType::empty_size();
            if(isFail(alloc->resizeBlock(this, empty_size))) {
                return OpStatus::FAIL;
            }
        }

        return init();
    }

    void dump(std::ostream& out = std::cout) const
    {
        std::unique_ptr<TextBlockDumper> dumper = std::make_unique<TextBlockDumper>(out);
        this->generateDataEvents(dumper.get());
    }


    void generateDataEvents(IBlockDataEventHandler* handler) const
    {
        Base::generateDataEvents(handler);

        handler->startStruct();
        handler->startGroup("VBM_TREE");

        if (has_index())
        {
            handler->startGroup("INDEX");
            index()->generateDataEvents(handler);
            handler->endGroup();
        }

        auto meta = this->metadata();

        handler->value("SIZE",      &meta->size());
        handler->value("DATA_SIZE", &meta->data_size());

        TreeLayout layout = compute_tree_layout(meta->data_size());

        if (layout.levels_max >= 0)
        {
            handler->startGroup("TREE_LAYOUT", layout.index_size);

            for (int32_t c = 0; c < layout.index_size; c++)
            {
                handler->value("LAYOUT_ITEM", &this->size_index()[c]);
            }

            handler->endGroup();
        }


        auto offsets_num = number_of_offsets(meta->data_size());
        handler->startGroup("OFFSETS", offsets_num);

        auto offsets = this->offsets();

        handler->value("OFFSETS", offsets, offsets_num, IBlockDataEventHandler::BYTE_ARRAY);

        handler->endGroup();



        handler->startGroup("DATA", meta->size());

        const ValueData* values = this->values();

        size_t position = 0;

        int32_t size = meta->size();

        Codec codec;

        for (int32_t idx = 0; idx < size; idx++)
        {
            Value value;
            auto len = codec.decode(values, value, position);
            position += len;

            handler->value("TREE_ITEM", BlockValueProviderFactory::provider(value));
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
        FieldFactory<int32_t>::serialize(buf, meta->data_size());

        if (has_index())
        {
            index()->serialize(buf);
        }

        Base::template serializeSegment<SizesValue>(buf, SIZE_INDEX);
        Base::template serializeSegment<OffsetsType>(buf, OFFSETS);

        int32_t data_block_size = this->data_block_size();

        FieldFactory<ValueData>::serialize(buf, this->values(), data_block_size);
    }


    template <typename DeserializationData>
    void deserialize(DeserializationData& buf)
    {
        Base::deserialize(buf);

        auto meta = this->metadata();

        FieldFactory<int32_t>::deserialize(buf, meta->size());
        FieldFactory<int32_t>::deserialize(buf, meta->data_size());

        if (has_index())
        {
            index()->deserialize(buf);
        }

        Base::template deserializeSegment<int32_t>(buf, SIZE_INDEX);
        Base::template deserializeSegment<OffsetsType>(buf, OFFSETS);

        int32_t data_block_size = this->data_block_size();

        FieldFactory<ValueData>::deserialize(buf, this->values(), data_block_size);
    }


    template <typename Walker>
    auto find(Walker&& walker) const
    {
        auto meta = this->metadata();
        auto values = this->values();

        size_t data_size = meta->data_size();
        int32_t size = meta->size();

        Codec codec;

        if (!this->has_index())
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
            int32_t index_size  = this->index()->size();
            auto idx = this->index()->find(walker).local_pos();
            if (idx < index_size)
            {
                size_t local_pos = (idx << BranchingFactorVLog2) + offset(idx);

                TreeLayout layout = compute_tree_layout(data_size);
                layout.valaue_block_size_prefix = this->size_index();

                int32_t prefix = this->sum_index(layout, idx);

                for (int32_t local_idx = prefix; local_pos < data_size; local_idx++)
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


    auto find_ge(const Value& value) const
    {
        return find(FindGEWalker(value));
    }

    auto find_gt(const Value& value) const
    {
        return find(FindGTWalker(value));
    }




    auto findGTForward(const Value& val) const
    {
        return this->find_gt(val);
    }

    auto findGEForward(const Value& val) const
    {
        return this->find_ge(val);
    }

    auto findGTForward(int32_t block, const Value& val) const
    {
        return this->find_gt(val);
    }

    auto findGEForward(int32_t block, const Value& val) const
    {
        return this->find_ge(val);
    }



    class FindResult {
        int32_t idx_;
    public:
        template <typename Fn>
        FindResult(Fn&& fn): idx_(fn.local_pos()) {}
        int32_t local_pos() const {return idx_;}

        void set_idx(int32_t idx)
        {
            this->idx_ = idx;
        }
    };

    auto findForward(SearchType search_type, int32_t block, int32_t start, const Value& val) const
    {
        if (search_type == SearchType::GT)
        {
            return FindResult(findGTForward(block, start, val));
        }
        else {
            return FindResult(findGEForward(block, start, val));
        }
    }

    auto findForward(SearchType search_type, int32_t block, int32_t start, const OptionalT<Value>& val) const
    {
        if (search_type == SearchType::GT)
        {
            return FindResult(findGTForward(block, start, val.value()));
        }
        else {
            return FindResult(findGEForward(block, start, val.value()));
        }
    }

    auto findForward(SearchType search_type, int32_t block, const Value& val) const
    {
        if (search_type == SearchType::GT)
        {
            return FindResult(findGTForward(block, val));
        }
        else {
            return FindResult(findGEForward(block, val));
        }
    }

    auto findForward(SearchType search_type, int32_t block, const OptionalT<Value>& val) const
    {
    	if (search_type == SearchType::GT)
    	{
    		return FindResult(findGTForward(block, val.value()));
    	}
    	else {
    		return FindResult(findGEForward(block, val.value()));
    	}
    }


    template <typename IOBuffer>
    bool readTo(ReadState& state, IOBuffer& buffer) const
    {
        Codec codec;

        auto val = codec.describe(state.values()[0], state.data_pos()[0]);

        if (buffer.put(val))
        {
            state.data_pos()[0] += val.length();
            return true;
        }
        else {
            return false;
        }
    }



    template <typename ConsumerFn>
    void read(int32_t block, int32_t start, int32_t end, ConsumerFn&& fn) const
    {
        auto meta = this->metadata();

        MEMORIA_V1_ASSERT(start, >=, 0);
        MEMORIA_V1_ASSERT(start, <=, end);
        MEMORIA_V1_ASSERT(end, <=, meta->size());

        auto values         = this->values();
        TreeLayout layout   = compute_tree_layout(meta->data_size());
        size_t pos          = locate(layout, values, start).idx;
        size_t data_size    = meta->data_size();

        Codec codec;

        int32_t c;
        for (c = start; c < end && pos < data_size; c++)
        {
            Value value;
            auto len = codec.decode(values, value, pos);
            fn(block, value);
            fn.next();

            pos += len;
        }
    }

    template <typename ConsumerFn>
    int32_t describe(int32_t block, int32_t start, int32_t end, ConsumerFn&& fn) const
    {
        auto meta = this->metadata();

        MEMORIA_V1_ASSERT(start, >=, 0);
        MEMORIA_V1_ASSERT(start, <=, end);
        MEMORIA_V1_ASSERT(end, <=, meta->size());

        auto values         = this->values();
        TreeLayout layout   = compute_tree_layout(meta->data_size());
        size_t pos          = locate(layout, values, start).idx;
        size_t data_size    = meta->data_size();

        Codec codec;

        int32_t c;
        for (c = start; c < end && pos < data_size; c++)
        {
            auto descr = codec.describe(values, pos);
            if (fn(block, descr))
            {
                fn.next();
                pos += descr.length();
            }
            else {
                return c - start;
            }
        }

        return end - start;
    }


    template <typename ConsumerFn>
    void read(int32_t start, int32_t end, ConsumerFn&& fn) const
    {
        read(0, start, end, std::forward<ConsumerFn>(fn));
    }

    OpStatus reindex()
    {
        auto meta = this->metadata();

        int32_t data_size     = meta->data_size();
        TreeLayout layout = compute_tree_layout(data_size);

        if (isFail(reindex(layout, meta))) {
            return OpStatus::FAIL;
        }

        return OpStatus::OK;
    }

    void check() const
    {
        auto meta = this->metadata();

        int32_t data_size       = meta->data_size();
        TreeLayout layout   = compute_tree_layout(data_size);

        check(layout, meta);
    }




    struct FindGEWalker {
        const Value& target_;
        int32_t idx_ = 0;
    public:
        FindGEWalker(const Value& target): target_(target) {}

        template <typename T>
        bool compare(T&& value)
        {
            return compare_ge(value, target_);
        }

        void next() {}

        int32_t& local_pos() {return idx_;}
        const int32_t& local_pos() const {return idx_;}

        FindGEWalker& idx(int32_t value) {
            idx_ = value;
            return *this;
        }
    };

    struct FindGTWalker {
        const Value& target_;

        int32_t idx_;
    public:
        FindGTWalker(const Value& target): target_(target) {}

        template <typename T>
        bool compare(T&& value)
        {
            return compare_gt(value, target_);
        }

        void next() {}

        int32_t& local_pos() {return idx_;}
        const int32_t& local_pos() const {return idx_;}

        FindGTWalker& idx(int32_t value) {
            idx_ = value;
            return *this;
        }
    };


protected:

    int32_t data_block_size() const
    {
        int32_t size = this->element_size(VALUES);
        return PackedAllocatable::roundUpBytesToAlignmentBlocks(size) / sizeof(ValueData);
    }



    bool has_index() const
    {
        return this->element_size(INDEX) > 0;
    }


    MyType* index() {
        return this->template get<MyType>(INDEX);
    }

    const MyType* index() const {
        return this->template get<MyType>(INDEX);
    }

    int32_t* size_index() {
        return this->template get<int32_t>(SIZE_INDEX);
    }

    const int32_t* size_index() const {
        return this->template get<int32_t>(SIZE_INDEX);
    }

    OffsetsType offset(int32_t idx) const
    {
        return offsets()[idx];
    }

    void set_offset(int32_t idx, OffsetsType value)
    {
        offsets()[idx] = value;
    }

    void set_offset(OffsetsType* block, int32_t idx, int32_t value)
    {
        block[idx] = value;
    }

    OffsetsType* offsets() {
        return this->template get<OffsetsType>(OFFSETS);
    }

    const OffsetsType* offsets() const {
        return this->template get<OffsetsType>(OFFSETS);
    }

    ValueData* values() {
        return this->template get<ValueData>(VALUES);
    }
    const ValueData* values() const {
        return this->template get<ValueData>(VALUES);
    }

    static constexpr int32_t number_of_offsets(int32_t values)
    {
        return values > 0 ? divUpV(values) : 1;
    }

    static constexpr int32_t offsets_segment_size(int32_t values)
    {
        return PackedAllocatable::roundUpBytesToAlignmentBlocks(number_of_offsets(values) * sizeof(OffsetsType));
    }

    static constexpr int32_t divUpV(int32_t value) {
        return (value >> BranchingFactorVLog2) + ((value & BranchingFactorVMask) ? 1 : 0);
    }

    static constexpr int32_t divUpI(int32_t value) {
        return (value >> BranchingFactorILog2) + ((value & BranchingFactorIMask) ? 1 : 0);
    }

//    template <int32_t Divisor>
//    static constexpr int32_t divUp(int32_t value, int32_t divisor) {
//        return (value / Divisor) + ((value % Divisor) ? 1 : 0);
//    }



    LocateResult locate(TreeLayout& layout, const ValueData* values, int32_t idx) const
    {
        auto meta = this->metadata();

        size_t data_size = meta->data_size();

        if (data_size > 0)
        {
            LocateResult locate_result;

            if (layout.levels_max >= 0)
            {
                layout.valaue_block_size_prefix = this->size_index();
                locate_result = this->locate_index(layout, idx);
            }

            int32_t window_num = locate_result.idx;

            int32_t window_start = window_num << BranchingFactorVLog2;
            if (window_start >= 0)
            {
                Codec codec;

                size_t offset = this->offset(window_num);

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


    LocateResult locate_index(TreeLayout& data, int32_t idx) const
    {
        int32_t branch_start = 0;

        int32_t sum = 0;

        for (int32_t level = 1; level <= data.levels_max; level++)
        {
            int32_t level_start = data.level_starts[level];

            for (int c = level_start + branch_start; c < level_start + data.level_sizes[level]; c++)
            {
                if (sum + data.valaue_block_size_prefix[c] > idx)
                {
                    if (level < data.levels_max)
                    {
                        branch_start = (c - level_start) << BranchingFactorILog2;
                        goto next_level;
                    }
                    else {
                        return LocateResult(c - level_start, sum);
                    }
                }
                else {
                    sum += data.valaue_block_size_prefix[c];
                }
            }

            return LocateResult(-1, sum);

            next_level:;
        }

        return LocateResult(-1, sum);
    }

    int32_t sum_index(const TreeLayout& layout, int32_t end) const
    {
        int32_t sum = 0;
        sum_index(layout, sum, 0, end, layout.levels_max);
        return sum;
    }


    void sum_index(const TreeLayout& layout, int32_t& sum, int32_t start, int32_t end, int32_t level) const
    {
        int32_t level_start = layout.level_starts[level];

        int32_t branch_end = (start | BranchingFactorIMask) + 1;
        int32_t branch_start = end & ~BranchingFactorIMask;

        if (end <= branch_end || branch_start == branch_end)
        {
            for (int32_t c = start + level_start; c < end + level_start; c++)
            {
                sum += layout.valaue_block_size_prefix[c];
            }
        }
        else {
            for (int32_t c = start + level_start; c < branch_end + level_start; c++)
            {
                sum += layout.valaue_block_size_prefix[c];
            }

            sum_index(
                    layout,
                    sum,
                    branch_end >> BranchingFactorILog2,
                    branch_start >> BranchingFactorILog2,
                    level - 1
            );

            for (int32_t c = branch_start + level_start; c < end + level_start; c++)
            {
                sum += layout.valaue_block_size_prefix[c];
            }
        }
    }


    OpStatus resize(int32_t data_size, int32_t length)
    {
        int32_t new_data_size = data_size + length;

        int32_t data_segment_size    = PackedAllocatable::roundUpBytesToAlignmentBlocks(new_data_size);
        int32_t offsets_segment_size = this->offsets_segment_size(new_data_size);
        int32_t index_size           = MyType::index_size(new_data_size);

        if (isFail(this->resizeBlock(VALUES, data_segment_size))) {
            return OpStatus::FAIL;
        }

        if (isFail(this->resizeBlock(OFFSETS, offsets_segment_size))) {
            return OpStatus::FAIL;
        }

        if (isFail(this->resizeBlock(SIZE_INDEX, index_size * sizeof(SizesValue)))) {
            return OpStatus::FAIL;
        }

        return OpStatus::OK;
    }


    OpStatus insert_space(int32_t start, int32_t length)
    {
        auto meta = this->metadata();

        int32_t data_size = meta->data_size();
        if (isFail(resize(data_size, length))) {
            return OpStatus::FAIL;
        }

        auto values = this->values();

        Codec codec;
        codec.move(values, start, start + length, data_size - start);

        meta->data_size() += length;

        return OpStatus::OK;
    }



    OpStatus remove_space(int32_t start, int32_t length)
    {
        auto meta = this->metadata();

        int32_t data_size = meta->data_size();
        auto values = this->values();

        Codec codec;
        int32_t end = start + length;
        codec.move(values, end, start, data_size - end);

        if (isFail(resize(data_size, -(end - start)))) {
            return OpStatus::FAIL;
        }

        meta->data_size() -= (end - start);

        return OpStatus::OK;
    }

    static int32_t index_size(int32_t capacity)
    {
        TreeLayout layout;
        compute_tree_layout(capacity, layout);
        return layout.index_size;
    }


    OpStatus reindex(TreeLayout& layout, Metadata* meta)
    {
        if (layout.levels_max >= 0)
        {
            auto values     = this->values();
            auto size_index = this->size_index();
            auto offsets    = this->offsets();

            Base::clear(SIZE_INDEX);
            Base::clear(OFFSETS);

            layout.valaue_block_size_prefix = size_index;

            int32_t levels      = layout.levels_max + 1;
            int32_t level_start = layout.level_starts[levels - 1];
            size_t data_size   = meta->data_size();

            std::unique_ptr<ValueData[]> buffer = std::make_unique<ValueData[]>(data_size);

            Codec codec;

            size_t pos = 0;
            SizesValue size_cnt = 0;
            size_t threshold = BranchingFactorV;
            size_t buffer_pos = 0;

            set_offset(offsets, 0, 0);

            int32_t idx = 0;

            typename Codec::ValuePtr value;

            while(pos < data_size)
            {
                if (pos >= threshold)
                {
                    set_offset(offsets, idx + 1, pos - threshold);
                    size_index[level_start + idx] = size_cnt;

                    threshold += BranchingFactorV;

                    buffer_pos += codec.encode(buffer.get(), value, buffer_pos);

                    idx++;
                    size_cnt  = 0;
                }

                value = codec.describe(values, pos);

                size_cnt++;

                pos += value.length();
            }

            buffer_pos += codec.encode(buffer.get(), value, buffer_pos);
            size_index[level_start + idx] = size_cnt;

            idx++;

            for (int32_t level = levels - 1; level > 0; level--)
            {
                int32_t previous_level_start = layout.level_starts[level - 1];
                int32_t previous_level_size  = layout.level_sizes[level - 1];

                int32_t current_level_start  = layout.level_starts[level];

                int32_t current_level_size = layout.level_sizes[level];

                for (int i = 0; i < previous_level_size; i++)
                {
                    SizesValue sizes_sum  = 0;

                    int32_t start       = (i << BranchingFactorILog2) + current_level_start;
                    int32_t window_end  = ((i + 1) << BranchingFactorILog2);

                    int32_t end = (window_end <= current_level_size ? window_end : current_level_size) + current_level_start;

                    for (int32_t c = start; c < end; c++)
                    {
                        sizes_sum += size_index[c];
                    }

                    size_index[previous_level_start + i] = sizes_sum;
                }
            }

            Base::free(INDEX);

            if (idx > 0)
            {                
                if(isFail(this->template allocateEmpty<MyType>(INDEX))) {
                    return OpStatus::FAIL;
                }

                if (isFail(index()->insert_data(0, buffer.get(), 0, buffer_pos, idx))) {
                    return OpStatus::FAIL;
                }
            }
        }
        else {
            Base::clear(OFFSETS);
            Base::free(INDEX);
        }

        return OpStatus::OK;
    }


    void check(TreeLayout& layout, const Metadata* meta) const
    {
        size_t data_size = meta->data_size();
        int32_t offsets_size = this->element_size(OFFSETS);

        Codec codec;

        if (layout.levels_max >= 0)
        {
            MEMORIA_V1_ASSERT(this->element_size(SIZE_INDEX), >, 0);

            auto values     = this->values();
            auto size_index = this->size_index();

            layout.valaue_block_size_prefix = size_index;

            int32_t levels = layout.levels_max + 1;

            int32_t level_start = layout.level_starts[levels - 1];



            size_t pos = 0;
            SizesValue size_cnt = 0;
            size_t threshold = BranchingFactorV;
            int32_t total_size = 0;

            MEMORIA_V1_ASSERT(offset(0), ==, 0);

            auto index = has_index() ? this->index() : nullptr;

            Value prev_value;
            Value value;

            int32_t idx = 0;
            while(pos < data_size)
            {
                if (pos >= threshold)
                {
                    MEMORIA_V1_ASSERT(offset(idx + 1), ==, pos - threshold);
                    MEMORIA_V1_ASSERT(size_index[level_start + idx], ==, size_cnt);

                    threshold += BranchingFactorV;

                    if (index)
                    {
                        MEMORIA_V1_ASSERT(value, ==, index->value(idx));
                    }

                    idx++;

                    total_size += size_cnt;

                    size_cnt  = 0;
                }

                auto len = codec.decode(values, value, pos);

                size_cnt++;

                pos += len;
            }

            MEMORIA_V1_ASSERT(pos, ==, data_size);

            if (index) {
                MEMORIA_V1_ASSERT(value, ==, index->value(idx));
            }

            MEMORIA_V1_ASSERT(size_index[level_start + idx], ==, size_cnt);
            MEMORIA_V1_ASSERT(meta->size(), ==, size_cnt + total_size);

            for (int32_t level = levels - 1; level > 0; level--)
            {
                int32_t previous_level_start = layout.level_starts[level - 1];
                int32_t previous_level_size  = layout.level_sizes[level - 1];

                int32_t current_level_start  = layout.level_starts[level];

                int32_t current_level_size = layout.level_sizes[level];

                for (int i = 0; i < previous_level_size; i++)
                {
                    SizesValue sizes_sum  = 0;

                    int32_t start       = (i << BranchingFactorILog2) + current_level_start;
                    int32_t window_end  = ((i + 1) << BranchingFactorILog2);

                    int32_t end = (window_end <= current_level_size ? window_end : current_level_size) + current_level_start;

                    for (int32_t c = start; c < end; c++)
                    {
                        sizes_sum += size_index[c];
                    }

                    MEMORIA_V1_ASSERT(size_index[previous_level_start + i], ==, sizes_sum);
                }
            }

            if (index) {
                index->check();
            }
        }
        else {
            MEMORIA_V1_ASSERT(this->element_size(SIZE_INDEX), ==, 0);

            int32_t data_size = meta->data_size();

            if (data_size > 0)
            {
//              auto values = this->values();
//
//              Value prev_value;
//              Value value;
//
//              size_t pos = 0;
//
//              int32_t idx = 0;
//              while(pos < data_size)
//              {
//                  auto len = codec.decode(values, value, pos);
//
//                  if (pos > 0)
//                  {
//                      MEMORIA_V1_ASSERT(value, >=, prev_value);
//                  }
//
//                  prev_value = value;
//
//                  pos += len;
//                  idx++;
//              }
//
//              MEMORIA_V1_ASSERT((int32_t)pos, ==, data_size);
//              MEMORIA_V1_ASSERT(idx, ==, meta->size());
//
//              MEMORIA_V1_ASSERT(offsets_size, ==, offsets_segment_size(data_size));
//              MEMORIA_V1_ASSERT(offset(0), ==, 0);
            }
            else {
                MEMORIA_V1_ASSERT(offsets_size, ==, PackedAllocatable::roundUpBytesToAlignmentBlocks(sizeof(OffsetsType)));
            }

            MEMORIA_V1_ASSERT(meta->data_size(), <=, (int32_t)BranchingFactorV);
        }
    }

    OpStatus insert_data(int32_t at, const ValueData* buffer, int32_t start, int32_t end, int32_t size)
    {
        auto meta = this->metadata();

        Codec codec;

        int32_t data_size = end - start;

        auto values = this->values();

        size_t insertion_pos = at;

        if (isFail(insert_space(insertion_pos, data_size))) {
            return OpStatus::FAIL;
        }

        values = this->values();

        codec.copy(buffer, start, values, insertion_pos, data_size);

        meta->size() += size;

        if (isFail(reindex())) {
            return OpStatus::FAIL;
        }

        return OpStatus::OK;
    }

    static TreeLayout compute_tree_layout(int32_t size)
    {
        TreeLayout layout;
        compute_tree_layout(size, layout);
        return layout;
    }

    static int32_t compute_tree_layout(int32_t size, TreeLayout& layout)
    {
        if (size <= BranchingFactorV)
        {
            layout.levels_max = -1;
            layout.index_size = 0;

            return 0;
        }
        else {
            int32_t level = 0;

            layout.level_sizes[level] = divUpV(size);
            level++;

            while((layout.level_sizes[level] = divUpI(layout.level_sizes[level - 1])) > 1)
            {
                level++;
            }

            level++;

            for (int c = 0; c < level / 2; c++)
            {
                auto tmp = layout.level_sizes[c];
                layout.level_sizes[c] = layout.level_sizes[level - c - 1];
                layout.level_sizes[level - c - 1] = tmp;
            }

            int32_t level_start = 0;

            for (int c = 0; c < level; c++)
            {
                layout.level_starts[c] = level_start;
                level_start += layout.level_sizes[c];
            }

            layout.index_size = level_start;
            layout.levels_max = level - 1;

            return level;
        }
    }


};




template <typename Types>
struct PkdStructSizeType<PkdVBMTree<Types>> {
    static const PackedSizeType Value = PackedSizeType::VARIABLE;
};


template <typename Types>
struct StructSizeProvider<PkdVBMTree<Types>> {
    static const int32_t Value = 1;
};

template <typename Types>
struct IndexesSize<PkdVBMTree<Types>> {
    static const int32_t Value = 1;
};

template <typename T>
struct PkdSearchKeyTypeProvider<PkdVBMTree<T>> {
	using Type = OptionalT<typename PkdVBMTree<T>::Value>;
};


}}
