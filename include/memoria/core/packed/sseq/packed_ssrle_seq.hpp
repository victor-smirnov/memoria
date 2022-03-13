
// Copyright 2021-2022 Victor Smirnov
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

#include <memoria/core/memory/ptr_cast.hpp>
#include <memoria/core/ssrle/ssrle.hpp>

#include <memoria/core/tools/static_array.hpp>
#include <memoria/core/tools/optional.hpp>
#include <memoria/core/types/algo/select.hpp>

#include <memoria/profiles/common/block_operations.hpp>

#include <memoria/core/iovector/io_substream_ssrle.hpp>
#include <memoria/core/packed/sseq/packed_ssrle_seq_so.hpp>

#include <memoria/core/packed/tools/packed_allocator.hpp>
#include <memoria/core/packed/datatype_buffer/packed_datatype_buffer.hpp>

#include <memoria/core/integer/accumulator_common.hpp>

#include "ssrleseq/ssrleseq_reindex_fn.hpp"
#include "ssrleseq/ssrleseq_iterator.hpp"


#include <ostream>

namespace memoria {

namespace io {

template <size_t AlphabetSize> class IOSSRLEBufferImpl;
template <size_t AlphabetSize> class IOSSRLEBufferView;

}


template <
    size_t AlphabetSize_,
    size_t BytesPerBlock_,
    bool Use64BitSize_ = false
>
struct PkdSSRLSeqTypes {
    static constexpr size_t AlphabetSize    = AlphabetSize_;
    static constexpr size_t BytesPerBlock   = BytesPerBlock_;
    static constexpr bool Use64BitSize      = Use64BitSize_;
};

template <typename Types> class PkdSSRLESeq;

template <
    size_t AlphabetSize,
    size_t BytesPerBlock = 256,
    bool Use64BitSize = false
>
using PkdSSRLESeqT = PkdSSRLESeq<PkdSSRLSeqTypes<AlphabetSize, BytesPerBlock, Use64BitSize>>;

template <typename Types_>
class PkdSSRLESeq: public PackedAllocator {

    using Base = PackedAllocator;

    template <typename, typename>
    friend class PackedSSRLESeqSO;

public:
    static constexpr uint32_t VERSION = 1;

    using Types  = Types_;
    using MyType = PkdSSRLESeq<Types_>;

    static constexpr PkdSearchType KeySearchType        = PkdSearchType::SUM;
    static constexpr PkdSearchType SearchType           = PkdSearchType::SUM;
    static constexpr PackedDataTypeSize SizeType        = PackedDataTypeSize::VARIABLE;


    static constexpr size_t AlphabetSize    = Types::AlphabetSize;
    static constexpr size_t BitsPerSymbol   = BitsPerSymbolConstexpr(AlphabetSize);

    static_assert(AlphabetSize >= 2 && AlphabetSize <= 256, "AlphabetSize must be in [2, ... 256]");

    using SymbolsRunT = SSRLERun<BitsPerSymbol>;
    using RunTraits   = SSRLERunTraits<BitsPerSymbol>;
    using CodeUnitT   = typename RunTraits::CodeUnitT;
    using SymbolT     = typename RunTraits::SymbolT;
    using RunSizeT    = typename RunTraits::RunSizeT;
    using SeqSizeT    = IfThenElse<Types::Use64BitSize, uint64_t, UAcc128T>;

    using Iterator = ssrleseq::SymbolsRunIterator<MyType>;

    static constexpr size_t BytesPerBlockBase          = Types::BytesPerBlock;
    static constexpr size_t BytesPerBlock              = BytesPerBlockBase * BitsPerSymbol;
    static constexpr size_t AtomsPerBlock              = BytesPerBlock / sizeof(typename RunTraits::CodeUnitT);
    static constexpr size_t SegmentSizeInAtoms         = RunTraits::SEGMENT_SIZE_UNITS;

    static_assert(BytesPerBlock % 8 == 0, "Must be multiple of 8 bytes");

    enum {
        METADATA, SIZE_INDEX, SUM_INDEX, SYMBOLS, TOTAL_SEGMENTS_
    };

    using Base::clear;

    using SumValueT  = SeqSizeT;
    using SizeValueT = SeqSizeT;

    using SumIndex    = PackedDataTypeBufferT<SumValueT, true, AlphabetSize, DTOrdering::SUM>;
    using SizeIndex   = PackedDataTypeBufferT<SizeValueT, true, 1, DTOrdering::SUM>;

    using SumIndexSO  = typename SumIndex::SparseObject;
    using SizeIndexSO = typename SizeIndex::SparseObject;

    class Metadata {
        SeqSizeT size_;
        uint64_t code_units_;
        psize_t flags_;
    public:
        const SeqSizeT& size() const        {return size_;}
        void set_size(const SeqSizeT& val)  {size_ = val;}

        void add_size(const SeqSizeT& val) {size_ += val;}
        void sub_size(const SeqSizeT& val) {size_ -= val;}

        const uint64_t& code_units() const  {return code_units_;}
        void set_code_units(uint64_t val)   {code_units_ = val;}

        psize_t flags() const {return flags_;}
        void set_flags(psize_t flags) {flags_ = flags;}

        SeqSizeT& size_mut()        {return size_;}
        uint64_t& code_units_mut()  {return code_units_;}

        const psize_t& flags_imm() const  {return flags_;}
        psize_t& flags_mut()        {return flags_;}
    };

    struct Tools {};

    using GrowableIOSubstream = io::IOSSRLEBufferImpl<AlphabetSize>;
    using IOSubstreamView     = io::IOSSRLEBufferView<AlphabetSize>;

    using ExtData = std::tuple<>;
    using SparseObject = PackedSSRLESeqSO<ExtData, MyType>;


    static constexpr size_t number_of_indexes(size_t capacity) {
        return capacity <= BytesPerBlock ? 0 : div_up(capacity, BytesPerBlock);
    }

    static constexpr size_t div_up(size_t value, size_t divisor) {
        return (value / divisor) + ((value % divisor) ? 1 : 0);
    }

public:
    PkdSSRLESeq() = default;

    const SeqSizeT& size() const noexcept {return metadata()->size();}
    uint64_t code_units() noexcept {return metadata()->code_units();}

    // FIXME: Make setters private/protected

    // ====================================== Accessors ================================= //

    const Metadata* metadata() const noexcept {
        return Base::template get<Metadata>(METADATA);
    }

    Metadata* metadata() noexcept {
        return Base::template get<Metadata>(METADATA);
    }

    bool has_sum_index() const noexcept {
        return element_size(SUM_INDEX) > 0;
    }

    SumIndex* sum_index() noexcept {
        return Base::template get<SumIndex>(SUM_INDEX);
    }

    const SumIndex* sum_index() const noexcept {
        return Base::template get<SumIndex>(SUM_INDEX);
    }

    bool has_size_index() const noexcept {
        return element_size(SIZE_INDEX) > 0;
    }

    SizeIndex* size_index() noexcept {
        return Base::template get<SizeIndex>(SIZE_INDEX);
    }

    const SizeIndex* size_index() const noexcept {
        return Base::template get<SizeIndex>(SIZE_INDEX);
    }

    Span<CodeUnitT> symbols() noexcept {
        return span<CodeUnitT>(this, SYMBOLS);
    }

    Span<const CodeUnitT> symbols() const noexcept {
        return span<CodeUnitT>(this, SYMBOLS);
    }

    size_t symbols_block_capacity() const noexcept {
        return symbols_block_capacity(metadata());
    }

    size_t symbols_block_capacity(const Metadata* meta) const noexcept {
        return symbols_block_size() - meta->code_units();
    }

    size_t symbols_block_size() const noexcept {
        return this->element_size(SYMBOLS);
    }

    bool has_index() const noexcept {
        return has_size_index() && has_sum_index();
    }

    Tools tools() const noexcept {
        return Tools();
    }

    Iterator iterator() const noexcept {
        return Iterator(span<CodeUnitT>(this, SYMBOLS));
    }

    static Iterator iterator_from(Span<const CodeUnitT> units) {
        return Iterator(units);
    }

    Iterator iterator(size_t atom_idx) const noexcept {
        auto syms = span<CodeUnitT>(this, SYMBOLS);
        return Iterator(syms, atom_idx);
    }

    Iterator block_iterator(size_t block_num) const noexcept {
        auto syms = span<CodeUnitT>(this, SYMBOLS);
        return Iterator(syms, block_num * AtomsPerBlock);
    }


public:

    // ===================================== Allocation ================================= //

    using PackedAllocator::block_size;

    size_t block_size(const MyType* other) const {
        return block_size(this->symbols_block_size() + other->symbols_block_size());
    }

    static size_t block_size(size_t symbols_block_capacity)
    {
        size_t metadata_length = PackedAllocatable::round_up_bytes_to_alignment_blocks(sizeof(Metadata));

        size_t symbols_block_capacity_aligned = PackedAllocatable::round_up_bytes_to_alignment_blocks(
                    symbols_block_capacity
        );

        size_t index_size = number_of_indexes(symbols_block_capacity_aligned);

        size_t size_index_length{};
        size_t sum_index_length{};

        if (index_size > 0) {
            size_index_length   = SizeIndex::packed_block_size(index_size);
            sum_index_length    = SumIndex::packed_block_size(index_size);
        }

        size_t block_size          = Base::block_size(
                    metadata_length
                    + size_index_length
                    + sum_index_length
                    + symbols_block_capacity_aligned,
                    TOTAL_SEGMENTS_);

        return block_size;
    }

    VoidResult init(size_t block_size)
    {
        MEMORIA_ASSERT_RTN(block_size, >=, empty_size());
        return init();
    }

    VoidResult init() {
        return init_bs(empty_size());
    }

    VoidResult init_bs(size_t block_size)
    {
        MEMORIA_TRY_VOID(Base::init(block_size, TOTAL_SEGMENTS_));

        MEMORIA_TRY(meta, Base::template allocate<Metadata>(METADATA));

        meta->set_size(SeqSizeT{});
        meta->set_code_units(0);

        Base::set_block_type(SIZE_INDEX, PackedBlockType::ALLOCATABLE);
        Base::set_block_type(SUM_INDEX,  PackedBlockType::ALLOCATABLE);
        Base::set_block_type(SYMBOLS, PackedBlockType::RAW_MEMORY);

        return VoidResult::of();
    }

    VoidResult clear()
    {
        MEMORIA_TRY_VOID(Base::resize_block(SYMBOLS, 0));

        MEMORIA_TRY_VOID(removeIndex());

        auto meta = this->metadata();

        meta->set_size(SeqSizeT{});
        meta->set_code_units(0);

        return VoidResult::of();
    }

    void reset() {
        auto meta = this->metadata();

        meta->set_size(SeqSizeT{});
        meta->set_code_units(0);
    }


public:
    static constexpr size_t default_size(size_t available_space)
    {
        return empty_size();
    }

    VoidResult init_default(size_t block_size) {
        return init();
    }


    static size_t empty_size()
    {
        size_t metadata_length     = PackedAllocatable::round_up_bytes_to_alignment_blocks(sizeof(Metadata));
        size_t size_index_length   = 0;
        size_t sum_index_length    = 0;
        size_t symbols_block_capacity = 0;

        size_t block_size          = Base::block_size(
                    metadata_length
                    + size_index_length
                    + sum_index_length
                    + symbols_block_capacity,
                    TOTAL_SEGMENTS_);

        return block_size;
    }


    VoidResult removeIndex()
    {
        MEMORIA_TRY_VOID(Base::free(SIZE_INDEX));
        MEMORIA_TRY_VOID(Base::free(SUM_INDEX));

        return VoidResult::of();
    }


    VoidResult createIndex(size_t capacity) {
        MEMORIA_TRY_VOID(allocate_empty<SizeIndex>(SIZE_INDEX));
        MEMORIA_TRY_VOID(allocate_empty<SumIndex>(SUM_INDEX));
        return VoidResult::of();
    }

    // ========================================= Update ================================= //

private:

public:

    VoidResult ensure_capacity(size_t capacity)
    {
        size_t current_capacity = this->symbols_block_capacity();

        if (current_capacity < capacity) {
            return enlargeData(capacity - current_capacity);
        }

        return VoidResult::of();
    }

    bool has_capacity(size_t required_capacity) const
    {
        size_t current_capacity = this->symbols_block_capacity();
        return current_capacity >= required_capacity;
    }


    VoidResult enlargeData(size_t length)
    {
        size_t new_size = this->element_size(SYMBOLS) + length;
        MEMORIA_TRY_VOID(Base::resize_block(SYMBOLS, new_size));
        return VoidResult::of();
    }


public:




    template <typename SerializationData>
    void serialize(SerializationData& buf) const
    {
        Base::serialize(buf);

        const Metadata* meta = this->metadata();

        FieldFactory<SeqSizeT>::serialize(buf, meta->size());
        FieldFactory<uint64_t>::serialize(buf, meta->code_units());
        FieldFactory<psize_t>::serialize(buf, meta->flags_imm());

        if (has_size_index()){
            size_index()->serialize(buf);
        }

        if (has_sum_index()){
            sum_index()->serialize(buf);
        }

        FieldFactory<CodeUnitT>::serialize(buf, symbols().data(), meta->code_units());
    }


    template <typename DeserializationData>
    void deserialize(DeserializationData& buf)
    {
        Base::deserialize(buf);

        Metadata* meta = this->metadata();

        FieldFactory<SeqSizeT>::deserialize(buf, meta->size_mut());
        FieldFactory<uint64_t>::deserialize(buf, meta->code_units_mut());
        FieldFactory<psize_t>::deserialize(buf, meta->flags_mut());

        if (has_size_index()) {
            size_index()->deserialize(buf);
        }

        if (has_sum_index()) {
            sum_index()->deserialize(buf);
        }

        FieldFactory<CodeUnitT>::deserialize(buf, symbols().data(), meta->code_units());
    }

    void configure_io_substream(io::IOSubstream& substream) const
    {
        auto& seq = io::substream_cast<IOSubstreamView>(substream);
        seq.configure(this);
    }
};


template <typename Types>
struct PackedStructTraits<PkdSSRLESeq<Types>> {
    using SearchKeyDataType = BigInt;

    using AccumType = BigInt;
    using SearchKeyType = BigInt;

    static constexpr PackedDataTypeSize DataTypeSize = PackedDataTypeSize::VARIABLE;

    static constexpr PkdSearchType KeySearchType = PkdSearchType::SUM;
    static constexpr size_t Indexes = PkdSSRLESeq<Types>::AlphabetSize;
};



}
