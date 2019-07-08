
// Copyright 2013 Victor Smirnov
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

#include <memoria/v1/core/packed/tools/packed_allocator.hpp>
#include <memoria/v1/core/packed/tree/fse/packed_fse_quick_tree.hpp>

#include <memoria/v1/core/tools/static_array.hpp>
#include <memoria/v1/core/tools/optional.hpp>
#include <memoria/v1/core/bignum/primitive_codec.hpp>

#include <memoria/v1/profiles/common/block_operations.hpp>

#include <memoria/v1/core/iovector/io_symbol_sequence_base.hpp>

#include "rleseq/rleseq_reindex_fn.hpp"
#include "rleseq/rleseq_iterator.hpp"

#include <ostream>

namespace memoria {
namespace v1 {

namespace io {

template <int32_t AlphabetSize> class PackedRLESymbolSequence;
template <int32_t AlphabetSize> class PackedRLESymbolSequenceView;

}


using rleseq::RLESymbolsRun;
using rleseq::Location;

namespace {
    static constexpr int32_t SymbolsRange(int32_t symbols) {
        return 0; // No ranges defined at the moment
    }

    template <int32_t Symbols, int32_t SymbolsRange = SymbolsRange(Symbols)>
    struct SumIndexFactory: HasType<PkdFQTreeT<int64_t, Symbols>> {};
}




template <
    int32_t Symbols_,
    int32_t BytesPerBlock
>
struct PkdRLSeqTypes {
    static const int32_t Blocks                 = Symbols_;
    static const int32_t ValuesPerBranch        = BytesPerBlock;
};

template <typename Types> class PkdRLESeq;

template <
    int32_t Symbols,
    int32_t BytesPerBlock = 128
>
using PkdRLESeqT = PkdRLESeq<PkdRLSeqTypes<Symbols, BytesPerBlock>>;

template <typename Types_>
class PkdRLESeq: public PackedAllocator {

    using Base = PackedAllocator;

public:
    static constexpr uint32_t VERSION = 1;

    using Types  = Types_;
    using MyType = PkdRLESeq<Types_>;

    static constexpr PkdSearchType KeySearchType    = PkdSearchType::SUM;
    static constexpr PkdSearchType SearchType       = PkdSearchType::SUM;
    static const PackedSizeType SizeType            = PackedSizeType::VARIABLE;

    static constexpr int32_t ValuesPerBranch            = Types::ValuesPerBranch;
    static constexpr int32_t ValuesPerBranchLog2        = NumberOfBits(Types::ValuesPerBranch);

    static constexpr int32_t Indexes                    = Types::Blocks;
    static constexpr int32_t Symbols                    = Types::Blocks;

    static constexpr size_t MaxRunLength            = MaxRLERunLength;

    enum {
        METADATA, SIZE_INDEX, OFFSETS, SUM_INDEX, SYMBOLS, TOTAL_SEGMENTS__
    };

    using Base::clear;

    using Value = uint8_t;
    using IndexValue = int64_t;

    using Codec = ValueCodec<uint64_t>;

    using Values = core::StaticVector<int64_t, Indexes>;

    using SumIndex  = typename SumIndexFactory<Symbols>::Type;
    using SizeIndex = PkdFQTreeT<int64_t, 1>;

    using OffsetsType = uint8_t;

    class Metadata {
        int32_t size_;
        int32_t data_size_;
    public:
        int32_t& size()                 {return size_;}
        const int32_t& size() const     {return size_;}

        int32_t& data_size()                 {return data_size_;}
        const int32_t& data_size() const     {return data_size_;}
    };

    using SizesT = core::StaticVector<int32_t, 1>;

    struct Tools {};

    using Iterator = rleseq::RLESeqIterator<MyType>;

    using GrowableIOSubstream = io::PackedRLESymbolSequence<Symbols>;
    using IOSubstreamView     = io::PackedRLESymbolSequenceView<Symbols>;

    int32_t number_of_offsets() const
    {
        return number_of_offsets(this->element_size(SYMBOLS));
    }

    static constexpr int32_t number_of_offsets(int32_t values)
    {
        return values > 0 ? divUp(values, ValuesPerBranch) : 1;
    }

    static constexpr int32_t number_of_indexes(int32_t capacity)
    {
        return capacity <= ValuesPerBranch ? 0 : divUp(capacity, ValuesPerBranch);
    }

    static constexpr int32_t offsets_segment_size(int32_t values)
    {
        return PackedAllocatable::roundUpBytesToAlignmentBlocks(number_of_offsets(values) * sizeof(OffsetsType));
    }

    static constexpr size_t divUp(size_t value, size_t divisor) {
        return (value / divisor) + ((value % divisor) ? 1 : 0);
    }



public:
    PkdRLESeq() = default;

    int32_t& size() {return metadata()->size();}
    const int32_t& size() const {return metadata()->size();}

    int32_t& data_size() {return metadata()->data_size();}
    const int32_t& data_size() const {return metadata()->data_size();}



    static constexpr RLESymbolsRun decode_run(uint64_t value)
    {
        return rleseq::DecodeRun<Symbols>(value);
    }

    static constexpr uint64_t encode_run(int32_t symbol, uint64_t length)
    {
        return rleseq::EncodeRun<Symbols, MaxRunLength>(symbol, length);
    }


    // ====================================== Accessors ================================= //

    Metadata* metadata()
    {
        return Base::template get<Metadata>(METADATA);
    }

    const Metadata* metadata() const
    {
        return Base::template get<Metadata>(METADATA);
    }

    SumIndex* sum_index()
    {
        return Base::template get<SumIndex>(SUM_INDEX);
    }

    const SumIndex* sum_index() const
    {
        return Base::template get<SumIndex>(SUM_INDEX);
    }

    SizeIndex* size_index()
    {
        return Base::template get<SizeIndex>(SIZE_INDEX);
    }

    const SizeIndex* size_index() const
    {
        return Base::template get<SizeIndex>(SIZE_INDEX);
    }

    OffsetsType* offsets() {
        return this->template get<OffsetsType>(OFFSETS);
    }

    const OffsetsType* offsets() const {
        return this->template get<OffsetsType>(OFFSETS);
    }

    OffsetsType offset(int32_t idx) const
    {
        return offsets()[idx];
    }

    void set_offset(int32_t idx, OffsetsType value)
    {
        set_offsets(offsets(), idx, value);
    }

    void set_offset(OffsetsType* block, int32_t idx, int32_t value)
    {
        block[idx] = value;
    }

    int32_t symbols_block_capacity() const
    {
        return symbols_block_capacity(metadata());
    }

    int32_t symbols_block_capacity(const Metadata* meta) const
    {
        return symbols_block_size() - meta->data_size();
    }

    int32_t symbols_block_size() const
    {
        return this->element_size(SYMBOLS);
    }

    bool has_index() const {
        return Base::element_size(SIZE_INDEX) > 0;
    }

    Value* symbols()
    {
        return Base::template get<Value>(SYMBOLS);
    }

    const Value* symbols() const
    {
        return Base::template get<Value>(SYMBOLS);
    }

    Tools tools() const {
        return Tools();
    }

    class SymbolAccessor {
        MyType& seq_;
        int32_t idx_;
    public:
        SymbolAccessor(MyType& seq, int32_t idx): seq_(seq), idx_(idx) {}

        Value operator=(Value val)
        {
            seq_.set_symbol(idx_, val);
            return val;
        }

        operator Value() const {
            return seq_.get_symbol(idx_);
        }

        Value value() const {
            return seq_.get_symbol(idx_);
        }
    };

    SymbolAccessor symbol(int32_t idx)
    {
        return SymbolAccessor(*this, idx);
    }

    int32_t get_symbol(int32_t idx) const
    {
        auto result = this->find_run(idx);

        if (!result.out_of_range1())
        {
            return result.run().symbol();
        }
        else {
            MMA1_THROW(BoundsException()) << WhatInfo(fmt::format8(u"Symbol index {} is out of range {}", idx, this->size()));
        }
    }



    void set_symbol(int32_t idx, int32_t symbol)
    {
        auto location = this->find_run(idx);

        if (!location.out_of_range())
        {
            if (location.symbol() != symbol)
            {
                if (location.run_prefix() > 0)
                {
                    if (location.run_suffix() > 1)
                    {
                        location = split_run(location, 1);
                        insert_run(location.data_pos(), symbol, 1);
                    }
                    else {
                        auto run_value_length = add_run_length(location, -1);
                        insert_run(location.data_pos() + run_value_length, symbol, 1);
                    }
                }
                else if (location.run().length() > 1)
                {
                    add_run_length(location, -1);
                    insert_run(location.data_pos(), symbol, 1);
                }
                else {
                    remove_run(location);
                    insert_run(location.data_pos(), symbol, 1);
                    try_merge_two_adjustent_runs(location.data_pos());
                }

                reindex();
            }
        }
        else {
            MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Symbol index {} is out of range {}", idx, this->size()));
        }
    }


    int32_t value(int32_t symbol, int32_t idx) const {
        return this->symbol(idx) == symbol;
    }


    class ConstSymbolAccessor {
        const MyType& seq_;
        int32_t idx_;
    public:
        ConstSymbolAccessor(const MyType& seq, int32_t idx): seq_(seq), idx_(idx) {}

        operator Value() const {
            return seq_.get_symbol(idx_);
        }

        Value value() const {
            return seq_.get_symbol(idx_);
        }
    };

    ConstSymbolAccessor symbol(int32_t idx) const
    {
        return ConstSymbolAccessor(*this, idx);
    }


    MMA1_PKD_OOM_SAFE
    OpStatus append(int32_t symbol, uint64_t length)
    {
        MEMORIA_V1_ASSERT_TRUE(symbol >= 0 && symbol < Symbols);

        auto meta = this->metadata();

        auto run_value = encode_run(symbol, length);

        Codec codec;
        auto len = codec.length(run_value);

        if(isFail(ensure_capacity(len))) {
            return OpStatus::FAIL;
        }

        meta->data_size() += codec.encode(this->symbols(), run_value, meta->data_size());
        meta->size() += length;

        return OpStatus::OK;
    }



    OpStatus append_and_reindex(int32_t symbol, uint64_t length)
    {
        if(isFail(append(symbol, length))) {
            return OpStatus::FAIL;
        }
        return reindex();
    }

    bool emplace_back(int32_t symbol, uint64_t length)
    {
        auto meta = this->metadata();

        auto run_value = encode_run(symbol, length);

        Codec codec;
        auto len = codec.length(run_value);

        if (has_capacity(len))
        {
            meta->data_size() += codec.encode(this->symbols(), run_value, meta->data_size());
            meta->size()      += length;

            return true;
        }

        return false;
    }


public:

    // ===================================== Allocation ================================= //

    using PackedAllocator::block_size;

    int32_t block_size(const MyType* other) const
    {
        return block_size(this->symbols_block_size() + other->symbols_block_size());
    }

    static int32_t block_size(int32_t symbols_capacity)
    {
        int32_t metadata_length     = PackedAllocatable::roundUpBytesToAlignmentBlocks(sizeof(Metadata));

        int32_t capacity = symbols_capacity;

        int32_t index_size = number_of_indexes(capacity);

        int32_t size_index_length   = index_size > 0 ? SizeIndex::block_size(index_size) : 0;
        int32_t sum_index_length    = index_size > 0 ? SumIndex::block_size(index_size) : 0;

        int32_t offsets_length      = offsets_segment_size(capacity);
        int32_t values_length       = PackedAllocatable::roundUpBytesToAlignmentBlocks(capacity);

        int32_t block_size          = Base::block_size(metadata_length + size_index_length  + offsets_length + sum_index_length + values_length, TOTAL_SEGMENTS__);
        return block_size;
    }

    OpStatus init(int32_t block_size)
    {
        MEMORIA_V1_ASSERT(block_size, >=, empty_size());

        return init();
    }

    OpStatus init() {
        return init_bs(empty_size());
    }

    OpStatus init_bs(int32_t block_size)
    {
        if(isFail(Base::init(block_size, TOTAL_SEGMENTS__))) {
            return OpStatus::FAIL;
        }

        Metadata* meta  = Base::template allocate<Metadata>(METADATA);

        if(isFail(meta)) {
            return OpStatus::FAIL;
        }

        meta->size() = 0;
        meta->data_size() = 0;

        if(isFail(this->template allocateArrayBySize<OffsetsType>(OFFSETS, number_of_offsets(0)))) {
            return OpStatus::FAIL;
        }

        Base::setBlockType(SIZE_INDEX, PackedBlockType::ALLOCATABLE);
        Base::setBlockType(SUM_INDEX,  PackedBlockType::ALLOCATABLE);
        Base::setBlockType(SYMBOLS, PackedBlockType::RAW_MEMORY);

        return OpStatus::OK;
        // other sections are empty at this moment
    }

    OpStatus clear()
    {
        if(isFail(Base::resizeBlock(SYMBOLS, 0))) {
            return OpStatus::FAIL;
        }

        if(isFail(removeIndex())) {
            return OpStatus::FAIL;
        }

        auto meta = this->metadata();

        meta->size()        = 0;
        meta->data_size()   = 0;

        return OpStatus::OK;
    }

    void reset() {
        auto meta = this->metadata();

        meta->size()        = 0;
        meta->data_size()   = 0;
    }


public:
    static int32_t empty_size()
    {
        int32_t metadata_length     = PackedAllocatable::roundUpBytesToAlignmentBlocks(sizeof(Metadata));
        int32_t size_index_length   = 0;
        int32_t offsets_length      = offsets_segment_size(0);
        int32_t sum_index_length    = 0;
        int32_t values_length       = 0;

        int32_t block_size          = Base::block_size(metadata_length + size_index_length  + offsets_length + sum_index_length + values_length, TOTAL_SEGMENTS__);
        return block_size;
    }


    OpStatus removeIndex()
    {
        Base::free(SIZE_INDEX);
        Base::free(SUM_INDEX);

        if(isFail(Base::resizeBlock(OFFSETS, offsets_segment_size(0)))) {
            return OpStatus::FAIL;
        }

        return OpStatus::OK;
    }


    OpStatus createIndex(int32_t index_size)
    {
        int32_t size_index_block_size = SizeIndex::block_size(index_size);
        if(isFail(Base::resizeBlock(SIZE_INDEX, size_index_block_size))) {
            return OpStatus::FAIL;
        }

        int32_t sum_index_block_size = SumIndex::block_size(index_size);
        if(isFail(Base::resizeBlock(SUM_INDEX, sum_index_block_size))) {
            return OpStatus::FAIL;
        }

        auto size_index = this->size_index();
        size_index->allocatable().setAllocatorOffset(this);
        if(isFail(size_index->init(index_size))) {
            return OpStatus::FAIL;
        }

        auto sum_index = this->sum_index();
        sum_index->allocatable().setAllocatorOffset(this);
        if(isFail(sum_index->init(index_size))) {
            return OpStatus::FAIL;
        }

        return OpStatus::OK;
    }


    uint64_t populate_entry(io::SymbolsBuffer& buffer, uint64_t idx, int32_t symbol, bool entry_start) const
    {
        auto iter = this->iterator(idx);
        if (MMA1_LIKELY(iter.has_data()))
        {
            if (MMA1_UNLIKELY(entry_start && iter.symbol() == symbol))
            {
                if (MMA1_UNLIKELY(iter.remaining_run_length() > 1))
                {
                    buffer.append_run(iter.symbol(), iter.remaining_run_length());
                    return idx + 1;
                }
                else
                {
                    iter.next_run();
                }
            }

            while (iter.has_data())
            {
                if (iter.symbol() >= symbol)
                {
                    buffer.append_run(iter.symbol(), iter.remaining_run_length());
                    iter.next_run();
                }
                else {
                    return iter.local_pos();
                }
            }
        }

        return this->size();
    }

    uint64_t populate_while(io::SymbolsBuffer& buffer, uint64_t idx, int32_t symbol) const
    {
        auto iter = this->iterator(idx);
        while (iter.has_data())
        {
            if (iter.symbol() >= symbol)
            {
                buffer.append_run(iter.symbol(), iter.remaining_run_length());
                iter.next_run();
            }
            else {
                return iter.local_pos();
            }
        }

        return this->size();
    }

    uint64_t populate(io::SymbolsBuffer& buffer, uint64_t idx) const
    {
        auto iter = this->iterator(idx);
        while (iter.has_data())
        {
            buffer.append_run(iter.symbol(), iter.remaining_run_length());
            iter.next_run();
        }

        return this->size();
    }

    uint64_t populate(io::SymbolsBuffer& buffer, uint64_t idx, uint64_t size) const
    {
        auto iter = this->iterator(idx);
        uint64_t total{};
        while (iter.has_data())
        {
            auto len = iter.remaining_run_length();

            if (MMA1_LIKELY(len + total <= size))
            {
                total += len;
                buffer.append_run(iter.symbol(), len);
                iter.next_run();
            }
            else {
                auto remaining = size - total;
                buffer.append_run(iter.symbol(), remaining);
                total += remaining;
            }
        }

        buffer.finish();

        return idx + total;
    }

    Iterator iterator(int32_t symbol_pos) const
    {
        auto meta = this->metadata();

        if (symbol_pos < meta->size())
        {
            if (!has_index())
            {
                auto result = locate_run(meta, 0, symbol_pos, 0);
                return Iterator(symbols(), result.data_pos(), meta->data_size(), result.local_idx(), result.run_base(), result.run());
            }
            else
            {
                auto find_result = this->size_index()->find_gt(0, symbol_pos);

                int32_t local_pos       = symbol_pos - find_result.prefix();
                size_t block_offset = find_result.local_pos() * ValuesPerBranch;
                auto offset         = offsets()[find_result.local_pos()];

                block_offset += offset;

                auto result = locate_run(meta, block_offset, local_pos, find_result.prefix());
                return Iterator(symbols(), result.data_pos(), meta->data_size(), result.local_idx(), result.run_base(), result.run());
            }
        }
        else {
            return end();
        }
    }

    Iterator end() const
    {
        auto meta = this->metadata();
        return Iterator(symbols(), meta->data_size(), meta->data_size(), meta->size(), 0, RLESymbolsRun());
    }

    Iterator begin() const
    {
        return iterator(0);
    }


    // ========================================= Update ================================= //

    MMA1_PKD_OOM_SAFE
    OpStatus reindex()
    {
        rleseq::ReindexFn<MyType> reindex_fn;
        return reindex_fn.reindex(*this);
    }

    void check() const
    {
        if (has_index())
        {
            size_index()->check();
            sum_index()->check();

            rleseq::ReindexFn<MyType> reindex_fn;
            reindex_fn.check(*this);
        }
    }

    OpStatus ensure_capacity(int32_t capacity)
    {
        int32_t current_capacity = this->symbols_block_capacity();

        if (current_capacity < capacity)
        {
            return enlargeData(capacity - current_capacity);
        }

        return OpStatus::OK;
    }

    bool has_capacity(int32_t required_capacity) const
    {
        int32_t current_capacity = this->symbols_block_capacity();
        return current_capacity >= required_capacity;
    }


    OpStatus enlargeData(int32_t length)
    {
        int32_t new_size = this->element_size(SYMBOLS) + length;
        return toOpStatus(Base::resizeBlock(SYMBOLS, new_size));
    }

protected:

    MMA1_PKD_OOM_SAFE
    OpStatus shrinkData(int32_t length)
    {
        int32_t current_size = this->element_size(SYMBOLS);

        int32_t new_size = current_size - length;

        if (!isFail(new_size))
        {
            return toOpStatus(Base::resizeBlock(SYMBOLS, new_size));
        }

        return OpStatus::OK;
    }


    MMA1_PKD_OOM_SAFE
    OpStatus shrink_to_data()
    {
        return shrinkData(this->symbols_block_capacity());
    }

public:

    template <int32_t Offset, int32_t Size, typename AccessorFn, typename T2, template <typename, int32_t> class BranchNodeEntryItem>
    OpStatus _insert_b(int32_t idx, BranchNodeEntryItem<T2, Size>& accum, AccessorFn&& values)
    {
        return insert(idx, values(0));
    }



    OpStatus insert(int32_t pos, int32_t symbol)
    {
        return insert(pos, symbol, 1);
    }

    OpStatus removeSpace(int32_t start, int32_t end) {
        return remove(start, end);
    }

    OpStatus remove(int32_t start, int32_t end, bool compactify = false)
    {
        auto meta = this->metadata();

        MEMORIA_V1_ASSERT(start, >=, 0);
        MEMORIA_V1_ASSERT(end, >=, 0);
        MEMORIA_V1_ASSERT(start, <=, end);
        MEMORIA_V1_ASSERT(end, <=, meta->size());

        auto location_start = find_run(start);
        auto location_end   = find_run(end);

        if (location_end.local_idx() > 0)
        {
            if (location_start.run_prefix() > 0)
            {
                if(isFail(remove_runs(location_start, location_end))) {
                    return OpStatus::FAIL;
                }

                if (location_start.run().symbol() == location_end.run().symbol())
                {
                    try_merge_two_adjustent_runs(location_start.data_pos());
                }
            }
            else {
                remove_runs_one_sided_left(location_start, location_end);
                try_merge_two_adjustent_runs(location_start.data_pos());
            }
        }
        else if (location_start.run_prefix() > 0)
        {
            remove_runs_one_sided(location_start, location_end.data_pos());

            try_merge_two_adjustent_runs(location_start.data_pos());
        }
        else {
            remove_runs_two_sided(location_start.data_pos(), location_end.data_pos());
        }

        meta->size() -= end - start;

        if (compactify)
        {
            return this->compactify();
        }
        else {
            if(isFail(shrink_to_data())) {
                return OpStatus::FAIL;
            }
            return reindex();
        }
    }

//     void insert(int32_t idx, int32_t symbol, uint64_t length)
//     {
//         auto location = find_run(idx);
// 
//         if (!location.out_of_range1())
//         {
//             if (location.symbol() != symbol || location.run().length() + length > MaxRunLength)
//             {
//                 if (location.run_prefix() > 0)
//                 {
//                     location = split_run(location);
//                 }
// 
//                 insert_run(location.data_pos(), symbol, length);
//             }
//             else {
//                 add_run_length(location, length);
//             }
//         }
//         else {
//             insert_run(location.data_pos(), symbol, length);
//         }
// 
//         reindex();
//     }



    OpStatus insert(int32_t idx, int32_t symbol, uint64_t length)
    {
        auto meta = this->metadata();
        
        if (idx < meta->size()) 
        {
            auto location = find_run(meta, idx);
            
            if (location.run().symbol() != symbol || location.run().length() + length > MaxRunLength)
            {
                if (location.run_prefix() > 0)
                {
                    auto location_s = split_run(location);
                    if (isFail(location_s)) {
                        return OpStatus::FAIL;
                    }
                    location = location_s.value();
                }

                if(isFail(insert_run(location.data_pos(), symbol, length))) {
                    return OpStatus::FAIL;
                }
            }
            else {
                if(isFail(add_run_length(location, length))) {
                    return OpStatus::FAIL;
                }
            }
        }
        else {
            auto location = find_last_run(meta);
            
            if (location.run().symbol() != symbol || location.run().length() + length > MaxRunLength)
            {
                if(isFail(insert_run(location.data_pos() + location.data_length(), symbol, length))) {
                    return OpStatus::FAIL;
                }
            }
            else {
                if(isFail(add_run_length(location, length))) {
                    return OpStatus::FAIL;
                }
            }
        }
        
        return reindex();
    }


    MMA1_PKD_OOM_SAFE
    template <typename InputSource>
    OpStatus insert_from(int32_t at, const InputSource* buffer, int32_t start, int32_t size)
    {
        if (size > 0)
        {
            auto meta = this->metadata();

            MEMORIA_V1_ASSERT(at, <= , meta->size());

            auto start_run  = buffer->find_run(start);
            auto end_run    = buffer->find_run(start + size);

            auto symbols     = this->symbols();
            auto buf_symbols = buffer->symbols();

            Codec codec;

            auto location = find_run(at);

            if (start_run.data_pos() < end_run.data_pos())
            {
                if (location.run_prefix() > 0)
                {
                    auto location_s = split_run(location);
                    if (isFail(location_s)) {
                        return OpStatus::FAIL;
                    }

                    location = location_s.value();
                }

                uint64_t start_run_value = encode_run(start_run.run().symbol(), start_run.run_suffix());
                size_t start_run_value_length = codec.length(start_run_value);

                if (end_run.run_prefix() > 0)
                {
                    uint64_t end_run_value = encode_run(end_run.run().symbol(), end_run.run_prefix());
                    size_t end_run_value_length = codec.length(end_run_value);

                    size_t to_copy      = end_run.data_pos() - start_run.data_end();
                    size_t total_length = start_run_value_length + to_copy + end_run_value_length;

                    if(isFail(ensure_capacity(total_length))) {
                        return OpStatus::FAIL;
                    }

                    size_t pos = location.data_pos();
                    codec.move(symbols, pos, pos + total_length, meta->data_size() - pos);

                    pos += codec.encode(symbols, start_run_value, pos);
                    codec.copy(buf_symbols, start_run.data_end(), symbols, pos, to_copy);

                    pos += to_copy;
                    codec.encode(symbols, end_run_value, pos);

                    meta->data_size() += total_length;
                }
                else {
                    size_t to_copy      = end_run.data_pos() - start_run.data_end();
                    size_t total_length = start_run_value_length + to_copy;

                    if(isFail(ensure_capacity(total_length))) {
                        return OpStatus::FAIL;
                    }

                    size_t pos = location.data_pos();
                    codec.move(symbols, pos, pos + total_length, meta->data_size() - pos);

                    pos += codec.encode(symbols, start_run_value, pos);
                    codec.copy(buf_symbols, start_run.data_end(), symbols, pos, to_copy);

                    meta->data_size() += total_length;
                }

                meta->size() += size;
            }
            else if (location.run().symbol() == start_run.run().symbol())
            {
                if(isFail(add_run_length(location, size))) {
                    return OpStatus::FAIL;
                }
            }
            else {
                if (location.run_prefix() > 0)
                {
                    auto location_s = split_run(location);

                    if (isFail(location_s)) {
                        return OpStatus::FAIL;
                    }

                    location = location_s.value();
                }

                if(isFail(insert_run(location.data_pos(), start_run.run().symbol(), size))) {
                    return OpStatus::FAIL;
                }
            }

            return reindex();
        }

        return OpStatus::OK;
    }








    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    OpStatus _insert(int32_t idx, int32_t symbol, BranchNodeEntryItem<T, Size>& accum)
    {
        if(isFail(insert(idx, symbol))) {
            return OpStatus::FAIL;
        }

        sum<Offset>(idx, accum);

        return OpStatus::OK;
    }

    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    OpStatus _update(int32_t idx, int32_t symbol, BranchNodeEntryItem<T, Size>& accum)
    {
        sub<Offset>(idx, accum);

        this->symbol(idx) = symbol;

        if(isFail(this->reindex())) {
            return OpStatus::FAIL;
        }

        sum<Offset>(idx, accum);

        return OpStatus::OK;
    }

    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    OpStatus _remove(int32_t idx, BranchNodeEntryItem<T, Size>& accum)
    {
        sub<Offset>(idx, accum);
        return remove(idx, idx + 1);
    }


    // ========================================= Node ================================== //




    OpStatus splitTo(MyType* other, int32_t idx)
    {
        auto meta = this->metadata();

        if (idx < meta->size())
        {
            auto other_meta = other->metadata();

            auto location   = this->find_run(idx);

            Codec codec;

            size_t symbols_to_move      = location.run_suffix();
            uint64_t suffix_value       = encode_run(location.run().symbol(), symbols_to_move);
            size_t  suffix_value_length = codec.length(suffix_value);

            int32_t current_run_data_length = location.data_length();

            int32_t data_to_move_remainder  = meta->data_size() - (location.data_pos() + current_run_data_length);

            int32_t data_to_move = data_to_move_remainder + suffix_value_length;

            if(isFail(other->ensure_capacity(data_to_move))) {
                return OpStatus::FAIL;
            }

            codec.move(other->symbols(), 0, data_to_move, other_meta->data_size());

            codec.copy(symbols(), location.data_pos() + current_run_data_length, other->symbols(), suffix_value_length, data_to_move_remainder);
            codec.encode(other->symbols(), suffix_value, 0);

            other_meta->data_size() += data_to_move;
            other_meta->size()      += meta->size() - idx;

            //other->try_merge_two_adjustent_runs(0);

            if(isFail(other->compactify())) {
                return OpStatus::FAIL;
            }

            return remove(idx, meta->size(), true);
        }

        return OpStatus::OK;
    }

    OpStatus mergeWith(MyType* other) const
    {
        auto meta       = this->metadata();
        auto other_meta = other->metadata();

        int32_t data_to_move = meta->data_size();

        if (isFail(other->ensure_capacity(data_to_move))) {
            return OpStatus::FAIL;
        }

        Codec codec;

        //codec.move(other->symbols(), 0, data_to_move, other_meta->data_size());
        codec.copy(symbols(), 0, other->symbols(), other_meta->data_size(), data_to_move);

        other_meta->data_size() += data_to_move;
        other_meta->size()      += meta->size();

        if (isFail(other->compactify_runs())) {
            return OpStatus::FAIL;
        }

        return other->reindex();
    }


    // ========================================= Query ================================= //

    Values sums() const
    {
        if (has_index())
        {
            auto index = this->index();
            return index->sums();
        }
        else {
            return sums(size());
        }
    }


    Values sums(int32_t to) const
    {
        if (has_index())
        {
            auto index = this->index();

            int32_t index_block = to / ValuesPerBranch;

            auto isums = index->sums(0, index_block);

            auto vsums = tools().sum(index_block * ValuesPerBranch, to);

            vsums.sumUp(isums);

            return vsums;
        }
        else
        {
            auto vsums = tools().sum(0, to);
            return vsums;
        }
    }




    Values ranks(int32_t to) const
    {
        Values vals;

        for (int32_t symbol = 0; symbol < Indexes; symbol++)
        {
            vals[symbol] = rank(to, symbol);
        }

        return vals;
    }

    Values ranks() const
    {
        return this->ranks(this->size());
    }




    Values sumsAt(int32_t idx) const
    {
        Values values;
        values[symbol(idx)] = 1;

        return values;
    }

    Values sums(int32_t from, int32_t to) const
    {
        return sums(to) - sums(from);
    }


    void sums(int32_t from, int32_t to, Values& values) const
    {
        values += sums(from, to);
    }

    void sums(Values& values) const
    {
        values += sums();
    }

    Values sum_v(int32_t from, int32_t to) const {
        return sums(from, to);
    }


    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    void max(BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset <= Size - Indexes, "Invalid balanced tree structure");

        for (int32_t block = 0; block < Indexes; block++)
        {
            accum[block + Offset] = rank(block);
        }
    }



    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    void sum(BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset <= Size - Indexes, "Invalid balanced tree structure");

        for (int32_t block = 0; block < Indexes; block++)
        {
            accum[block + Offset] += rank(block);
        }
    }

    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    void sum(int32_t start, int32_t end, BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset <= Size - Indexes, "Invalid balanced tree structure");

        for (int32_t block = 0; block < Indexes; block++)
        {
            accum[block + Offset] += rank(start, end, block);
        }
    }

    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    void sum(int32_t idx, BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset <= Size - Indexes, "Invalid balanced tree structure");

        accum[symbol(idx) + Offset] ++;
    }

    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    void sub(int32_t idx, BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset <= Size - Indexes, "Invalid balanced tree structure");

        accum[symbol(idx) + Offset]--;
    }


    template <int32_t Offset, int32_t From, int32_t To, typename T, template <typename, int32_t, int32_t> class BranchNodeEntryItem>
    void sum(int32_t start, int32_t end, BranchNodeEntryItem<T, From, To>& accum) const
    {
        for (int32_t block = 0; block < Indexes; block++)
        {
            accum[block + Offset] += rank(block, start, end);
        }
    }


    int32_t sum(int32_t symbol, int32_t start, int32_t end) const
    {
        return rank(start, end, symbol);
    }

    int32_t sum(int32_t symbol, int32_t end) const
    {
        return rank(end, symbol);
    }

    int32_t sum(int32_t symbol) const
    {
        return rank(symbol);
    }





    template <typename T>
    void _add(int32_t symbol, T& value) const
    {
        value += rank(symbol);
    }

    template <typename T>
    void _add(int32_t symbol, int32_t end, T& value) const
    {
        value += rank(end, symbol);
    }

    template <typename T>
    void _add(int32_t symbol, int32_t start, int32_t end, T& value) const
    {
        value += rank(start, end, symbol);
    }



    template <typename T>
    void _sub(int32_t symbol, T& value) const
    {
        value -= rank(symbol);
    }

    template <typename T>
    void _sub(int32_t symbol, int32_t end, T& value) const
    {
        value -= rank(end, symbol);
    }

    template <typename T>
    void _sub(int32_t symbol, int32_t start, int32_t end, T& value) const
    {
        value -= rank(start, end, symbol);
    }


    int32_t get_values(int32_t idx) const
    {
        MEMORIA_V1_ASSERT(idx , <, size());
        return tools().get(symbols(), idx);
    }

    bool test(int32_t idx, int32_t symbol) const
    {
        MEMORIA_V1_ASSERT(idx , <, size());
        return iterator(idx).symbol() == symbol;
    }

    int32_t rank(int32_t symbol) const
    {
        if (has_index())
        {
            const auto* index = this->sum_index();
            return index->sum(symbol);
        }
        else {
            return rank(size(), symbol);
        }
    }

    int32_t rank(int32_t start, int32_t end, int32_t symbol) const
    {
        int32_t rank_start  = rank(start, symbol);
        int32_t rank_end    = rank(end, symbol);

        return rank_end - rank_start;
    }

    uint64_t rank(int32_t end, int32_t symbol) const
    {
        auto meta = this->metadata();
        int32_t size = meta->size();

        MEMORIA_V1_ASSERT_TRUE(end >= 0);
        MEMORIA_V1_ASSERT_TRUE(symbol >= 0 && symbol < Symbols);

        if (has_index())
        {
            const SumIndex* sum_index  = this->sum_index();

            if (end < size)
            {
                auto location       = find_run(end);
                auto block          = location.data_pos() / ValuesPerBranch;
                auto block_start    = block * ValuesPerBranch;

                auto rank_base  = sum_index->sum(symbol, block);

                auto block_offset = offsets()[block];

                auto local_rank = block_rank(meta, block_start + block_offset, end - location.block_base(), symbol);

                return local_rank + rank_base;
            }
            else {
                return sum_index->sum(symbol);
            }
        }
        else {
            return block_rank(meta, 0, end, symbol);
        }
    }

    SelectResult selectFW(uint64_t rank, int32_t symbol) const
    {
        auto meta    = this->metadata();
        auto symbols = this->symbols();

        MEMORIA_V1_ASSERT_TRUE(symbol >= 0 && symbol < Symbols);

        if (has_index())
        {
            const SumIndex* sum_index   = this->sum_index();
            auto find_result            = sum_index->find_ge(symbol, rank);
            int32_t blocks                  = sum_index->size();

            if (find_result.local_pos() < blocks)
            {
                auto block_start   = find_result.local_pos() * ValuesPerBranch;
                auto block_offset  = offsets()[find_result.local_pos()];
                uint64_t local_rank = rank - find_result.prefix();

                auto block_size_start  = this->size_index()->sum(0, find_result.local_pos());

                auto result = block_select(meta, symbols, block_start + block_offset, local_rank, block_size_start, symbol);

                result.rank() += find_result.prefix();

                return result;
            }
            else {
                return SelectResult(meta->size(), find_result.prefix(), false);
            }
        }
        else {
            return block_select(meta, symbols, 0, rank, 0, symbol);
        }
    }

    SelectResult selectBW(uint64_t rank, int32_t symbol) const
    {
        return selectBW(size(), rank, symbol);
    }


    SelectResult selectFW(int32_t pos, uint64_t rank, int32_t symbol) const
    {
        MEMORIA_V1_ASSERT(rank, >=, 1);
        MEMORIA_V1_ASSERT(pos, >=, -1);

        auto meta = this->metadata();

        if (pos < meta->size())
        {
            uint64_t rank_prefix = this->rank(pos + 1, symbol);
            auto result = selectFW(rank_prefix + rank, symbol);

            result.rank() -= rank_prefix;

            return result;
        }
        else {
            return SelectResult(meta->size(), 0, false);
        }
    }

    SelectResult selectBW(int32_t pos, uint64_t rank, int32_t symbol) const
    {
        auto meta = this->metadata();

        MEMORIA_V1_ASSERT(rank, >=, 1);
        MEMORIA_V1_ASSERT(pos, >=, 0);
        MEMORIA_V1_ASSERT(pos, <=, meta->size());

        uint64_t rank_prefix = this->rank(pos, symbol);
        if (rank_prefix >= rank)
        {
            auto target_rank = rank_prefix - (rank - 1);
            auto result = selectFW(target_rank, symbol);

            if (result.is_found())
            {
                result.rank() = rank;
            }
            else {
                result.rank() = rank_prefix;
            }

            return result;
        }
        else {
            return SelectResult(0, rank_prefix, false);
        }
    }


    SelectResult selectGEFW(uint64_t rank, int32_t symbol) const
    {
        auto meta    = this->metadata();
        auto symbols = this->symbols();

        MEMORIA_V1_ASSERT_TRUE(symbol >= 0 && symbol < Symbols);


        return block_select_ge(meta, symbols, 0, rank, 0, symbol);
    }

    SelectResult selectGEFW(int32_t pos, uint64_t rank, int32_t symbol) const
    {
        auto meta    = this->metadata();
        auto symbols = this->symbols();

        MEMORIA_V1_ASSERT_TRUE(symbol >= 0 && symbol < Symbols);

        auto location = find_run(pos);

        if (location.run().symbol() == symbol)
        {
        	rank += location.local_idx() + 1;

        	auto result = block_select_ge(meta, symbols, location.data_pos(), rank, location.block_base() + location.run_base(), symbol);

        	result.rank() -= (location.local_idx() + 1);
        	return result;
        }
        else {
        	return block_select_ge(meta, symbols, location.data_pos(), rank, location.block_base() + location.run_base(), symbol);
        }
    }



    rleseq::CountResult countFW(int32_t start_pos) const
    {
        auto location = find_run(start_pos);
        return block_count_fw(metadata(), symbols(), location);
    }

    rleseq::CountResult countBW(int32_t start_pos) const
    {
        MEMORIA_V1_ASSERT(start_pos, >=, 0);
        return block_count_bw(metadata(), symbols(), start_pos);
    }


    void dump(std::ostream& out = std::cout, bool dump_index = true) const
    {
        TextBlockDumper dumper(out);
        generateDataEvents(&dumper);
    }


    void generateDataEvents(IBlockDataEventHandler* handler) const
    {
        handler->startGroup("PACKED_RLE_SEQUENCE");
        auto meta = this->metadata();

        handler->value("SIZE", &meta->size());
        handler->value("DATA_SIZE", &meta->data_size());

        if (has_index())
        {
            handler->startGroup("INDEXES");
            size_index()->generateDataEvents(handler);
            sum_index()->generateDataEvents(handler);
            handler->endGroup();
        }

        handler->value("OFFSETS", BlockValueProviderFactory::provider(true, number_of_offsets(), [&](int32_t idx) {
            return offsets()[idx];
        }));

        handler->startGroup("SYMBOL RUNS", size());

        auto iter = this->iterator(0);

        int64_t values[4] = {0, 0, 0};

        while (iter.has_data())
        {
            values[1] = iter.run().symbol();
            values[2] = iter.run().length();

            handler->value("RUN", values, 3);

            values[0] += iter.run().length();

            iter.next_run();
        }

        handler->endGroup();

        handler->endGroup();
    }

    template <typename SerializationData>
    void serialize(SerializationData& buf) const
    {
        Base::serialize(buf);

        const Metadata* meta = this->metadata();

        FieldFactory<int32_t>::serialize(buf, meta->size());
        FieldFactory<int32_t>::serialize(buf, meta->data_size());

        FieldFactory<OffsetsType>::serialize(buf, offsets(), number_of_offsets());

        if (has_index())
        {
            size_index()->serialize(buf);
            sum_index()->serialize(buf);
        }

        FieldFactory<Value>::serialize(buf, symbols(), meta->data_size());
    }


    template <typename DeserializationData>
    void deserialize(DeserializationData& buf)
    {
        Base::deserialize(buf);

        Metadata* meta = this->metadata();

        FieldFactory<int32_t>::deserialize(buf, meta->size());
        FieldFactory<int32_t>::deserialize(buf, meta->data_size());

        FieldFactory<OffsetsType>::deserialize(buf, offsets(), number_of_offsets());

        if (has_index()) {
            size_index()->deserialize(buf);
            sum_index()->deserialize(buf);
        }

        FieldFactory<Value>::deserialize(buf, symbols(), meta->data_size());
    }


    auto find_run(int32_t symbol_pos) const
    {
        return find_run(this->metadata(), symbol_pos);
    }

    OpStatus insert_io_substream(int32_t at, const io::IOSubstream& substream, int32_t start, int32_t size)
    {
        const MyType* buffer = T2T<const MyType*>(io::substream_cast<io::IOSymbolSequence>(substream).buffer());
        return this->insert_from(at, buffer, start, size);
    }


    void configure_io_substream(io::IOSubstream& substream)
    {
        auto& seq = io::substream_cast<IOSubstreamView>(substream);
        seq.configure(this);
    }
    
private:
    
    auto find_run(const Metadata* meta, int32_t symbol_pos) const
    {
        if (symbol_pos >= 0)
        {
            if (symbol_pos < meta->size())
            {
                if (!has_index())
                {
                    return locate_run(meta, 0, symbol_pos, 0);
                }
                else
                {
                    auto find_result = this->size_index()->find_gt(0, symbol_pos);

                    int32_t local_pos   = symbol_pos - find_result.prefix();
                    size_t block_offset = find_result.local_pos() * ValuesPerBranch;
                    auto offset         = offsets()[find_result.local_pos()];

                    block_offset += offset;

                    return locate_run(meta, block_offset, local_pos, find_result.prefix());
                }
            }
            else {
                return Location(meta->data_size(), 0, 0, meta->data_size(), symbol_pos, RLESymbolsRun(), true);
            }
        }
        else {
            MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Symbol index must be >= 0: {}", symbol_pos));
        }
    }
    

    auto find_last_run(const Metadata* meta) const
    {
        if (!has_index())
        {
            return locate_last_run(meta, 0, 0);
        }
        else
        {
            auto size_index     = this->size_index();
            int32_t block_idx   = size_index->size() - 1;
            auto prefix         = size_index->sum(0, block_idx);

            
            size_t block_offset = block_idx * ValuesPerBranch;
            auto offset         = offsets()[block_idx];

            block_offset += offset;

            return locate_last_run(meta, block_offset, prefix);
        }
    }
    
public:    
    

    OpStatus compactify()
    {
        if (isFail(compactify_runs())) {
            return OpStatus::FAIL;
        }

        return reindex();
    }

private:
    auto select_fw_is(Iterator iter, int32_t symbol, uint64_t rank) const
    {
        MEMORIA_V1_ASSERT(rank, >=, 1);

        uint64_t cnt = 0;

        while (iter.has_data())
        {
            if (iter.symbol() == symbol) {
                if (++cnt == rank)
                {
                    return iter;
                }
            }
        }

        return end();
    }


    size_t block_rank(const Metadata* meta, size_t data_pos, size_t idx, int32_t symbol) const
    {
        Codec codec;
        size_t data_size = meta->data_size();
        auto symbols = this->symbols();

        size_t run_base  = 0;
        uint64_t rank     = 0;

        while (data_pos < data_size)
        {
            uint64_t run_value = 0;
            auto len = codec.decode(symbols, run_value, data_pos);
            auto run = decode_run(run_value);

            auto run_length = run.length();

            if (run.symbol() == symbol)
            {
                if (idx >= run_base + run_length)
                {
                    rank += run_length;
                }
                else {
                    return rank + idx - run_base;
                }
            }
            else if (idx < run_base + run_length)
            {
                return rank;
            }

            run_base += run_length;
            data_pos += len;
        }

        return rank;
    }

    class BlockSelectResult {
        uint64_t rank_;
        size_t data_pos_;
        size_t local_idx_;
        size_t block_idx_;
    public:
        BlockSelectResult(uint64_t rank, size_t data_pos, size_t local_idx, size_t block_idx):
            rank_(rank), data_pos_(data_pos), local_idx_(local_idx), block_idx_(block_idx)
        {}

        uint64_t rank() const {return rank_;}
        size_t data_pos() const {return data_pos_;}
        size_t local_idx() const {return local_idx_;}
        size_t block_idx() const {return block_idx_;}
    };


    SelectResult block_select(const Metadata* meta, const Value* symbols, size_t data_pos, uint64_t rank, size_t block_size_prefix, int32_t symbol) const
    {
        Codec codec;
        size_t data_size = meta->data_size();

        uint64_t rank_base = 0;
        size_t  run_base  = 0;

        RLESymbolsRun run;

        while (data_pos < data_size)
        {
            uint64_t run_value = 0;
            auto len = codec.decode(symbols, run_value, data_pos);
            run = decode_run(run_value);

            auto run_length = run.length();

            if (run.symbol() == symbol)
            {
                if (rank > rank_base + run_length)
                {
                    rank_base += run_length;
                    run_base  += run_length;
                }
                else {
                    size_t local_idx   = rank - rank_base - 1;
                    return SelectResult(run_base + block_size_prefix + local_idx, rank_base + local_idx + 1, true);
                }
            }
            else {
                run_base += run_length;
            }

            data_pos += len;
        }

        return SelectResult(run_base + block_size_prefix, rank_base, false);
    }


    SelectResult block_select_ge(const Metadata* meta, const Value* symbols, size_t data_pos, uint64_t rank, size_t block_size_prefix, int32_t symbol) const
    {
        Codec codec;
        size_t data_size = meta->data_size();

        uint64_t rank_base = 0;
        size_t  run_base  = 0;

        RLESymbolsRun run;

        while (data_pos < data_size)
        {
            uint64_t run_value = 0;
            auto len = codec.decode(symbols, run_value, data_pos);
            run = decode_run(run_value);

            auto run_length = run.length();

            if (run.symbol() == symbol)
            {
                if (rank > rank_base + run_length)
                {
                    rank_base += run_length;
                    run_base  += run_length;
                }
                else {
                    size_t local_idx = rank - rank_base - 1;
                    return SelectResult(run_base + block_size_prefix + local_idx, rank_base + local_idx + 1, true);
                }
            }
            else if (run.symbol() < symbol)
            {
            	size_t local_idx = 0;
            	return SelectResult(run_base + block_size_prefix + local_idx, rank_base + local_idx + 1, true);
            }
            else {
                run_base += run_length;
            }

            data_pos += len;
        }

        return SelectResult(meta->size(), rank_base, false);
    }



    rleseq::CountResult block_count_fw(const Metadata* meta, const Value* symbols, const Location& location) const
    {
        size_t pos = location.data_pos();
        size_t data_size = meta->data_size();

        Codec codec;

        uint64_t local_pos = location.local_idx();

        uint64_t count = 0;
        int32_t last_symbol = -1;

        while (pos < data_size)
        {
            uint64_t run_value = 0;
            auto len = codec.decode(symbols, run_value, pos);
            auto run = decode_run(run_value);

            if (run.symbol() == last_symbol || last_symbol == -1)
            {
                pos += len;
                count += run.length() - local_pos;
                last_symbol = run.symbol();

                local_pos = 0;
            }
            else {
                return rleseq::CountResult(count, last_symbol);
            }
        }

        return rleseq::CountResult(count, last_symbol);
    }


    rleseq::CountResult block_count_bw(const Metadata* meta, const Value* symbols, uint64_t start_pos) const
    {
        size_t pos = 0;
        size_t data_size = meta->data_size();

        Codec codec;

        uint64_t count = 0;
        uint64_t run_base = 0;

        int32_t last_symbol = -1;

        while (pos < data_size)
        {
            uint64_t run_value = 0;
            auto len = codec.decode(symbols, run_value, pos);
            auto run = decode_run(run_value);

            if (run.symbol() != last_symbol) {
                count = 0;
            }

            if (start_pos >= run_base + run.length())
            {
                count += run.length();
                last_symbol = run.symbol();
            }
            else {
                return rleseq::CountResult(count + start_pos - run_base + 1, run.symbol());
            }

            run_base += run.length();
            pos += len;
        }

        MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"countBW start position is out of range: {} {}", start_pos, meta->size()));
    }



    OpStatus compactify_runs()
    {
        auto meta    = this->metadata();
        auto symbols = this->symbols();

        size_t pos = 0;
        size_t data_size = meta->data_size();

        Codec codec;

        while (pos < data_size)
        {
            size_t pos0 = pos;

            int32_t last_symbol = -1;
            int64_t total_length = 0;

            size_t runs;
            for (runs = 0; pos0 < data_size; runs++)
            {
                uint64_t run_value = 0;
                auto len = codec.decode(symbols, run_value, pos0);
                auto run = decode_run(run_value);

                if (run.symbol() == last_symbol || last_symbol == -1)
                {
                    if (total_length + run.length() <= MaxRunLength)
                    {
                        pos0 += len;
                        total_length += run.length();
                        last_symbol = run.symbol();
                    }
                    else {
                        break;
                    }
                }
                else {
                    break;
                }
            }


            if (runs > 1)
            {
                auto new_run_value          = encode_run(last_symbol, total_length);
                size_t new_run_value_length = codec.length(new_run_value);
                size_t current_length       = pos0 - pos;

                if (current_length > new_run_value_length)
                {
                    codec.move(symbols, pos0, pos + new_run_value_length, data_size - pos0);

                    codec.encode(symbols, new_run_value, pos);

                    data_size -= current_length - new_run_value_length;
                    pos += new_run_value_length;
                }
                else {
                    pos = pos0;
                }
            }
            else {
                pos = pos0;
            }
        }

        meta->data_size() = data_size;

        return shrink_to_data();
    }



    Location locate_run(const Metadata* meta, size_t pos, size_t idx, size_t size_base) const
    {
        Codec codec;
        auto symbols = this->symbols();

        size_t base = 0;

        size_t limit = meta->data_size();

        while (pos < limit)
        {
            uint64_t run_value = 0;
            auto len = codec.decode(symbols, run_value, pos);
            auto run = decode_run(run_value);

            auto run_length = run.length();

            if (idx >= base + run_length)
            {
                base += run_length;
                pos += len;
            }
            else {
                return Location(pos, len, idx - base, size_base, base, run);
            }
        }

        MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Symbol index is out of bounds: {} {}", idx, meta->size()));
    }
    
    
    Location locate_last_run(const Metadata* meta, size_t pos, size_t size_base) const
    {
        Codec codec;
        auto symbols = this->symbols();

        uint64_t base = 0;

        size_t limit = meta->data_size();

        RLESymbolsRun run;
        size_t len{};
        size_t last_pos{};
        uint64_t last_base;
        
        while (pos < limit)
        {
            last_pos  = pos;
            last_base = base;
            
            uint64_t run_value = 0;
            len = codec.decode(symbols, run_value, pos);
            run = decode_run(run_value);

            base += run.length();
            pos  += len;
        }
        
        return Location(last_pos, len, meta->size() - last_base, size_base, last_base, run, true);
    }



    OpStatus remove_runs(const Location& start, const Location& end)
    {
        if (start.data_pos() < end.data_pos())
        {
            auto meta = this->metadata();

            Codec codec;

            uint64_t new_start_run_value     = encode_run(start.run().symbol(), start.run_prefix());
            size_t new_start_run_length     = codec.length(new_start_run_value);

            uint64_t new_end_run_value       = encode_run(end.run().symbol(), end.run_suffix());
            size_t new_end_run_length       = codec.length(new_end_run_value);

            size_t hole_start   = start.data_pos() + new_start_run_length;
            size_t hole_end     = end.data_pos() + (end.data_length() - new_end_run_length);

            size_t to_remove    = hole_end - hole_start;

            auto symbols = this->symbols();

            codec.move(symbols, hole_end, hole_start, meta->data_size() - hole_end);

            codec.encode(symbols, new_start_run_value, start.data_pos());
            codec.encode(symbols, new_end_run_value, hole_start);

            meta->data_size() -= to_remove;
        }
        else {
            int64_t delta = end.local_idx() - start.local_idx();
            if (isFail(add_run_length0(start, -delta))) {
                return OpStatus::FAIL;
            }
        }

        return OpStatus::OK;
    }


    void remove_runs_one_sided_left(const Location& start, const Location& end)
    {
        Codec codec;

        auto meta = this->metadata();

        uint64_t new_end_run_value       = encode_run(end.run().symbol(), end.run_suffix());
        size_t new_end_run_length       = codec.length(new_end_run_value);

        size_t hole_start   = start.data_pos();
        size_t hole_end     = end.data_pos() + (end.data_length() - new_end_run_length);

        size_t to_remove    = hole_end - hole_start;

        auto symbols = this->symbols();

        codec.move(symbols, hole_end, hole_start, meta->data_size() - hole_end);

        codec.encode(symbols, new_end_run_value, hole_start);

        meta->data_size() -= to_remove;
    }


    void remove_runs_one_sided(const Location& start, size_t end)
    {
        Codec codec;

        auto meta = this->metadata();

        uint64_t new_run_value   = encode_run(start.run().symbol(), start.run_prefix());
        size_t new_run_length   = codec.length(new_run_value);

        size_t hole_start = start.data_pos() + new_run_length;
        size_t to_remove  = end - hole_start;

        auto symbols = this->symbols();

        codec.move(symbols, end, hole_start, meta->data_size() - end);

        codec.encode(symbols, new_run_value, start.data_pos());

        meta->data_size() -= to_remove;
    }

    void remove_runs_two_sided(size_t start, size_t end)
    {
        size_t to_remove  = end - start;

        auto symbols = this->symbols();

        auto meta = this->metadata();

        Codec codec;
        codec.move(symbols, end, start, meta->data_size() - end);

        meta->data_size() -= to_remove;
    }

    void try_merge_two_adjustent_runs(size_t run_pos)
    {
        auto meta    = this->metadata();
        auto symbols = this->symbols();

        Codec codec;

        uint64_t first_run_value = 0;
        auto first_len = codec.decode(symbols, first_run_value, run_pos);

        if (run_pos + first_len < meta->data_size())
        {
            uint64_t next_run_value = 0;
            auto next_len = codec.decode(symbols, next_run_value, run_pos + first_len);

            auto first_run  = decode_run(first_run_value);
            auto next_run   = decode_run(next_run_value);

            if (first_run.symbol() == next_run.symbol() && first_run.length() + next_run.length() <= MaxRunLength)
            {
                uint64_t new_run_value      = encode_run(first_run.symbol(), first_run.length() + next_run.length());
                size_t new_run_value_length = codec.length(new_run_value);

                size_t window_size = first_len + next_len;

                if (new_run_value_length <= window_size)
                {
                    size_t to_remove = window_size - new_run_value_length;

                    codec.move(symbols, run_pos + first_len + next_len, run_pos + new_run_value_length, meta->data_size() - (run_pos  + window_size));
                    codec.encode(symbols, new_run_value, run_pos);

                    meta->data_size() -= to_remove;
                }
            }
        }
    }

    MMA1_PKD_OOM_SAFE
    OpStatusT<Location> split_run(const Location& location, uint64_t subtraction = 0)
    {
        uint64_t pos = location.local_idx();

        if (pos > 0 && pos < location.run().length())
        {
            Codec codec;
            auto symbols = this->symbols();
            auto meta    = this->metadata();

            uint64_t prefix_run_value    = encode_run(location.run().symbol(), location.run_prefix());
            size_t  prefix_value_length = codec.length(prefix_run_value);

            uint64_t suffix_run_value = encode_run(location.run().symbol(), location.run_suffix() - subtraction);
            size_t  suffix_value_length = codec.length(suffix_run_value);

            size_t total_length = prefix_value_length + suffix_value_length;

            if (total_length >= location.data_length())
            {
                size_t delta = total_length - location.data_length();

                if (delta > 0)
                {
                    if (isFail(ensure_capacity(delta))) {
                        return OpStatus::FAIL;
                    }

                    codec.move(symbols, location.data_end(), location.data_end() + delta, meta->data_size() - location.data_end());
                }

                auto pos_tmp = location.data_pos();
                pos_tmp += codec.encode(symbols, prefix_run_value, pos_tmp);
                codec.encode(symbols, suffix_run_value, pos_tmp);

                meta->data_size() += delta;
                meta->size() -= subtraction;
            }
            else {
                size_t delta = location.data_length() - total_length;

                codec.move(symbols, location.data_end(), location.data_end() - delta, meta->data_size() - location.data_end());

                auto pos_tmp = location.data_pos();
                pos_tmp += codec.encode(symbols, prefix_run_value, pos_tmp);
                codec.encode(symbols, suffix_run_value, pos_tmp);

                meta->data_size() -= delta;
                meta->size() -= subtraction;

                if (isFail(shrink_to_data())) {
                    return OpStatus::FAIL;
                }
            }

            return OpStatusT<Location>(Location(
                    location.data_pos() + prefix_value_length,
                    suffix_value_length,
                    0,
                    location.block_base(),
                    location.run_base() + location.run_prefix(),
                    RLESymbolsRun(location.run().symbol(), location.run_suffix())
            ));
        }
        else {
            MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"split_run: invalid split position: {} {}", pos, location.run().length()));
        }
    }

    OpStatusT<size_t> add_run_length0(const Location& location, int64_t length)
    {
        Codec codec;
        auto meta = this->metadata();

        uint64_t run_value      = encode_run(location.run().symbol(), location.run().length() + length);
        size_t run_value_length = codec.length(run_value);

        if(isFail(ensure_capacity(run_value_length))) {
            return OpStatus::FAIL;
        }

        auto symbols = this->symbols();

        codec.move(symbols, location.data_end(), location.data_pos() + run_value_length, meta->data_size() - location.data_end());
        codec.encode(symbols, run_value, location.data_pos());

        meta->data_size() += (run_value_length - location.data_length());

        return OpStatusT<size_t>(run_value_length);
    }

    OpStatusT<size_t> add_run_length(const Location& location, int64_t length)
    {
        auto run_value_length_s = add_run_length0(location, length);

        if(isFail(run_value_length_s)) {
            return OpStatus::FAIL;
        }

        auto meta = this->metadata();
        meta->size() += length;

        return run_value_length_s;
    }

    OpStatus remove_run(const Location& location)
    {
        Codec codec;
        auto meta = this->metadata();

        auto symbols = this->symbols();

        codec.move(symbols, location.data_end(), location.data_pos(), meta->data_size() - location.data_end());

        meta->data_size() -= location.data_length();
        meta->size() -= location.run().length();

        return shrink_to_data();
    }

    OpStatusT<size_t> insert_run(const size_t& location, int32_t symbol, uint64_t length)
    {
        Codec codec;
        auto symbols = this->symbols();
        auto meta    = this->metadata();

        uint64_t run_value       = encode_run(symbol, length);
        size_t run_value_length  = codec.length(run_value);

        if (isFail(ensure_capacity(run_value_length))) {
            return OpStatus::FAIL;
        }

        codec.move(symbols, location, location + run_value_length, meta->data_size() - location);
        codec.encode(symbols, run_value, location);

        meta->data_size() += run_value_length;
        meta->size() += length;

        return OpStatusT<size_t>(run_value_length);
    }
};

template <typename T>
struct StructSizeProvider<PkdRLESeq<T>> {
    static const int32_t Value = PkdRLESeq<T>::Indexes;
};



}}
