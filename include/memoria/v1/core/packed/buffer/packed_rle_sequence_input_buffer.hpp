
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

#include <memoria/v1/core/packed/tools/packed_allocator.hpp>
#include <memoria/v1/core/packed/tree/fse/packed_fse_quick_tree.hpp>

#include <memoria/v1/core/tools/static_array.hpp>
#include <memoria/v1/core/tools/optional.hpp>
#include <memoria/v1/core/bignum/primitive_codec.hpp>

#include <memoria/v1/metadata/page.hpp>

#include "rleseq/rleseqbuffer_reindex_fn.hpp"
#include <memoria/v1/core/packed/sseq/rleseq/rleseq_iterator.hpp>

#include <ostream>

namespace memoria {
namespace v1 {

using rleseq::RLESymbolsRun;
using rleseq::Location;


template <typename Types_>
class PkdRLESeqInputBuffer: public PackedAllocator {

    using Base = PackedAllocator;

public:
    static constexpr uint32_t VERSION = 1;

    using Types  = Types_;
    using MyType = PkdRLESeqInputBuffer<Types_>;

    static constexpr int32_t ValuesPerBranch        = Types::ValuesPerBranch;
    static constexpr int32_t ValuesPerBranchLog2    = NumberOfBits(Types::ValuesPerBranch);

    static constexpr int32_t Indexes                = Types::Blocks;
    static constexpr int32_t Symbols                = Types::Blocks;

    static constexpr size_t MaxRunLength        = MaxRLERunLength;

    enum {
        METADATA, SIZE_INDEX, OFFSETS, SYMBOLS, TOTAL_SEGMENTS__
    };

    using Base::clear;

    using Value = uint8_t;

    using Codec = ValueCodec<uint64_t>;

    using Values = core::StaticVector<int64_t, Indexes>;

    using SizeIndex = PkdFQTreeT<int64_t, 1>;

    using Iterator = rleseq::RLESeqIterator<MyType>;

    using InputType = Value;
    using OffsetsType = uint8_t;

    struct AppendState {};

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
        return PackedAllocator::roundUpBytesToAlignmentBlocks(number_of_offsets(values) * sizeof(OffsetsType));
    }

    static constexpr size_t divUp(size_t value, size_t divisor) {
        return (value / divisor) + ((value % divisor) ? 1 : 0);
    }


public:
    PkdRLESeqInputBuffer() = default;

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

    int32_t data_capacity() const
    {
        return symbols_block_size();
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

    int32_t get_symbol(int32_t idx) const
    {
        auto result = this->find_run(idx);

        if (!result.out_of_range())
        {
            return result.symbol();
        }
        else {
            throw Exception(MA_SRC, SBuf() << "Symbol index " << idx << " is out of range " << this->size());
        }
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


    void append(int32_t symbol, uint64_t length)
    {
        MEMORIA_V1_ASSERT_TRUE(symbol >= 0 && symbol < Symbols);

        auto meta = this->metadata();

        auto run_value = encode_run(symbol, length);

        Codec codec;
        meta->data_size() += codec.encode(this->symbols(), run_value, meta->data_size());
        meta->size() += length;
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

    bool has_capacity(int32_t required_capacity) const
    {
        int32_t current_capacity = this->symbols_block_capacity();
        return current_capacity >= required_capacity;
    }


public:

    // ===================================== Allocation ================================= //

    void init(const SizesT& symbols_capacity)
    {
        init(block_size(symbols_capacity[0]), symbols_capacity);
    }


    void init(int32_t block_size, const SizesT& symbols_capacity)
    {
        Base::init(block_size, TOTAL_SEGMENTS__);

        int32_t capacity = symbols_capacity[0];

        Metadata* meta  = Base::template allocate<Metadata>(METADATA);

        meta->size()      = 0;
        meta->data_size() = 0;

        this->template allocateArrayBySize<OffsetsType>(OFFSETS, number_of_offsets(capacity));
        this->template allocateArrayBySize<OffsetsType>(SYMBOLS, capacity);

        int32_t index_size = number_of_indexes(capacity);

        if (index_size > 0)
        {
            int32_t size_index_block_size = SizeIndex::block_size(index_size);
            Base::resizeBlock(SIZE_INDEX, size_index_block_size);

            auto size_index = this->size_index();
            size_index->init_by_block(size_index_block_size, index_size);
        }
        else {
            Base::setBlockType(SIZE_INDEX, PackedBlockType::ALLOCATABLE);
        }
    }

    static int32_t block_size(const SizesT& symbols_capacity) {
        return block_size(symbols_capacity[0]);
    }

    static int32_t block_size(int32_t symbols_capacity)
    {
        int32_t metadata_length     = Base::roundUpBytesToAlignmentBlocks(sizeof(Metadata));

        int32_t capacity = symbols_capacity;

        int32_t index_size = number_of_indexes(capacity);

        int32_t size_index_length   = index_size > 0 ? SizeIndex::block_size(index_size) : 0;
        int32_t offsets_length      = offsets_segment_size(capacity);
        int32_t values_length       = Base::roundUpBytesToAlignmentBlocks(capacity);

        int32_t block_size          = Base::block_size(metadata_length + size_index_length  + offsets_length + values_length, TOTAL_SEGMENTS__);
        return block_size;
    }


    void clear_index()
    {
        if (has_index())
        {
            int32_t capacity    = element_size(SYMBOLS);
            int32_t index_size  = number_of_indexes(capacity);

            Base::clear(SIZE_INDEX);

            auto size_index = this->size_index();

            int32_t size_index_block_size = SizeIndex::block_size(index_size);
            size_index->init_by_block(size_index_block_size, index_size);
        }
    }


    void copyTo(MyType* other) const
    {
        auto meta = this->metadata();
        auto other_meta = other->metadata();

        other_meta->size()      = meta->size();
        other_meta->data_size() = meta->data_size();

        Codec codec;
        codec.copy(this->symbols(), 0, other->symbols(), 0, meta->data_size());
    }



public:


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
                auto find_result    = this->size_index()->find_gt(0, symbol_pos);

                int32_t local_pos       = symbol_pos - find_result.prefix();
                size_t block_offset = find_result.idx() * ValuesPerBranch;
                auto offset         = offsets()[find_result.idx()];

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

    void reindex()
    {
        rleseq::buffer::ReindexFn<MyType> reindex_fn;
        reindex_fn.reindex(*this);
    }

    void check() const
    {
        if (has_index())
        {
            size_index()->check();

            rleseq::buffer::ReindexFn<MyType> reindex_fn;
            reindex_fn.check(*this);
        }
    }

    void clear()
    {
        auto meta = this->metadata();
        meta->size() = 0;
        meta->data_size() = 0;
    }

    void reset() {clear();}


    void dump(std::ostream& out = std::cout, bool dump_index = true) const
    {
        TextPageDumper dumper(out);

        generateDataEvents(&dumper);
    }

    void generateDataEvents(IPageDataEventHandler* handler) const
    {
        handler->startGroup("PACKED_RLE_SEQUENCE");
        auto meta = this->metadata();

        handler->value("SIZE", &meta->size());
        handler->value("DATA_SIZE", &meta->data_size());

        if (has_index())
        {
            handler->startGroup("INDEXES");
            size_index()->generateDataEvents(handler);
            handler->endGroup();
        }

        handler->value("OFFSETS", PageValueProviderFactory::provider(true, number_of_offsets(), [&](int32_t idx) {
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



    auto find_run(int32_t symbol_pos) const
    {
        if (symbol_pos >= 0)
        {
            auto meta = this->metadata();

            if (symbol_pos < meta->size())
            {
                if (!has_index())
                {
                    return locate_run(meta, 0, symbol_pos, 0);
                }
                else
                {
                    auto find_result = this->size_index()->find_gt(0, symbol_pos);

                    int32_t local_pos       = symbol_pos - find_result.prefix();
                    size_t block_offset = find_result.idx() * ValuesPerBranch;
                    auto offset         = offsets()[find_result.idx()];

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

    void compactify()
    {
        compactify_runs();
        reindex();
    }

private:

    void compactify_runs()
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

};




}}
