
// Copyright 2021 Victor Smirnov
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

#include <memoria/profiles/common/block_operations.hpp>

#include <memoria/core/packed/sseq/packed_ssrle_searchable_seq_so.hpp>

#include <memoria/core/packed/tools/packed_allocator.hpp>
#include <memoria/core/packed/tree/fse/packed_fse_quick_tree.hpp>

#include <memoria/core/integer/accumulator_common.hpp>

#include "ssrleseq/ssrleseq_reindex_fn.hpp"
#include "ssrleseq/ssrleseq_iterator.hpp"


#include <ostream>

namespace memoria {

namespace io {

template <int32_t AlphabetSize> class PackedSSRLESymbolSequence;
template <int32_t AlphabetSize> class PackedSSRLESymbolSequenceView;

}


template <
    int32_t BitsPerSymbol_,
    int32_t BytesPerBlock_
>
struct PkdSSRLSeqTypes {
    static const int32_t BitsPerSymbol       = BitsPerSymbol_;
    static const int32_t BytesPerBlock       = BytesPerBlock_;
};

template <typename Types> class PkdSSRLESeq;

template <
    int32_t BitsPerSymbol,
    int32_t BytesPerBlock = 256
>
using PkdSSRLESeqT = PkdSSRLESeq<PkdSSRLSeqTypes<BitsPerSymbol, BytesPerBlock>>;

template <typename Types_>
class PkdSSRLESeq: public PackedAllocator {

    using Base = PackedAllocator;

public:
    static constexpr uint32_t VERSION = 1;

    using Types  = Types_;
    using MyType = PkdSSRLESeq<Types_>;

    static constexpr PkdSearchType KeySearchType        = PkdSearchType::SUM;
    static constexpr PkdSearchType SearchType           = PkdSearchType::SUM;
    static const PackedDataTypeSize SizeType            = PackedDataTypeSize::VARIABLE;


    static constexpr int32_t Indexes                    = 1 << Types::BitsPerSymbol;
    static constexpr int32_t BitsPerSymbol              = Types::BitsPerSymbol;
    static constexpr int32_t Symbols                    = Indexes;

    using SymbolsRunT = SSRLERun<BitsPerSymbol>;
    using RunTraits   = SSRLERunTraits<BitsPerSymbol>;
    using CodeUnitT   = typename RunTraits::CodeUnitT;
    using SymbolT     = typename RunTraits::SymbolT;
    using RunSizeT    = typename RunTraits::RunSizeT;
    using SeqSizeT    = UAcc128T;

    using Iterator = ssrleseq::SymbolsRunIterator<MyType>;

    static constexpr size_t BytesPerBlock              = Types::BytesPerBlock;
    static constexpr size_t BytesPerBlockLog2          = NumberOfBits(Types::BytesPerBlock);
    static constexpr size_t AtomsPerBlock              = BytesPerBlock / sizeof(typename RunTraits::CodeUnitT);
    static constexpr size_t SegmentSizeInAtoms         = RunTraits::SEGMENT_SIZE_UNITS;


    static_assert(BytesPerBlock % 8 == 0, "Must be multiple of 8 bytes");

    enum {
        METADATA, SIZE_INDEX, SUM_INDEX, SYMBOLS, TOTAL_SEGMENTS_
    };


    using Base::clear;

    //using Value = uint8_t;
    //using IndexValue    = int64_t;
    //using IndexDataType = int64_t;

    //using Values = core::StaticVector<int64_t, Indexes>;

    using SumValueT  = SeqSizeT;
    using SizeValueT = SeqSizeT;

    using SumIndex  = PkdFQTreeT<SumValueT, Symbols>;
    using SizeIndex = PkdFQTreeT<SizeValueT, 1>;

    class Metadata {
        SeqSizeT size_;
        uint64_t data_size_;
    public:
        SeqSizeT& size()                 {return size_;}
        const SeqSizeT& size() const     {return size_;}

        uint64_t& data_size()                 {return data_size_;}
        const uint64_t& data_size() const     {return data_size_;}
    };

    //using SizesT = core::StaticVector<int32_t, 1>;

    struct Tools {};

    using GrowableIOSubstream = io::PackedSSRLESymbolSequence<BitsPerSymbol>;
    using IOSubstreamView     = io::PackedSSRLESymbolSequenceView<BitsPerSymbol>;

    using ExtData = std::tuple<>;
    using SparseObject = PackedSSRLESeqSO<ExtData, MyType>;


    static constexpr size_t number_of_indexes(size_t capacity) {
        return capacity <= BytesPerBlock ? 0 : divUp(capacity, BytesPerBlock);
    }

    static constexpr size_t divUp(size_t value, size_t divisor) {
        return (value / divisor) + ((value % divisor) ? 1 : 0);
    }

public:
    PkdSSRLESeq() = default;

    SeqSizeT size() const noexcept {return metadata()->size();}
    uint64_t data_size() noexcept {return metadata()->data_size();}

    // FIXME: Make setters private/protected

    // ====================================== Accessors ================================= //

    const Metadata* metadata() const noexcept {
        return Base::template get<Metadata>(METADATA);
    }

    Metadata* metadata() noexcept {
        return Base::template get<Metadata>(METADATA);
    }

    SumIndex* sum_index() noexcept {
        return Base::template get<SumIndex>(SUM_INDEX);
    }

    const SumIndex* sum_index() const noexcept {
        return Base::template get<SumIndex>(SUM_INDEX);
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
        return symbols_block_size() - meta->data_size();
    }

    size_t symbols_block_size() const noexcept {
        return this->element_size(SYMBOLS);
    }

    bool has_index() const noexcept {
        return Base::element_size(SIZE_INDEX) > 0;
    }

    Tools tools() const noexcept {
        return Tools();
    }

    Iterator iterator() const noexcept {
        return Iterator(span<CodeUnitT>(this, SYMBOLS));
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

    static size_t block_size(int32_t symbols_block_capacity)
    {
        size_t metadata_length     = PackedAllocatable::roundUpBytesToAlignmentBlocks(sizeof(Metadata));

        size_t symbols_block_capacity_aligned = PackedAllocatable::roundUpBytesToAlignmentBlocks(
                    symbols_block_capacity
        );

        size_t index_size          = number_of_indexes(symbols_block_capacity_aligned);

        size_t size_index_length   = index_size > 0 ? SizeIndex::block_size(index_size) : 0;
        size_t sum_index_length    = index_size > 0 ? SumIndex::block_size(index_size) : 0;

        size_t block_size          = Base::block_size(
                    metadata_length
                    + size_index_length
                    + sum_index_length
                    + symbols_block_capacity_aligned,
                    TOTAL_SEGMENTS_);

        return block_size;
    }

    VoidResult init(size_t block_size) noexcept
    {
        MEMORIA_ASSERT_RTN(block_size, >=, empty_size());
        return init();
    }

    VoidResult init() noexcept {
        return init_bs(empty_size());
    }

    VoidResult init_bs(size_t block_size) noexcept
    {
        MEMORIA_TRY_VOID(Base::init(block_size, TOTAL_SEGMENTS_));

        MEMORIA_TRY(meta, Base::template allocate<Metadata>(METADATA));

        meta->size() = SeqSizeT{};
        meta->data_size() = 0;

        Base::setBlockType(SIZE_INDEX, PackedBlockType::ALLOCATABLE);
        Base::setBlockType(SUM_INDEX,  PackedBlockType::ALLOCATABLE);
        Base::setBlockType(SYMBOLS, PackedBlockType::RAW_MEMORY);

        return VoidResult::of();
    }

    VoidResult clear() noexcept
    {
        MEMORIA_TRY_VOID(Base::resizeBlock(SYMBOLS, 0));

        MEMORIA_TRY_VOID(removeIndex());

        auto meta = this->metadata();

        meta->size()        = SeqSizeT{};
        meta->data_size()   = 0;

        return VoidResult::of();
    }

    void reset() noexcept {
        auto meta = this->metadata();

        meta->size()        = SeqSizeT{};
        meta->data_size()   = 0;
    }


public:
    static constexpr size_t default_size(size_t available_space) noexcept
    {
        return empty_size();
    }

    VoidResult init_default(size_t block_size) noexcept {
        return init();
    }


    static size_t empty_size()
    {
        size_t metadata_length     = PackedAllocatable::roundUpBytesToAlignmentBlocks(sizeof(Metadata));
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


    VoidResult removeIndex() noexcept
    {
        MEMORIA_TRY_VOID(Base::free(SIZE_INDEX));
        MEMORIA_TRY_VOID(Base::free(SUM_INDEX));

        return VoidResult::of();
    }


    VoidResult createIndex(size_t index_size) noexcept
    {
        size_t size_index_block_size = SizeIndex::block_size(index_size);
        MEMORIA_TRY_VOID(Base::resizeBlock(SIZE_INDEX, size_index_block_size));

        size_t sum_index_block_size = SumIndex::block_size(index_size);
        MEMORIA_TRY_VOID(Base::resizeBlock(SUM_INDEX, sum_index_block_size));

        auto size_index = this->size_index();
        size_index->allocatable().setAllocatorOffset(this);
        MEMORIA_TRY_VOID(size_index->init(index_size));

        auto sum_index = this->sum_index();
        sum_index->allocatable().setAllocatorOffset(this);
        MEMORIA_TRY_VOID(sum_index->init(index_size));

        return VoidResult::of();
    }


//    uint64_t populate_entry(io::SymbolsBuffer& buffer, uint64_t idx, int32_t symbol, bool entry_start) const
//    {
//        auto iter = this->iterator(idx);
//        if (MMA_LIKELY(iter.has_data()))
//        {
//            if (MMA_UNLIKELY(entry_start && iter.symbol() == symbol))
//            {
//                if (MMA_UNLIKELY(iter.remaining_run_length() > 1))
//                {
//                    buffer.append_run(iter.symbol(), iter.remaining_run_length());
//                    return idx + 1;
//                }
//                else
//                {
//                    iter.next_run();
//                }
//            }

//            while (iter.has_data())
//            {
//                if (iter.symbol() >= symbol)
//                {
//                    buffer.append_run(iter.symbol(), iter.remaining_run_length());
//                    iter.next_run();
//                }
//                else {
//                    return iter.local_pos();
//                }
//            }
//        }

//        return this->size();
//    }

//    uint64_t populate_while(io::SymbolsBuffer& buffer, uint64_t idx, int32_t symbol) const
//    {
//        auto iter = this->iterator(idx);
//        while (iter.has_data())
//        {
//            if (iter.symbol() >= symbol)
//            {
//                buffer.append_run(iter.symbol(), iter.remaining_run_length());
//                iter.next_run();
//            }
//            else {
//                buffer.finish();
//                return iter.local_pos();
//            }
//        }

//        buffer.finish();

//        return this->size();
//    }

//    uint64_t populate(io::SymbolsBuffer& buffer, uint64_t idx) const
//    {
//        auto iter = this->iterator(idx);
//        while (iter.has_data())
//        {
//            buffer.append_run(iter.symbol(), iter.remaining_run_length());
//            iter.next_run();
//        }

//        buffer.finish();

//        return this->size();
//    }

//    uint64_t populate(io::SymbolsBuffer& buffer, uint64_t idx, uint64_t size) const
//    {
//        auto iter = this->iterator(idx);
//        uint64_t total{};
//        while (iter.has_data())
//        {
//            auto len = iter.remaining_run_length();

//            if (MMA_LIKELY(len + total <= size))
//            {
//                total += len;
//                buffer.append_run(iter.symbol(), len);
//                iter.next_run();
//            }
//            else {
//                auto remaining = size - total;
//                buffer.append_run(iter.symbol(), remaining);
//                total += remaining;
//            }
//        }

//        buffer.finish();

//        return idx + total;
//    }



    // ========================================= Update ================================= //

    MMA_PKD_OOM_SAFE
    VoidResult reindex(bool compactify = true) noexcept {
        return do_reindex(Optional<Span<SymbolsRunT>>{}, compactify);
    }

private:
    MMA_PKD_OOM_SAFE
    VoidResult do_reindex(
            Optional<Span<SymbolsRunT>> data,
            bool compactify = false
    ) noexcept
    {
        ssrleseq::ReindexFn<MyType> reindex_fn;
        return reindex_fn.reindex(*this, data, compactify);
    }
public:

    VoidResult check() const noexcept
    {
        return wrap_throwing([&]() -> VoidResult {
            if (has_index())
            {
                MEMORIA_TRY_VOID(size_index()->check());
                MEMORIA_TRY_VOID(sum_index()->check());

                ssrleseq::ReindexFn<MyType> reindex_fn;
                return reindex_fn.check(*this);
            }

            return VoidResult::of();
        });
    }

    VoidResult ensure_capacity(int32_t capacity) noexcept
    {
        int32_t current_capacity = this->symbols_block_capacity();

        if (current_capacity < capacity)
        {
            return enlargeData(capacity - current_capacity);
        }

        return VoidResult::of();
    }

    bool has_capacity(int32_t required_capacity) const
    {
        int32_t current_capacity = this->symbols_block_capacity();
        return current_capacity >= required_capacity;
    }


    VoidResult enlargeData(int32_t length) noexcept
    {
        int32_t new_size = this->element_size(SYMBOLS) + length;
        MEMORIA_TRY_VOID(Base::resizeBlock(SYMBOLS, new_size));
        return VoidResult::of();
    }

protected:

//    MMA_PKD_OOM_SAFE
//    VoidResult shrinkData(size_t length) noexcept
//    {
//        size_t current_size = this->element_size(SYMBOLS);

//        if (current_size > length)
//        {
//            size_t new_size = current_size - length;
//            MEMORIA_TRY_VOID(Base::resizeBlock(SYMBOLS, new_size));
//        }

//        return VoidResult::of();
//    }


//    MMA_PKD_OOM_SAFE
//    VoidResult shrink_to_data() noexcept
//    {
//        return shrinkData(this->symbols_block_capacity());
//    }

    SeqSizeT count_symbols(Span<const SymbolsRunT> runs) noexcept
    {
        SeqSizeT sum{};
        for (const auto& run: runs) {
            sum += run.full_run_length();
        }
        return sum;
    }

public:

    Result<size_t> append(Span<const SymbolsRunT> runs) noexcept
    {
        using ResultT = Result<size_t>;

        if (runs.size() == 0) {
            return ResultT::of(size_t{});
        }

        Metadata* meta = metadata();
        if (meta->size())
        {
            Location loc = locate_last_run(meta);
            SeqSizeT len = loc.run.full_run_length();

            if (runs.size() == 1)
            {
                auto res = RunTraits::insert(loc.run, runs[0], len);                
                return do_append(meta, loc.unit_idx, len, res.span());
            }            
            else {
                return do_append(meta, meta->data_size(), 0, runs);
            }
        }
        else {
            return do_append(meta, 0, 0, runs);
        }
    }
private:

    SizeTResult do_append(Metadata* meta, size_t start, SeqSizeT run_len0, Span<const SymbolsRunT> runs) noexcept
    {
        size_t data_size = RunTraits::compute_size(runs, start);
        size_t syms_size = element_size(SYMBOLS) / sizeof (CodeUnitT);
        if (data_size > syms_size)
        {
            size_t bs = data_size * sizeof(CodeUnitT);
            MEMORIA_TRY(can_alocate, try_allocation(SYMBOLS, bs));
            if (can_alocate)
            {
                MEMORIA_TRY_VOID(resizeBlock(SYMBOLS, bs));
            }
            else {
                size_t new_syms_size = syms_size > 0 ? syms_size : 4;
                while (new_syms_size < data_size) {
                    new_syms_size *= 2;
                }

                return SizeTResult::of(new_syms_size * sizeof(CodeUnitT));
            }
        }

        Span<CodeUnitT> syms = symbols();
        RunTraits::write_segments_to(runs, syms, start);

        meta->data_size() = data_size;
        meta->size() -= run_len0;
        meta->size() += count_symbols(runs);

        MEMORIA_TRY_VOID(reindex());

        return SizeTResult::of(size_t{});
    }

    template <typename T1, typename T2>
    void append_all(std::vector<T1>& vv, Span<T2> span) const {
        vv.insert(vv.end(), span.begin(), span.end());
    }

    VoidResult split_buffer(
            std::vector<SymbolsRunT>& left,
            std::vector<SymbolsRunT>& right,
            SeqSizeT pos) const noexcept
    {
        std::vector<SymbolsRunT> runs = iterator().as_vector();
        return split_buffer(runs, left, right, pos);
    }

    VoidResult split_buffer(
            const std::vector<SymbolsRunT>& runs,
            std::vector<SymbolsRunT>& left,
            std::vector<SymbolsRunT>& right,
            SeqSizeT pos) const noexcept
    {
        auto res = find_in_syms(runs, pos);

        if (res.run_idx < runs.size())
        {
            auto s_res = runs[res.run_idx].split(res.local_offset);

            append_all(left, to_const_span(runs, 0, res.run_idx));
            append_all(left, s_res.left.span());

            append_all(right, s_res.right.span());

            if (res.run_idx + 1 < runs.size()) {
                append_all(right, to_span(runs).subspan(res.run_idx + 1));
            }
        }
        else {
            left = std::move(runs);
        }

        return VoidResult::of();
    }



public:

    VoidResult insert(SeqSizeT idx, Span<const SymbolsRunT> runs) noexcept
    {
        std::vector<SymbolsRunT> left;
        std::vector<SymbolsRunT> right;

        SeqSizeT size = this->size();

        if (idx <= size)
        {
            MEMORIA_TRY_VOID(split_buffer(left, right, idx));

            append_all(left, runs);
            append_all(left, to_const_span(right));

            compactify_runs(left);

            size_t new_data_size = RunTraits::compute_size(left, 0);

            MEMORIA_TRY_VOID(resizeBlock(SYMBOLS, new_data_size * sizeof(CodeUnitT)));

            Span<CodeUnitT> atoms = symbols();
            RunTraits::write_segments_to(left, atoms, 0);

            Metadata* meta = metadata();
            meta->data_size() = new_data_size;
            meta->size() += count_symbols(runs);

            return do_reindex(to_span(left));
        }
        else {
            return MEMORIA_MAKE_GENERIC_ERROR("Range check in SSRLE sequence insert: idx={}, size={}", idx, size);
        }
    }

    VoidResult removeSpace(SeqSizeT start, SeqSizeT end) noexcept {
        return remove(start, end);
    }

    VoidResult remove(SeqSizeT start, SeqSizeT end, bool compactify = false) noexcept
    {
        if (end > start)
        {
            auto meta = this->metadata();

            MEMORIA_ASSERT_RTN(start, <=, end);
            MEMORIA_ASSERT_RTN(end, <=, meta->size());

            std::vector<SymbolsRunT> runs = iterator().as_vector();

            std::vector<SymbolsRunT> runs_res;

            if (end < meta->size()) {
                if (start > SeqSizeT{0}) {
                    std::vector<SymbolsRunT> right1;
                    MEMORIA_TRY_VOID(split_buffer(runs, runs_res, right1, start));

                    std::vector<SymbolsRunT> left2;
                    std::vector<SymbolsRunT> right2;
                    MEMORIA_TRY_VOID(split_buffer(right1, left2, right2, end - start));

                    append_all(runs_res, to_const_span(right2));
                }
                else {
                    std::vector<SymbolsRunT> left;
                    MEMORIA_TRY_VOID(split_buffer(runs, left, runs_res, end));
                }
            }
            else {
                if (start > SeqSizeT{0}) {
                    std::vector<SymbolsRunT> right;
                    MEMORIA_TRY_VOID(split_buffer(runs, runs_res, right, start));
                }
                else {
                    return clear();
                }
            }

            compactify_runs(runs_res);

            size_t new_data_size = RunTraits::compute_size(runs_res, 0);
            MEMORIA_TRY_VOID(resizeBlock(SYMBOLS, new_data_size * sizeof(CodeUnitT)));

            Span<CodeUnitT> atoms = symbols();
            RunTraits::write_segments_to(runs_res, atoms, 0);

            meta->data_size() = new_data_size;
            meta->size() -= end - start;

            return do_reindex(to_span(runs_res));
        }
        else {
            return VoidResult::of();
        }
    }

    VoidResult compactify() noexcept
    {
        std::vector<SymbolsRunT> syms = this->iterator().as_vector();

        compactify_runs(syms);

        size_t new_data_size = RunTraits::compute_size(syms, 0);

        auto new_block_size = PackedAllocatable::roundUpBytesToAlignmentBlocks(new_data_size * sizeof(CodeUnitT));
        MEMORIA_TRY_VOID(this->resizeBlock(SYMBOLS, new_block_size));

        Span<CodeUnitT> atoms = symbols();
        RunTraits::write_segments_to(syms, atoms, 0);

        return this->reindex();
    }

    void compactify_runs(Span<SymbolsRunT> runs) const noexcept
    {
        RunTraits::compactify_runs(runs);
    }

    void compactify_runs(std::vector<SymbolsRunT>& runs) const noexcept
    {
        RunTraits::compactify_runs(runs);
    }



    // ========================================= Node ================================== //
    VoidResult splitTo(MyType* other, SeqSizeT idx) noexcept
    {
        Metadata* meta = this->metadata();

        if (idx < meta->size())
        {
            Metadata* other_meta = other->metadata();

            auto location = this->find_run(idx);

            auto result = RunTraits::split(location.run, location.local_symbol_idx);

            size_t adjustment = location.run.size_in_units();
            std::vector<SymbolsRunT> right_runs = this->iterator(location.unit_idx + adjustment).as_vector();

            size_t left_data_size = RunTraits::compute_size(result.left.span(), location.unit_idx);
            MEMORIA_TRY_VOID(this->resizeBlock(SYMBOLS, left_data_size * sizeof(CodeUnitT)));

            Span<CodeUnitT> left_syms = symbols();
            RunTraits::write_segments_to(result.left.span(), left_syms, location.unit_idx);

            size_t right_atoms_size = RunTraits::compute_size(result.right.span(), right_runs);

            auto right_block_size = MyType::block_size(right_atoms_size * sizeof(CodeUnitT));
            MEMORIA_TRY_VOID(other->init_bs(right_block_size));
            MEMORIA_TRY_VOID(other->resizeBlock(SYMBOLS, right_atoms_size * sizeof(CodeUnitT)));

            Span<CodeUnitT> right_syms = other->symbols();
            size_t right_data_size = RunTraits::write_segments_to(result.right.span(), right_runs, right_syms);

            other_meta->size() = meta->size() - idx;
            other_meta->data_size() = right_data_size;

            meta->size() = idx;
            meta->data_size() = left_data_size;

            MEMORIA_TRY_VOID(other->reindex());

            return reindex();
        }
        else {
            return MEMORIA_MAKE_GENERIC_ERROR("Split index is out of range: {} :: {}", idx, meta->size());
        }
    }

    VoidResult mergeWith(MyType* other) const noexcept
    {
        auto meta       = this->metadata();
        auto other_meta = other->metadata();

        std::vector<SymbolsRunT> syms = other->iterator().as_vector();
        iterator().read_to(syms);

        compactify_runs(syms);

        size_t new_data_size = RunTraits::compute_size(syms, 0);

        auto new_block_size = new_data_size * sizeof(CodeUnitT);
        MEMORIA_TRY_VOID(other->resizeBlock(SYMBOLS, new_block_size));

        Span<CodeUnitT> atoms = other->symbols();
        RunTraits::write_segments_to(syms, atoms, 0);

        other_meta->size() += meta->size();
        other_meta->data_size() = new_data_size;

        return other->do_reindex(to_span(syms));
    }

    SymbolT access(SeqSizeT pos) const
    {
        const Metadata* meta = this->metadata();
        SeqSizeT size = meta->size();

        MEMORIA_V1_ASSERT_TRUE(pos < size);

        size_t unit_pos;
        SeqSizeT sum;

        if (has_index()) {
            const auto* index = this->size_index();
            auto res = index->find_gt(0, pos);
            unit_pos = res.local_pos() * AtomsPerBlock;
            sum = res.prefix();
        }
        else {
            unit_pos = 0;
            sum = SeqSizeT{};
        }

        auto ii = this->iterator(unit_pos);

        SymbolsRunT tgt;
        ii.do_while([&](const SymbolsRunT& run){
            SeqSizeT size = run.full_run_length();
            if (pos < sum + size) {
                tgt = run;
                return false;
            }
            else {
                sum += size;
                return true;
            }
        });

        if (tgt) {            
            return tgt.symbol(pos - sum);
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Invalid SSRLE sequence").do_throw();
        }
    }



    struct SelectResult {
        SeqSizeT idx;
        SeqSizeT rank;
    };


private:
    using FindResult = typename SumIndex::WalkerBase;

    struct SelectFwEqFn {
        FindResult index_fn(const SumIndex* index, SeqSizeT rank, SymbolT symbol) const {
            return index->find_ge(symbol, rank);
        }

        SeqSizeT full_rank_fn(const SymbolsRunT& run, SymbolT symbol) const {
            return run.full_rank_eq(symbol);
        }

        SeqSizeT select_fn(const SymbolsRunT& run, SeqSizeT rank, SymbolT symbol) const {
            return run.select_fw_eq(rank, symbol);
        }
    };

    template<typename Fn>
    struct SelectFwFnBase {
        FindResult index_fn(const SumIndex* index, SeqSizeT rank, SymbolT symbol) const
        {
            SeqSizeT sum{};
            int32_t size = index->size();
            int32_t c;

            for (c = 0; c < size; c++) {
                SeqSizeT value = self().sum(index, c, symbol);
                if (rank < sum + value) {
                    break;
                }
                else {
                    sum += value;
                }
            }

            return FindResult(c, sum);
        }

        const Fn& self() const {return *static_cast<const Fn*>(this);}
    };

    struct SelectFwLtFn: SelectFwFnBase<SelectFwLtFn> {
        SeqSizeT sum(const SumIndex* index, int32_t idx, int32_t symbol) const {
            SeqSizeT sum{};
            for (int32_t c = 0; c < symbol; c++) {
                sum += index->value(c, idx);
            }
            return sum;
        }

        SeqSizeT full_rank_fn(const SymbolsRunT& run, SymbolT symbol) const {
            return run.full_rank_lt(symbol);
        }

        SeqSizeT select_fn(const SymbolsRunT& run, SeqSizeT rank, SymbolT symbol) const {
            return run.select_fw_lt(rank, symbol);
        }
    };

    struct SelectFwLeFn: SelectFwFnBase<SelectFwLeFn> {
        SeqSizeT sum(const SumIndex* index, int32_t idx, int32_t symbol) const {
            SeqSizeT sum{};
            for (int32_t c = 0; c <= symbol; c++) {
                sum += index->value(c, idx);
            }
            return sum;
        }

        SeqSizeT full_rank_fn(const SymbolsRunT& run, SymbolT symbol) const {
            return run.full_rank_le(symbol);
        }

        SeqSizeT select_fn(const SymbolsRunT& run, SeqSizeT rank, SymbolT symbol) const {
            return run.select_fw_le(rank, symbol);
        }
    };

    struct SelectFwGtFn: SelectFwFnBase<SelectFwGtFn> {
        SeqSizeT sum(const SumIndex* index, int32_t idx, int32_t symbol) const {
            SeqSizeT sum{};
            for (int32_t c = symbol + 1; c < Symbols; c++) {
                sum += index->value(c, idx);
            }
            return sum;
        }

        SeqSizeT full_rank_fn(const SymbolsRunT& run, SymbolT symbol) const {
            return run.full_rank_gt(symbol);
        }

        SeqSizeT select_fn(const SymbolsRunT& run, SeqSizeT rank, SymbolT symbol) const {
            return run.select_fw_gt(rank, symbol);
        }
    };

    struct SelectFwGeFn: SelectFwFnBase<SelectFwGeFn> {
        SeqSizeT sum(const SumIndex* index, int32_t idx, int32_t symbol) const {
            SeqSizeT sum{};
            for (int32_t c = symbol; c < Symbols; c++) {
                sum += index->value(c, idx);
            }
            return sum;
        }

        SeqSizeT full_rank_fn(const SymbolsRunT& run, SymbolT symbol) const {
            return run.full_rank_ge(symbol);
        }

        SeqSizeT select_fn(const SymbolsRunT& run, SeqSizeT rank, SymbolT symbol) const {
            return run.select_fw_ge(rank, symbol);
        }
    };

    struct SelectFwNeqFn: SelectFwFnBase<SelectFwNeqFn> {
        SeqSizeT sum(const SumIndex* index, int32_t idx, int32_t symbol) const {
            SeqSizeT sum{};
            for (int32_t c = 0; c < Symbols; c++) {
                sum += c != symbol ? index->value(c, idx) : SeqSizeT{0};
            }
            return sum;
        }

        SeqSizeT full_rank_fn(const SymbolsRunT& run, SymbolT symbol) const {
            return run.full_rank_neq(symbol);
        }

        SeqSizeT select_fn(const SymbolsRunT& run, SeqSizeT rank, SymbolT symbol) const {
            return run.select_fw_neq(rank, symbol);
        }
    };



    template <typename Fn>
    SelectResult select_fw_fn(SeqSizeT rank, SymbolT symbol, Fn&& fn) const
    {
        MEMORIA_V1_ASSERT_TRUE(symbol < Symbols);

        size_t unit_pos;
        SeqSizeT rank_sum;
        SeqSizeT size_sum;

        if (has_index()) {
            const auto* sums = this->sum_index();

            auto res = fn.index_fn(sums, rank, symbol);
            if (res.idx() < sums->size())
            {
                unit_pos = res.idx() * AtomsPerBlock;
                rank_sum = res.prefix();

                const auto* sizes  = this->size_index();
                size_sum = sizes->sum(0, res.idx());
            }
            else {
                return SelectResult{size(), (RunSizeT)res.prefix()};
            }
        }
        else {
            unit_pos = 0;
            rank_sum = SeqSizeT{0};
            size_sum = SeqSizeT{0};
        }

        auto ii = this->iterator(unit_pos);

        bool res{};
        SeqSizeT idx;

        ii.do_while([&](const SymbolsRunT& run) {
            SeqSizeT run_rank = fn.full_rank_fn(run, symbol);
            if (MMA_UNLIKELY(rank < rank_sum + run_rank)) {
                idx = fn.select_fn(run, rank - rank_sum, symbol);
                res = true;
                return false;
            }
            else {
                rank_sum += run_rank;
                size_sum += SeqSizeT{run.full_run_length()};
                return true;
            }
        });

        if (res) {
            return SelectResult{idx + size_sum, rank_sum};
        }
        else {
            return SelectResult{size(), rank_sum};
        }
    }


    struct RankEqFn {
        SumValueT sum_fn(const SumIndex* index, int32_t idx, SymbolT symbol) const {
            return index->sum(symbol, idx);
        }

        SumValueT sum_all_fn(const SumIndex* index, SymbolT symbol) const {
            return index->sum(symbol);
        }

        SumValueT rank_fn(const SymbolsRunT& run, SeqSizeT idx, SymbolT symbol) const {
            return run.rank_eq(idx, symbol);
        }

        SumValueT full_rank_fn(const SymbolsRunT& run, SymbolT symbol) const {
            return run.full_rank_eq(symbol);
        }
    };



    struct RankLtFn {
        SumValueT sum_fn(const SumIndex* index, int32_t idx, int32_t symbol) const
        {
            SumValueT sum{};
            for (int32_t c = 0; c < symbol; c++) {
                sum += index->sum(c, idx);
            }
            return sum;
        }

        SumValueT sum_all_fn(const SumIndex* index, SymbolT symbol) const {
            return sum_fn(index, index->size(), symbol);
        }

        SumValueT rank_fn(const SymbolsRunT& run, SeqSizeT idx, SymbolT symbol) const {
            return run.rank_lt(idx, symbol);
        }

        SumValueT full_rank_fn(const SymbolsRunT& run, SymbolT symbol) const {
            return run.full_rank_lt(symbol);
        }
    };

    struct RankLeFn {
        SumValueT sum_fn(const SumIndex* index, int32_t idx, SymbolT symbol) const
        {
            SumValueT sum{};
            for (int32_t c = 0; c <= symbol; c++) {
                sum += index->sum(c, idx);
            }
            return sum;
        }

        SumValueT sum_all_fn(const SumIndex* index, SymbolT symbol) const {
            return sum_fn(index, index->size(), symbol);
        }

        SumValueT rank_fn(const SymbolsRunT& run, SeqSizeT idx, SymbolT symbol) const {
            return run.rank_le(idx, symbol);
        }

        SumValueT full_rank_fn(const SymbolsRunT& run, SymbolT symbol) const {
            return run.full_rank_le(symbol);
        }
    };

    struct RankGtFn {
        SumValueT sum_fn(const SumIndex* index, int32_t idx, int32_t symbol) const
        {
            SumValueT sum{};
            for (int32_t c = symbol + 1; c < Symbols; c++) {
                sum += index->sum(c, idx);
            }
            return sum;
        }

        SumValueT sum_all_fn(const SumIndex* index, SymbolT symbol) const {
            return sum_fn(index, index->size(), symbol);
        }

        SumValueT rank_fn(const SymbolsRunT& run, SeqSizeT idx, SymbolT symbol) const {
            return run.rank_gt(idx, symbol);
        }

        SumValueT full_rank_fn(const SymbolsRunT& run, SymbolT symbol) const {
            return run.full_rank_gt(symbol);
        }
    };

    struct RankGeFn {
        SumValueT sum_fn(const SumIndex* index, int32_t idx, int32_t symbol) const
        {
            SumValueT sum{};
            for (int32_t c = symbol; c < Symbols; c++) {
                sum += index->sum(c, idx);
            }
            return sum;
        }

        SumValueT sum_all_fn(const SumIndex* index, SymbolT symbol) const {
            return sum_fn(index, index->size(), symbol);
        }

        SumValueT rank_fn(const SymbolsRunT& run, SeqSizeT idx, SymbolT symbol) const {
            return run.rank_ge(idx, symbol);
        }

        SumValueT full_rank_fn(const SymbolsRunT& run, SymbolT symbol) const {
            return run.full_rank_ge(symbol);
        }
    };

    struct RankNeqFn {
        SumValueT sum_fn(const SumIndex* index, int32_t idx, int32_t symbol) const
        {
            SumValueT sum{};
            for (int32_t c = 0; c < Symbols; c++) {
                sum += c != symbol ? index->sum(c, idx) : SeqSizeT{0};
            }
            return sum;
        }

        SumValueT sum_all_fn(const SumIndex* index, SymbolT symbol) const {
            return sum_fn(index, index->size(), symbol);
        }

        SumValueT rank_fn(const SymbolsRunT& run, SeqSizeT idx, SymbolT symbol) const {
            return run.rank_neq(idx, symbol);
        }

        SumValueT full_rank_fn(const SymbolsRunT& run, SymbolT symbol) const {
            return run.full_rank_neq(symbol);
        }
    };


    template <typename Fn>
    SeqSizeT rank_fn(SeqSizeT pos, SymbolT symbol, Fn&& fn) const
    {
        if (MMA_UNLIKELY(!pos)) {
            return SeqSizeT{0};
        }

        const Metadata* meta = this->metadata();
        SeqSizeT size = meta->size();

        MEMORIA_V1_ASSERT_TRUE(pos <= size && symbol < Symbols);

        size_t unit_pos;
        SeqSizeT size_sum;
        SeqSizeT rank_sum;

        if (has_index()) {
            const auto* index = this->size_index();

            if (pos < size)
            {
                auto res = index->find_gt(0, pos);
                unit_pos = res.local_pos() * AtomsPerBlock;
                size_sum = res.prefix();

                const auto* sums  = this->sum_index();
                rank_sum = fn.sum_fn(sums, res.local_pos(), symbol);
            }
            else {
                const auto* sums  = this->sum_index();
                return fn.sum_all_fn(sums, symbol);
            }
        }
        else {
            unit_pos = 0;
            size_sum = SeqSizeT{0};
            rank_sum = SeqSizeT{0};
        }

        auto ii = this->iterator(unit_pos);

        ii.do_while([&](const SymbolsRunT& run){
            SeqSizeT size = run.full_run_length();
            if (pos < size_sum + size) {
                rank_sum += fn.rank_fn(run, pos - size_sum, symbol);
                return false;
            }
            else {
                size_sum += size;
                rank_sum += fn.full_rank_fn(run, symbol);
                return true;
            }
        });

        return rank_sum;
    }


public:
    SelectResult select_fw(SeqSizeT rank, SymbolT symbol, SeqOpType op_type) const {
        switch (op_type) {
            case SeqOpType::EQ : return select_fw_eq(rank, symbol, op_type);
            case SeqOpType::NEQ: return select_fw_neq(rank, symbol, op_type);
            case SeqOpType::LT : return select_fw_lt(rank, symbol, op_type);
            case SeqOpType::LE : return select_fw_le(rank, symbol, op_type);
            case SeqOpType::GT : return select_fw_gt(rank, symbol, op_type);
            case SeqOpType::GE : return select_fw_ge(rank, symbol, op_type);

            default: MEMORIA_MAKE_GENERIC_ERROR("Unsupported SeqOpType: {}", (size_t)op_type).do_throw();
        }
    }

    SelectResult select_fw(SeqSizeT idx, SeqSizeT rank, SymbolT symbol, SeqOpType op_type) const {
        switch (op_type) {
            case SeqOpType::EQ : return select_fw_eq(idx, rank, symbol, op_type);
            case SeqOpType::NEQ: return select_fw_neq(idx, rank, symbol, op_type);
            case SeqOpType::LT : return select_fw_lt(idx, rank, symbol, op_type);
            case SeqOpType::LE : return select_fw_le(idx, rank, symbol, op_type);
            case SeqOpType::GT : return select_fw_gt(idx, rank, symbol, op_type);
            case SeqOpType::GE : return select_fw_ge(idx, rank, symbol, op_type);

            default: MEMORIA_MAKE_GENERIC_ERROR("Unsupported SeqOpType: {}", (size_t)op_type).do_throw();
        }
    }

    SelectResult select_bw(SeqSizeT rank, SymbolT symbol, SeqOpType op_type) const {
        switch (op_type) {
            case SeqOpType::EQ : return select_bw_eq(rank, symbol, op_type);
            case SeqOpType::NEQ: return select_bw_neq(rank, symbol, op_type);
            case SeqOpType::LT : return select_bw_lt(rank, symbol, op_type);
            case SeqOpType::LE : return select_bw_le(rank, symbol, op_type);
            case SeqOpType::GT : return select_bw_gt(rank, symbol, op_type);
            case SeqOpType::GE : return select_bw_ge(rank, symbol, op_type);

            default: MEMORIA_MAKE_GENERIC_ERROR("Unsupported SeqOpType: {}", (size_t)op_type).do_throw();
        }
    }

    SelectResult select_bw(SeqSizeT idx, SeqSizeT rank, SymbolT symbol, SeqOpType op_type) const {
        switch (op_type) {
            case SeqOpType::EQ : return select_bw_eq(idx, rank, symbol, op_type);
            case SeqOpType::NEQ: return select_bw_neq(idx, rank, symbol, op_type);
            case SeqOpType::LT : return select_bw_lt(idx, rank, symbol, op_type);
            case SeqOpType::LE : return select_bw_le(idx, rank, symbol, op_type);
            case SeqOpType::GT : return select_bw_gt(idx, rank, symbol, op_type);
            case SeqOpType::GE : return select_bw_ge(idx, rank, symbol, op_type);

            default: MEMORIA_MAKE_GENERIC_ERROR("Unsupported SeqOpType: {}", (size_t)op_type).do_throw();
        }
    }


    SelectResult rank(SeqSizeT pos, SymbolT symbol, SeqOpType op_type) const {
        switch (op_type) {
            case SeqOpType::EQ : return rank_eq(pos, symbol, op_type);
            case SeqOpType::NEQ: return rank_neq(pos, symbol, op_type);
            case SeqOpType::LT : return rank_lt(pos, symbol, op_type);
            case SeqOpType::LE : return rank_le(pos, symbol, op_type);
            case SeqOpType::GT : return rank_gt(pos, symbol, op_type);
            case SeqOpType::GE : return rank_ge(pos, symbol, op_type);

            default: MEMORIA_MAKE_GENERIC_ERROR("Unsupported SeqOpType: {}", (size_t)op_type).do_throw();
        }
    }

    SelectResult rank(SeqSizeT start, SeqSizeT end, SymbolT symbol, SeqOpType op_type) const {
        switch (op_type) {
            case SeqOpType::EQ : return rank_eq(start, end, symbol, op_type);
            case SeqOpType::NEQ: return rank_neq(start, end, symbol, op_type);
            case SeqOpType::LT : return rank_lt(start, end, symbol, op_type);
            case SeqOpType::LE : return rank_le(start, end, symbol, op_type);
            case SeqOpType::GT : return rank_gt(start, end, symbol, op_type);
            case SeqOpType::GE : return rank_ge(start, end, symbol, op_type);

            default: MEMORIA_MAKE_GENERIC_ERROR("Unsupported SeqOpType: {}", (size_t)op_type).do_throw();
        }
    }

    SelectResult rank(SymbolT symbol, SeqOpType op_type) const {
        switch (op_type) {
            case SeqOpType::EQ : return rank_eq(symbol, op_type);
            case SeqOpType::NEQ: return rank_neq(symbol, op_type);
            case SeqOpType::LT : return rank_lt(symbol, op_type);
            case SeqOpType::LE : return rank_le(symbol, op_type);
            case SeqOpType::GT : return rank_gt(symbol, op_type);
            case SeqOpType::GE : return rank_ge(symbol, op_type);

            default: MEMORIA_MAKE_GENERIC_ERROR("Unsupported SeqOpType: {}", (size_t)op_type).do_throw();
        }
    }


    SelectResult select_fw_eq(SeqSizeT rank, SymbolT symbol) const {
        return select_fw_fn(rank, symbol, SelectFwEqFn());
    }

    SelectResult select_fw_neq(SeqSizeT rank, SymbolT symbol) const {
        return select_fw_fn(rank, symbol, SelectFwNeqFn());
    }

    SelectResult select_fw_lt(SeqSizeT rank, SymbolT symbol) const {
        return select_fw_fn(rank, symbol, SelectFwLtFn());
    }

    SelectResult select_fw_le(SeqSizeT rank, SymbolT symbol) const {
        return select_fw_fn(rank, symbol, SelectFwLeFn());
    }

    SelectResult select_fw_gt(SeqSizeT rank, SymbolT symbol) const {
        return select_fw_fn(rank, symbol, SelectFwGtFn());
    }

    SelectResult select_fw_ge(SeqSizeT rank, SymbolT symbol) const {
        return select_fw_fn(rank, symbol, SelectFwGeFn());
    }

    SelectResult select_fw_ge(SeqSizeT idx, SeqSizeT rank, SymbolT symbol) const {
        SeqSizeT rank_base = rank_ge(idx, symbol);
        return select_fw_ge(rank - rank_base, symbol);
    }

    SelectResult select_fw_gt(SeqSizeT idx, SeqSizeT rank, SymbolT symbol) const {
        SeqSizeT rank_base = rank_gt(idx, symbol);
        return select_fw_gt(rank - rank_base, symbol);
    }

    SelectResult select_fw_le(SeqSizeT idx, SeqSizeT rank, SymbolT symbol) const {
        SeqSizeT rank_base = rank_le(idx, symbol);
        return select_fw_le(rank - rank_base, symbol);
    }

    SelectResult select_fw_lt(SeqSizeT idx, SeqSizeT rank, SymbolT symbol) const {
        SeqSizeT rank_base = rank_lt(idx, symbol);
        return select_fw_lt(rank - rank_base, symbol);
    }

    SelectResult select_fw_eq(SeqSizeT idx, SeqSizeT rank, SymbolT symbol) const {
        SeqSizeT rank_base = rank_eq(idx, symbol);
        return select_fw_eq(rank - rank_base, symbol);
    }

    SelectResult select_fw_neq(SeqSizeT idx, SeqSizeT rank, SymbolT symbol) const {
        SeqSizeT rank_base = rank_neq(idx, symbol);
        return select_fw_neq(rank - rank_base, symbol);
    }



    SelectResult select_bw_eq(SeqSizeT rank, SymbolT symbol) const
    {
        SeqSizeT size = this->size();
        SeqSizeT full_rank = rank_eq(size, symbol);
        if (rank < full_rank) {
            return select_fw_eq(full_rank - rank - SeqSizeT{1}, symbol);
        }
        else {
            return SelectResult{size, full_rank};
        }
    }

    SelectResult select_bw_neq(SeqSizeT rank, SymbolT symbol) const
    {
        SeqSizeT size = this->size();
        SeqSizeT full_rank = rank_neq(size, symbol);
        if (rank < full_rank) {
            return select_fw_neq(full_rank - rank - SeqSizeT{1}, symbol);
        }
        else {
            return SelectResult{size, full_rank};
        }
    }

    SelectResult select_bw_lt(SeqSizeT rank, SymbolT symbol) const
    {
        SeqSizeT size = this->size();
        SeqSizeT full_rank = rank_lt(size, symbol);
        if (rank < full_rank) {
            return select_fw_lt(full_rank - rank - SeqSizeT{1}, symbol);
        }
        else {
            return SelectResult{size, full_rank};
        }
    }

    SelectResult select_bw_le(SeqSizeT rank, SymbolT symbol) const
    {
        SeqSizeT size = this->size();
        SeqSizeT full_rank = rank_le(size, symbol);
        if (rank < full_rank) {
            return select_fw_le(full_rank - rank - SeqSizeT{1}, symbol);
        }
        else {
            return SelectResult{size, full_rank};
        }
    }

    SelectResult select_bw_gt(SeqSizeT rank, SymbolT symbol) const
    {
        SeqSizeT size = this->size();
        SeqSizeT full_rank = rank_neq(size, symbol);
        if (rank < full_rank) {
            return select_fw_gt(full_rank - rank - SeqSizeT{1}, symbol);
        }
        else {
            return SelectResult{size, full_rank};
        }
    }

    SelectResult select_bw_ge(SeqSizeT rank, SymbolT symbol) const
    {
        SeqSizeT size = this->size();
        SeqSizeT full_rank = rank_neq(size, symbol);
        if (rank < full_rank) {
            return select_fw_ge(full_rank - rank - SeqSizeT{1}, symbol);
        }
        else {
            return SelectResult{size, full_rank};
        }
    }


    SelectResult select_bw_eq(SeqSizeT idx, SeqSizeT rank, SymbolT symbol) const
    {
        SeqSizeT full_rank = rank_eq(idx + SeqSizeT{1}, symbol);
        if (rank < full_rank) {
            return select_fw_eq(full_rank - rank - SeqSizeT{1}, symbol);
        }
        else {
            return SelectResult{idx + SeqSizeT{1}, full_rank};
        }
    }

    SelectResult select_bw_neq(SeqSizeT idx, SeqSizeT rank, SymbolT symbol) const
    {
        SeqSizeT full_rank = rank_neq(idx + SeqSizeT{1}, symbol);
        if (rank < full_rank) {
            return select_fw_neq(full_rank - rank - SeqSizeT{1}, symbol);
        }
        else {
            return SelectResult{idx + SeqSizeT{1}, full_rank};
        }
    }

    SelectResult select_bw_gt(SeqSizeT idx, SeqSizeT rank, SymbolT symbol) const
    {
        SeqSizeT full_rank = rank_gt(idx + SeqSizeT{1}, symbol);
        if (rank < full_rank) {
            return select_fw_gt(full_rank - rank - SeqSizeT{1}, symbol);
        }
        else {
            return SelectResult{idx + SeqSizeT{1}, full_rank};
        }
    }

    SelectResult select_bw_ge(SeqSizeT idx, SeqSizeT rank, SymbolT symbol) const
    {
        SeqSizeT full_rank = rank_ge(idx + SeqSizeT{1}, symbol);
        if (rank < full_rank) {
            return select_fw_ge(full_rank - rank - SeqSizeT{1}, symbol);
        }
        else {
            return SelectResult{idx + SeqSizeT{1}, full_rank};
        }
    }

    SelectResult select_bw_lt(SeqSizeT idx, SeqSizeT rank, SymbolT symbol) const
    {
        SeqSizeT full_rank = rank_lt(idx + SeqSizeT{1}, symbol);
        if (rank < full_rank) {
            return select_fw_lt(full_rank - rank - SeqSizeT{1}, symbol);
        }
        else {
            return SelectResult{idx + SeqSizeT{1}, full_rank};
        }
    }

    SelectResult select_bw_le(SeqSizeT idx, SeqSizeT rank, SymbolT symbol) const
    {
        SeqSizeT full_rank = rank_le(idx + SeqSizeT{1}, symbol);
        if (rank < full_rank) {
            return select_fw_le(full_rank - rank - SeqSizeT{1}, symbol);
        }
        else {
            return SelectResult{idx + SeqSizeT{1}, full_rank};
        }
    }




    SeqSizeT rank_eq(SeqSizeT idx, SymbolT symbol) const {
        return rank_fn(idx, symbol, RankEqFn());
    }

    SeqSizeT rank_lt(SeqSizeT idx, SymbolT symbol) const {
        return rank_fn(idx, symbol, RankLtFn());
    }

    SeqSizeT rank_le(SeqSizeT idx, SymbolT symbol) const {
        return rank_fn(idx, symbol, RankLeFn());
    }

    SeqSizeT rank_gt(SeqSizeT idx, SymbolT symbol) const {
        return rank_fn(idx, symbol, RankGtFn());
    }

    SeqSizeT rank_ge(SeqSizeT idx, SymbolT symbol) const {
        return rank_fn(idx, symbol, RankGeFn());
    }

    SeqSizeT rank_neq(SeqSizeT idx, SymbolT symbol) const {
        return rank_fn(idx, symbol, RankNeqFn());
    }


    SeqSizeT rank_eq(SeqSizeT start, size_t end, SymbolT symbol) const {
        SeqSizeT r1 = rank_eq(end, symbol);
        SeqSizeT r0 = rank_eq(start, symbol);
        return r1 - r0;
    }

    SeqSizeT rank_neq(SeqSizeT start, size_t end, SymbolT symbol) const {
        SeqSizeT r1 = rank_neq(end, symbol);
        SeqSizeT r0 = rank_neq(start, symbol);
        return r1 - r0;
    }

    SeqSizeT rank_lt(SeqSizeT start, size_t end, SymbolT symbol) const {
        SeqSizeT r1 = rank_lt(end, symbol);
        SeqSizeT r0 = rank_lt(start, symbol);
        return r1 - r0;
    }

    SeqSizeT rank_le(SeqSizeT start, size_t end, SymbolT symbol) const {
        SeqSizeT r1 = rank_le(end, symbol);
        SeqSizeT r0 = rank_le(start, symbol);
        return r1 - r0;
    }

    SeqSizeT rank_gt(SeqSizeT start, size_t end, SymbolT symbol) const {
        SeqSizeT r1 = rank_gt(end, symbol);
        SeqSizeT r0 = rank_gt(start, symbol);
        return r1 - r0;
    }

    SeqSizeT rank_ge(SeqSizeT start, size_t end, SymbolT symbol) const {
        SeqSizeT r1 = rank_ge(end, symbol);
        SeqSizeT r0 = rank_ge(start, symbol);
        return r1 - r0;
    }





    SeqSizeT rank_eq(SymbolT symbol) const {
        SeqSizeT size = this->size();
        return rank_eq(size, symbol);
    }

    SeqSizeT rank_neq(SymbolT symbol) const {
        SeqSizeT size = this->size();
        return rank_neq(size, symbol);
    }

    SeqSizeT rank_lt(SymbolT symbol) const {
        SeqSizeT size = this->size();
        return rank_lt(size, symbol);
    }

    SeqSizeT rank_le(SymbolT symbol) const {
        SeqSizeT size = this->size();
        return rank_le(size, symbol);
    }

    SeqSizeT rank_gt(SymbolT symbol) const {
        SeqSizeT size = this->size();
        return rank_gt(size, symbol);
    }

    SeqSizeT rank_ge(SymbolT symbol) const {
        SeqSizeT size = this->size();
        return rank_ge(size, symbol);
    }

    template <typename T>
    void ranks(SeqSizeT pos, Span<T> symbols) const
    {
        const Metadata* meta = this->metadata();
        SeqSizeT size = meta->size();

        MEMORIA_V1_ASSERT_TRUE(pos <= size);

        size_t atom_pos;
        SeqSizeT size_sum;
        SeqSizeT rank_sum;

        if (has_index()) {
            const auto* index = this->size_index();
            auto res = index->find_gt(0, pos);
            atom_pos = res.local_pos() * AtomsPerBlock;
            size_sum = res.prefix();

            const auto* sums  = this->sum_index();

            for (size_t c = 0; c < Symbols; c++) {
                symbols[c] += sums->sum(c, res.local_pos());
            }
        }
        else {
            atom_pos = 0;
            size_sum = 0;
            rank_sum = 0;
        }

        auto ii = this->iterator(atom_pos);

        ii.do_while([&](const SymbolsRunT& run){
            SeqSizeT size = run.full_run_length();
            if (pos < size_sum + size) {
                run.ranks(pos - size_sum, symbols);
                return false;
            }
            else {
                size_sum += size;
                run.full_ranks(symbols);
                return true;
            }
        });
    }



    SeqSizeT count_fw(SeqSizeT idx, SymbolT symbol) const
    {
        SeqSizeT rank = rank_neq(idx, symbol);
        SeqSizeT next_idx = select_fw_neq(rank, symbol).idx;
        return next_idx - idx;
    }

    SeqSizeT count_bw(SeqSizeT idx, SymbolT symbol) const
    {
        SeqSizeT rank = rank_neq(idx, symbol);
        if (rank) {
            SeqSizeT next_idx = select_fw_neq(rank - SeqSizeT{1}, symbol).idx;
            return idx + SeqSizeT{1} - next_idx;
        }
        else {
            return idx + SeqSizeT{1};
        }
    }



    VoidResult generateDataEvents(IBlockDataEventHandler* handler) const noexcept
    {
        handler->startGroup("PACKED_RLE_SEQUENCE");
        auto meta = this->metadata();

        handler->value("SIZE", &meta->size());
        handler->value("DATA_SIZE", &meta->data_size());

        if (has_index())
        {
            handler->startGroup("INDEXES");
            MEMORIA_TRY_VOID(size_index()->generateDataEvents(handler));
            MEMORIA_TRY_VOID(sum_index()->generateDataEvents(handler));
            handler->endGroup();
        }

        handler->startGroup("SYMBOL RUNS", size());

        Iterator iter = this->iterator();

        DebugCounter = 1;

        size_t offset{};
        iter.for_each([&](const SymbolsRunT& run){
            U8String str = format_u8("{} :: {}", offset, run);
            handler->value("RUN", BlockValueProviderT<U8String>(str));
            offset += run.full_run_length();
        });

        handler->endGroup();

        handler->endGroup();

        return VoidResult::of();
    }

    template <typename SerializationData>
    VoidResult serialize(SerializationData& buf) const noexcept
    {
        MEMORIA_TRY_VOID(Base::serialize(buf));

        const Metadata* meta = this->metadata();

        FieldFactory<SeqSizeT>::serialize(buf, meta->size());
        FieldFactory<uint64_t>::serialize(buf, meta->data_size());

        if (has_index())
        {
            MEMORIA_TRY_VOID(size_index()->serialize(buf));
            MEMORIA_TRY_VOID(sum_index()->serialize(buf));
        }

        FieldFactory<CodeUnitT>::serialize(buf, symbols(), meta->data_size());

        return VoidResult::of();
    }


    template <typename DeserializationData>
    VoidResult deserialize(DeserializationData& buf) noexcept
    {
        MEMORIA_TRY_VOID(Base::deserialize(buf));

        Metadata* meta = this->metadata();

        FieldFactory<SeqSizeT>::deserialize(buf, meta->size());
        FieldFactory<uint64_t>::deserialize(buf, meta->data_size());

        if (has_index()) {
            MEMORIA_TRY_VOID(size_index()->deserialize(buf));
            MEMORIA_TRY_VOID(sum_index()->deserialize(buf));
        }

        FieldFactory<CodeUnitT>::deserialize(buf, symbols(), meta->data_size());

        return VoidResult::of();
    }


    auto find_run(SeqSizeT symbol_pos) const
    {
        return find_run(this->metadata(), symbol_pos);
    }


    VoidResult insert_io_substream(int32_t at, const io::IOSubstream& substream, int32_t start, int32_t size) noexcept
    {
//        const MyType* buffer = ptr_cast<const MyType>(io::substream_cast<io::IOSymbolSequence>(substream).buffer());
//        return this->insert_from(at, buffer, start, size);

        return VoidResult::of();
    }


    void configure_io_substream(io::IOSubstream& substream) const
    {
        auto& seq = io::substream_cast<IOSubstreamView>(substream);
        seq.configure(this);
    }


    
private:
    struct Location {
        SymbolsRunT run;
        size_t unit_idx;
        RunSizeT local_symbol_idx;
    };


    auto find_run(const Metadata* meta, SeqSizeT symbol_pos) const noexcept
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

                uint64_t local_pos  = symbol_pos - find_result.prefix();
                size_t block_offset = find_result.local_pos() * AtomsPerBlock;

                return locate_run(meta, block_offset, local_pos, find_result.prefix());
            }
        }
        else {
            //return Location(meta->data_size(), 0, 0, meta->data_size(), symbol_pos, RLESymbolsRun(), true);
            return Location{};
        }
    }

    Location locate_run(const Metadata* meta, size_t start, SeqSizeT local_idx, size_t size_base) const noexcept
    {
        auto ii = iterator(start);

        SeqSizeT sum{};
        while (!ii.is_eos())
        {
            SymbolsRunT run = ii.get();

            if (run) {
                SeqSizeT offset = local_idx - sum;
                if (local_idx > sum + SeqSizeT{run.full_run_length()})
                {
                    sum += run.full_run_length();
                    ii.next();
                }
                else {
                    return Location{run, ii.idx(), (RunSizeT)offset};
                }
            }
            else if (run.is_padding()) {
                ii.next(run.run_length());
            }
            else {
                break;
            }
        }

        return Location{};
    }

    Location locate_last_run(const Metadata* meta) const noexcept
    {
        SymbolsRunT last;
        size_t last_unit_idx{};

        size_t remainder = meta->data_size() % SegmentSizeInAtoms;

        if (remainder == 0 && meta->data_size() > 0) {
            remainder = SegmentSizeInAtoms;
        }

        size_t segment_start = meta->data_size() - remainder;
        auto ii = iterator(segment_start);

        ii.for_each([&](const SymbolsRunT& run, size_t offset){
            last = run;
            last_unit_idx = offset;
        });

        if (last) {
            return Location{last, last_unit_idx, last.full_run_length()};
        }
        else {
            return Location{};
        }
    }

    struct FindInSymsResult {
        size_t run_idx;
        RunSizeT local_offset;
        SeqSizeT offset;
    };

    FindInSymsResult find_in_syms(Span<const SymbolsRunT> runs, SeqSizeT pos) const
    {
        SeqSizeT offset{};
        size_t c;
        for (c = 0; c < runs.size(); c++)
        {
            SeqSizeT len = runs[c].full_run_length();
            if (pos < offset + len) {
                break;
            }
            else {
                offset += len;
            }
        }

        return FindInSymsResult{c, (RunSizeT)(pos - offset), offset};
    }
};


template <typename Types>
struct PackedStructTraits<PkdSSRLESeq<Types>> {
    using SearchKeyDataType = BigInt;

    using AccumType = BigInt;
    using SearchKeyType = BigInt;

    static constexpr PackedDataTypeSize DataTypeSize = PackedDataTypeSize::VARIABLE;

    static constexpr PkdSearchType KeySearchType = PkdSearchType::SUM;
    static constexpr int32_t Indexes = PkdSSRLESeq<Types>::Indexes;
};



}
