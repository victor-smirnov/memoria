
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

#include <memoria/core/packed/tools/packed_allocator.hpp>
#include <memoria/core/packed/tree/fse/packed_fse_quick_tree.hpp>

#include <memoria/core/tools/static_array.hpp>
#include <memoria/core/tools/optional.hpp>
#include <memoria/core/bignum/primitive_codec.hpp>

#include <memoria/profiles/common/block_operations.hpp>

#include <memoria/core/packed/sseq/packed_ssrle_searchable_seq_so.hpp>

#include <memoria/core/memory/ptr_cast.hpp>

#include <memoria/core/ssrle/ssrle.hpp>

#include "ssrleseq/ssrleseq_reindex_fn.hpp"
#include "ssrleseq/ssrleseq_iterator.hpp"


#include <ostream>

namespace memoria {

namespace io {

template <int32_t AlphabetSize> class PackedSSRLESymbolSequence;
template <int32_t AlphabetSize> class PackedSSRLESymbolSequenceView;

}



namespace detail_ {
    static constexpr int32_t SSSymbolsRange(int32_t symbols) {
        return 0; // No ranges defined at the moment
    }

    template <int32_t Symbols, int32_t SymbolsRange = SSSymbolsRange(Symbols)>
    struct SSSumIndexFactory: HasType<PkdFQTreeT<int64_t, Symbols>> {};
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
    using SegmentT    = typename RunTraits::SegmentT;
    using CodeUnitT   = typename RunTraits::CodeUnitT;
    using AtomT       = typename RunTraits::AtomT;

    using Iterator = ssrleseq::SymbolsRunIterator<MyType>;

    static constexpr int32_t BytesPerBlock              = Types::BytesPerBlock;
    static constexpr int32_t BytesPerBlockLog2          = NumberOfBits(Types::BytesPerBlock);
    static constexpr int32_t AtomsPerBlock              = BytesPerBlock / sizeof(typename RunTraits::AtomT);
    static constexpr size_t  SegmentSizeInAtoms         = RunTraits::SEGMENT_SIZE_UNITS;


    static_assert(BytesPerBlock % 8 == 0, "Must be multiple of 8 bytes");

    enum {
        METADATA, SIZE_INDEX, SUM_INDEX, SYMBOLS, TOTAL_SEGMENTS_
    };



    using Base::clear;

    using Value = uint8_t;
    using IndexValue    = int64_t;
    using IndexDataType = int64_t;


    using Values = core::StaticVector<int64_t, Indexes>;

    using SumIndex  = typename detail_::SSSumIndexFactory<Symbols>::Type;
    using SizeIndex = PkdFQTreeT<int64_t, 1>;

    class Metadata {
        uint64_t size_;
        uint64_t data_size_;
    public:
        uint64_t& size()                 {return size_;}
        const uint64_t& size() const     {return size_;}

        uint64_t& data_size()                 {return data_size_;}
        const uint64_t& data_size() const     {return data_size_;}
    };

    using SizesT = core::StaticVector<int32_t, 1>;

    struct Tools {};

    using GrowableIOSubstream = io::PackedSSRLESymbolSequence<BitsPerSymbol>;
    using IOSubstreamView     = io::PackedSSRLESymbolSequenceView<BitsPerSymbol>;

    using ExtData = std::tuple<>;
    using SparseObject = PackedSSRLESeqSO<ExtData, MyType>;


    static constexpr int32_t number_of_indexes(int32_t capacity) {
        return capacity <= BytesPerBlock ? 0 : divUp(capacity, BytesPerBlock);
    }

    static constexpr size_t divUp(size_t value, size_t divisor) {
        return (value / divisor) + ((value % divisor) ? 1 : 0);
    }

public:
    PkdSSRLESeq() = default;

    uint64_t& size() noexcept {return metadata()->size();}
    const uint64_t& size() const noexcept {return metadata()->size();}

    uint64_t& data_size() noexcept {return metadata()->data_size();}
    const uint64_t& data_size() const noexcept {return metadata()->data_size();}


    // ====================================== Accessors ================================= //

    Metadata* metadata() noexcept {
        return Base::template get<Metadata>(METADATA);
    }

    const Metadata* metadata() const noexcept {
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

    Span<AtomT> symbols() noexcept {
        return span<AtomT>(this, SYMBOLS);
    }

    Span<const AtomT> symbols() const noexcept {
        return span<AtomT>(this, SYMBOLS);
    }

    int32_t symbols_block_capacity() const noexcept
    {
        return symbols_block_capacity(metadata());
    }

    int32_t symbols_block_capacity(const Metadata* meta) const noexcept
    {
        return symbols_block_size() - meta->data_size();
    }

    int32_t symbols_block_size() const noexcept
    {
        return this->element_size(SYMBOLS);
    }

    bool has_index() const noexcept {
        return Base::element_size(SIZE_INDEX) > 0;
    }


    Tools tools() const noexcept {
        return Tools();
    }

    Iterator iterator() const noexcept {
        return Iterator(span<AtomT>(this, SYMBOLS));
    }

    Iterator iterator(size_t atom_idx) const noexcept
    {
        auto syms = span<AtomT>(this, SYMBOLS);
        return Iterator(syms, atom_idx);
    }

    Iterator block_iterator(size_t block_num) const noexcept
    {
        auto syms = span<AtomT>(this, SYMBOLS);
        return Iterator(syms, block_num * AtomsPerBlock);
    }


public:

    // ===================================== Allocation ================================= //

    using PackedAllocator::block_size;

    int32_t block_size(const MyType* other) const
    {
        return block_size(this->symbols_block_size() + other->symbols_block_size());
    }

    static int32_t block_size(int32_t symbols_block_capacity)
    {
        int32_t metadata_length     = PackedAllocatable::roundUpBytesToAlignmentBlocks(sizeof(Metadata));

        int32_t symbols_block_capacity_aligned = PackedAllocatable::roundUpBytesToAlignmentBlocks(
                    symbols_block_capacity
        );

        int32_t index_size          = number_of_indexes(symbols_block_capacity_aligned);

        int32_t size_index_length   = index_size > 0 ? SizeIndex::block_size(index_size) : 0;
        int32_t sum_index_length    = index_size > 0 ? SumIndex::block_size(index_size) : 0;

        int32_t block_size          = Base::block_size(
                    metadata_length
                    + size_index_length
                    + sum_index_length
                    + symbols_block_capacity_aligned,
                    TOTAL_SEGMENTS_);

        return block_size;
    }

    VoidResult init(int32_t block_size) noexcept
    {
        MEMORIA_ASSERT_RTN(block_size, >=, empty_size());
        return init();
    }

    VoidResult init() noexcept {
        return init_bs(empty_size());
    }

    VoidResult init_bs(int32_t block_size) noexcept
    {
        MEMORIA_TRY_VOID(Base::init(block_size, TOTAL_SEGMENTS_));

        MEMORIA_TRY(meta, Base::template allocate<Metadata>(METADATA));

        meta->size() = 0;
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

        meta->size()        = 0;
        meta->data_size()   = 0;

        return VoidResult::of();
    }

    void reset() noexcept {
        auto meta = this->metadata();

        meta->size()        = 0;
        meta->data_size()   = 0;
    }


public:
    static constexpr int32_t default_size(int32_t available_space) noexcept
    {
        return empty_size();
    }

    VoidResult init_default(int32_t block_size) noexcept {
        return init();
    }


    static int32_t empty_size()
    {
        int32_t metadata_length     = PackedAllocatable::roundUpBytesToAlignmentBlocks(sizeof(Metadata));
        int32_t size_index_length   = 0;
        int32_t sum_index_length    = 0;
        int32_t symbols_block_capacity = 0;

        int32_t block_size          = Base::block_size(
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


    VoidResult createIndex(int32_t index_size) noexcept
    {
        int32_t size_index_block_size = SizeIndex::block_size(index_size);
        MEMORIA_TRY_VOID(Base::resizeBlock(SIZE_INDEX, size_index_block_size));

        int32_t sum_index_block_size = SumIndex::block_size(index_size);
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

    MMA_PKD_OOM_SAFE
    VoidResult shrinkData(int32_t length) noexcept
    {
        int32_t current_size = this->element_size(SYMBOLS);

        int32_t new_size = current_size - length;

        if (new_size < 0)
        {
            MEMORIA_TRY_VOID(Base::resizeBlock(SYMBOLS, new_size));
        }

        return VoidResult::of();
    }


    MMA_PKD_OOM_SAFE
    VoidResult shrink_to_data() noexcept
    {
        return shrinkData(this->symbols_block_capacity());
    }

    size_t count_symbols(Span<const SymbolsRunT> runs) noexcept
    {
        size_t sum = 0;
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
        if (meta->size() > 0)
        {
            Location loc = locate_last_run(meta);
            size_t len = loc.run.full_run_length();

            if (runs.size() == 1)
            {
                auto res = RunTraits::insert(loc.run, runs[0], len);                
                return do_append(meta, loc.atom_idx, len, res.span());
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

    Result<size_t> do_append(Metadata* meta, size_t start, size_t run_len0, Span<const SymbolsRunT> runs) noexcept
    {
        using ResultT = Result<size_t>;

        size_t data_size = RunTraits::compute_size(runs, start);
        size_t syms_size = element_size(SYMBOLS) / sizeof (AtomT);
        if (data_size > syms_size)
        {
            size_t bs = data_size * sizeof(AtomT);
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

                return ResultT::of(new_syms_size * sizeof(AtomT));
            }
        }

        Span<AtomT> syms = symbols();
        RunTraits::write_segments_to(runs, syms, start);

        meta->data_size() = data_size;
        meta->size() -= run_len0;
        meta->size() += count_symbols(runs);

        MEMORIA_TRY_VOID(reindex());

        return ResultT::of(size_t{});
    }

    template <typename T1, typename T2>
    void append_all(std::vector<T1>& vv, Span<T2> span) const {
        vv.insert(vv.end(), span.begin(), span.end());
    }

    UInt64Result split_buffer(
            std::vector<SymbolsRunT>& left,
            std::vector<SymbolsRunT>& right,
            uint64_t pos) const noexcept
    {
        std::vector<SymbolsRunT> runs = iterator().as_vector();
        return split_buffer(runs, left, right, pos);
    }

    UInt64Result split_buffer(
            const std::vector<SymbolsRunT>& runs,
            std::vector<SymbolsRunT>& left,
            std::vector<SymbolsRunT>& right,
            uint64_t pos) const noexcept
    {
        uint64_t size;

        auto res = find_in_syms(runs, pos);
        size = res.offset;

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

        return UInt64Result::of(size);
    }



public:

    VoidResult insert(uint64_t idx, Span<const SymbolsRunT> runs) noexcept
    {
        std::vector<SymbolsRunT> left;
        std::vector<SymbolsRunT> right;

        uint64_t size = this->size();

        if (idx <= size)
        {
            MEMORIA_TRY_VOID(split_buffer(left, right, idx));

            append_all(left, runs);
            append_all(left, to_const_span(right));

            compactify_runs(left);

            size_t new_data_size = RunTraits::compute_size(left, 0);

            MEMORIA_TRY_VOID(resizeBlock(SYMBOLS, new_data_size * sizeof(AtomT)));

            Span<AtomT> atoms = symbols();
            RunTraits::write_segments_to(left, atoms, 0);

            Metadata* meta = metadata();
            meta->data_size() = new_data_size;
            meta->size() += count_symbols(runs);

            MEMORIA_TRY_VOID(shrink_to_data());
            return do_reindex(to_span(left));
        }
        else {
            return MEMORIA_MAKE_GENERIC_ERROR("Range check in SSRLE sequence insert: idx={}, size={}", idx, size);
        }
    }

    VoidResult removeSpace(size_t start, size_t end) noexcept {
        return remove(start, end);
    }

    VoidResult remove(size_t start, size_t end, bool compactify = false) noexcept
    {
        if (end - start > 0)
        {
            auto meta = this->metadata();

            MEMORIA_ASSERT_RTN(start, <=, end);
            MEMORIA_ASSERT_RTN(end, <=, meta->size());

            std::vector<SymbolsRunT> runs = iterator().as_vector();

            std::vector<SymbolsRunT> runs_res;

            if (end < meta->size()) {
                if (start > 0) {
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
                if (start > 0) {
                    std::vector<SymbolsRunT> right;
                    MEMORIA_TRY_VOID(split_buffer(runs, runs_res, right, start));
                }
                else {
                    return clear();
                }
            }

            compactify_runs(runs_res);

            size_t new_data_size = RunTraits::compute_size(runs_res, 0);
            MEMORIA_TRY_VOID(resizeBlock(SYMBOLS, new_data_size * sizeof(AtomT)));

            Span<AtomT> atoms = symbols();
            RunTraits::write_segments_to(runs_res, atoms, 0);

            meta->data_size() = new_data_size;
            meta->size() -= end - start;

            return do_reindex(to_span(runs_res));
        }
        else {
            return VoidResult::of();
        }
    }

    VoidResult compactify()
    {
        std::vector<SymbolsRunT> syms = this->iterator().as_vector();

        compactify_runs(syms);

        size_t new_data_size = RunTraits::compute_size(syms, 0);

        auto new_block_size = PackedAllocatable::roundUpBytesToAlignmentBlocks(new_data_size * sizeof(AtomT));
        MEMORIA_TRY_VOID(this->resizeBlock(SYMBOLS, new_block_size));

        Span<AtomT> atoms = symbols();
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
    VoidResult splitTo(MyType* other, size_t idx) noexcept
    {
        Metadata* meta = this->metadata();

        if (idx < meta->size())
        {
            Metadata* other_meta = other->metadata();

            auto location = this->find_run(idx);

            auto result = RunTraits::split(location.run, location.local_idx);

            size_t adjustment = location.run.atom_size();
            std::vector<SymbolsRunT> right_runs = this->iterator(location.atom_idx + adjustment).as_vector();

            size_t left_data_size = RunTraits::compute_size(result.left.span(), location.atom_idx);

            MEMORIA_TRY_VOID(this->resizeBlock(SYMBOLS, left_data_size * sizeof(AtomT)));

            Span<AtomT> left_syms = symbols();
            RunTraits::write_segments_to(result.left.span(), left_syms, location.atom_idx);

            size_t right_atoms_size = RunTraits::compute_size(result.right.span(), right_runs);

            auto right_block_size = MyType::block_size(right_atoms_size * sizeof(AtomT));
            MEMORIA_TRY_VOID(other->init_bs(right_block_size));
            MEMORIA_TRY_VOID(other->resizeBlock(SYMBOLS, right_atoms_size * sizeof(AtomT)));

            Span<AtomT> right_syms = other->symbols();
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

        auto new_block_size = new_data_size * sizeof(AtomT);
        MEMORIA_TRY_VOID(other->resizeBlock(SYMBOLS, new_block_size));

        Span<AtomT> atoms = other->symbols();
        RunTraits::write_segments_to(syms, atoms, 0);

        other_meta->size() += meta->size();
        other_meta->data_size() = new_data_size;

        return other->do_reindex(to_span(syms));
    }

    size_t access(uint64_t pos) const
    {
        const Metadata* meta = this->metadata();
        size_t size = meta->size();

        MEMORIA_V1_ASSERT_TRUE(pos < size);

        size_t atom_pos;
        uint64_t sum;

        if (has_index()) {
            const auto* index = this->size_index();
            auto res = index->find_gt(0, pos);
            atom_pos = res.local_pos() * AtomsPerBlock;
            sum = res.prefix();
        }
        else {
            atom_pos = 0;
            sum = 0;
        }

        auto ii = this->iterator(atom_pos);

        SymbolsRunT tgt;
        ii.do_while([&](const SymbolsRunT& run){
            size_t size = run.full_run_length();
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
        size_t idx;
        uint64_t rank;
    };


private:
    using FindResult = typename SumIndex::WalkerBase;

    struct SelectFwEqFn {
        FindResult index_fn(const SumIndex* index, uint64_t rank, size_t symbol) const {
            return index->find_ge(symbol, rank);
        }

        uint64_t full_rank_fn(const SymbolsRunT& run, size_t symbol) const {
            return run.full_rank_eq(symbol);
        }

        size_t select_fn(const SymbolsRunT& run, uint64_t rank, size_t symbol) const {
            return run.select_fw_eq(rank, symbol);
        }
    };

    template<typename Fn>
    struct SelectFwFnBase {
        FindResult index_fn(const SumIndex* index, uint64_t rank, int32_t symbol) const
        {
            uint64_t sum{};
            int32_t size = index->size();
            int32_t c;

            for (c = 0; c < size; c++) {
                uint64_t value = self().sum(index, c, symbol);
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
        uint64_t sum(const SumIndex* index, int32_t idx, int32_t symbol) const {
            uint64_t sum{};
            for (int32_t c = 0; c < symbol; c++) {
                sum += index->value(c, idx);
            }
            return sum;
        }

        uint64_t full_rank_fn(const SymbolsRunT& run, size_t symbol) const {
            return run.full_rank_lt(symbol);
        }

        size_t select_fn(const SymbolsRunT& run, uint64_t rank, size_t symbol) const {
            return run.select_fw_lt(rank, symbol);
        }
    };

    struct SelectFwLeFn: SelectFwFnBase<SelectFwLeFn> {
        uint64_t sum(const SumIndex* index, int32_t idx, int32_t symbol) const {
            uint64_t sum{};
            for (int32_t c = 0; c <= symbol; c++) {
                sum += index->value(c, idx);
            }
            return sum;
        }

        uint64_t full_rank_fn(const SymbolsRunT& run, size_t symbol) const {
            return run.full_rank_le(symbol);
        }

        size_t select_fn(const SymbolsRunT& run, uint64_t rank, size_t symbol) const {
            return run.select_fw_le(rank, symbol);
        }
    };

    struct SelectFwGtFn: SelectFwFnBase<SelectFwGtFn> {
        uint64_t sum(const SumIndex* index, int32_t idx, int32_t symbol) const {
            uint64_t sum{};
            for (int32_t c = symbol + 1; c < Symbols; c++) {
                sum += index->value(c, idx);
            }
            return sum;
        }

        uint64_t full_rank_fn(const SymbolsRunT& run, size_t symbol) const {
            return run.full_rank_gt(symbol);
        }

        size_t select_fn(const SymbolsRunT& run, uint64_t rank, size_t symbol) const {
            return run.select_fw_gt(rank, symbol);
        }
    };

    struct SelectFwGeFn: SelectFwFnBase<SelectFwGeFn> {
        uint64_t sum(const SumIndex* index, int32_t idx, int32_t symbol) const {
            uint64_t sum{};
            for (int32_t c = symbol; c < Symbols; c++) {
                sum += index->value(c, idx);
            }
            return sum;
        }

        uint64_t full_rank_fn(const SymbolsRunT& run, size_t symbol) const {
            return run.full_rank_ge(symbol);
        }

        size_t select_fn(const SymbolsRunT& run, uint64_t rank, size_t symbol) const {
            return run.select_fw_ge(rank, symbol);
        }
    };

    struct SelectFwNeqFn: SelectFwFnBase<SelectFwNeqFn> {
        uint64_t sum(const SumIndex* index, int32_t idx, int32_t symbol) const {
            uint64_t sum{};
            for (int32_t c = 0; c < Symbols; c++) {
                sum += c != symbol ? index->value(c, idx) : 0;
            }
            return sum;
        }

        uint64_t full_rank_fn(const SymbolsRunT& run, size_t symbol) const {
            return run.full_rank_neq(symbol);
        }

        size_t select_fn(const SymbolsRunT& run, uint64_t rank, size_t symbol) const {
            return run.select_fw_neq(rank, symbol);
        }
    };



    template <typename Fn>
    SelectResult select_fw_fn(uint64_t rank, size_t symbol, Fn&& fn) const
    {
        MEMORIA_V1_ASSERT_TRUE(symbol < Symbols);

        size_t atom_pos;
        uint64_t rank_sum;
        uint64_t size_sum;

        if (has_index()) {
            const auto* sums = this->sum_index();

            auto res = fn.index_fn(sums, rank, symbol);
            if (res.idx() < sums->size())
            {
                atom_pos = res.idx() * AtomsPerBlock;
                rank_sum = res.prefix();

                const auto* sizes  = this->size_index();
                size_sum = sizes->sum(0, res.idx());
            }
            else {
                return SelectResult{size(), (uint64_t)res.prefix()};
            }
        }
        else {
            atom_pos = 0;
            rank_sum = 0;
            size_sum = 0;
        }

        auto ii = this->iterator(atom_pos);

        bool res{};
        size_t idx;

        ii.do_while([&](const SymbolsRunT& run) {
            uint64_t run_rank = fn.full_rank_fn(run, symbol);
            if (MMA_UNLIKELY(rank < rank_sum + run_rank)) {
                idx = fn.select_fn(run, rank - rank_sum, symbol);
                res = true;
                return false;
            }
            else {
                rank_sum += run_rank;
                size_sum += run.full_run_length();
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
        uint64_t sum_fn(const SumIndex* index, int32_t idx, int32_t symbol) const {
            return index->sum(symbol, idx);
        }

        uint64_t sum_all_fn(const SumIndex* index, int32_t symbol) const {
            return index->sum(symbol);
        }

        uint64_t rank_fn(const SymbolsRunT& run, uint64_t idx, size_t symbol) const {
            return run.rank_eq(idx, symbol);
        }

        uint64_t full_rank_fn(const SymbolsRunT& run, size_t symbol) const {
            return run.full_rank_eq(symbol);
        }
    };



    struct RankLtFn {
        uint64_t sum_fn(const SumIndex* index, int32_t idx, int32_t symbol) const
        {
            uint64_t sum{};
            for (int32_t c = 0; c < symbol; c++) {
                sum += index->sum(c, idx);
            }
            return sum;
        }

        uint64_t sum_all_fn(const SumIndex* index, int32_t symbol) const {
            return sum_fn(index, index->size(), symbol);
        }

        uint64_t rank_fn(const SymbolsRunT& run, uint64_t idx, size_t symbol) const {
            return run.rank_lt(idx, symbol);
        }

        uint64_t full_rank_fn(const SymbolsRunT& run, size_t symbol) const {
            return run.full_rank_lt(symbol);
        }
    };

    struct RankLeFn {
        uint64_t sum_fn(const SumIndex* index, int32_t idx, int32_t symbol) const
        {
            uint64_t sum{};
            for (int32_t c = 0; c <= symbol; c++) {
                sum += index->sum(c, idx);
            }
            return sum;
        }

        uint64_t sum_all_fn(const SumIndex* index, int32_t symbol) const {
            return sum_fn(index, index->size(), symbol);
        }

        uint64_t rank_fn(const SymbolsRunT& run, uint64_t idx, size_t symbol) const {
            return run.rank_le(idx, symbol);
        }

        uint64_t full_rank_fn(const SymbolsRunT& run, size_t symbol) const {
            return run.full_rank_le(symbol);
        }
    };

    struct RankGtFn {
        uint64_t sum_fn(const SumIndex* index, int32_t idx, int32_t symbol) const
        {
            uint64_t sum{};
            for (int32_t c = symbol + 1; c < Symbols; c++) {
                sum += index->sum(c, idx);
            }
            return sum;
        }

        uint64_t sum_all_fn(const SumIndex* index, int32_t symbol) const {
            return sum_fn(index, index->size(), symbol);
        }

        uint64_t rank_fn(const SymbolsRunT& run, uint64_t idx, size_t symbol) const {
            return run.rank_gt(idx, symbol);
        }

        uint64_t full_rank_fn(const SymbolsRunT& run, size_t symbol) const {
            return run.full_rank_gt(symbol);
        }
    };

    struct RankGeFn {
        uint64_t sum_fn(const SumIndex* index, int32_t idx, int32_t symbol) const
        {
            uint64_t sum{};
            for (int32_t c = symbol; c < Symbols; c++) {
                sum += index->sum(c, idx);
            }
            return sum;
        }

        uint64_t sum_all_fn(const SumIndex* index, int32_t symbol) const {
            return sum_fn(index, index->size(), symbol);
        }

        uint64_t rank_fn(const SymbolsRunT& run, uint64_t idx, size_t symbol) const {
            return run.rank_ge(idx, symbol);
        }

        uint64_t full_rank_fn(const SymbolsRunT& run, size_t symbol) const {
            return run.full_rank_ge(symbol);
        }
    };

    struct RankNeqFn {
        uint64_t sum_fn(const SumIndex* index, int32_t idx, int32_t symbol) const
        {
            uint64_t sum{};
            for (int32_t c = 0; c < Symbols; c++) {
                sum += c != symbol ? index->sum(c, idx) : 0;
            }
            return sum;
        }

        uint64_t sum_all_fn(const SumIndex* index, int32_t symbol) const {
            return sum_fn(index, index->size(), symbol);
        }

        uint64_t rank_fn(const SymbolsRunT& run, uint64_t idx, size_t symbol) const {
            return run.rank_neq(idx, symbol);
        }

        uint64_t full_rank_fn(const SymbolsRunT& run, size_t symbol) const {
            return run.full_rank_neq(symbol);
        }
    };


    template <typename Fn>
    uint64_t rank_fn(size_t pos, size_t symbol, Fn&& fn) const
    {
        if (MMA_UNLIKELY(pos == 0)) {
            return 0;
        }

        const Metadata* meta = this->metadata();
        size_t size = meta->size();

        MEMORIA_V1_ASSERT_TRUE(pos <= size && symbol < Symbols);

        size_t atom_pos;
        uint64_t size_sum;
        uint64_t rank_sum;

        if (has_index()) {
            const auto* index = this->size_index();

            if (pos < size)
            {
                auto res = index->find_gt(0, pos);
                atom_pos = res.local_pos() * AtomsPerBlock;
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
            atom_pos = 0;
            size_sum = 0;
            rank_sum = 0;
        }

        auto ii = this->iterator(atom_pos);

        ii.do_while([&](const SymbolsRunT& run){
            size_t size = run.full_run_length();
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
    SelectResult select_fw(uint64_t rank, size_t symbol, SeqOpType op_type) const {
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

    SelectResult select_fw(uint64_t idx, uint64_t rank, size_t symbol, SeqOpType op_type) const {
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

    SelectResult select_bw(uint64_t rank, size_t symbol, SeqOpType op_type) const {
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

    SelectResult select_bw(uint64_t idx, uint64_t rank, size_t symbol, SeqOpType op_type) const {
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


    SelectResult rank(uint64_t pos, size_t symbol, SeqOpType op_type) const {
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

    SelectResult rank(uint64_t start, uint64_t end, size_t symbol, SeqOpType op_type) const {
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

    SelectResult rank(size_t symbol, SeqOpType op_type) const {
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


    SelectResult select_fw_eq(uint64_t rank, size_t symbol) const {
        return select_fw_fn(rank, symbol, SelectFwEqFn());
    }

    SelectResult select_fw_neq(uint64_t rank, size_t symbol) const {
        return select_fw_fn(rank, symbol, SelectFwNeqFn());
    }

    SelectResult select_fw_lt(uint64_t rank, size_t symbol) const {
        return select_fw_fn(rank, symbol, SelectFwLtFn());
    }

    SelectResult select_fw_le(uint64_t rank, size_t symbol) const {
        return select_fw_fn(rank, symbol, SelectFwLeFn());
    }

    SelectResult select_fw_gt(uint64_t rank, size_t symbol) const {
        return select_fw_fn(rank, symbol, SelectFwGtFn());
    }

    SelectResult select_fw_ge(uint64_t rank, size_t symbol) const {
        return select_fw_fn(rank, symbol, SelectFwGeFn());
    }

    SelectResult select_fw_ge(uint64_t idx, uint64_t rank, size_t symbol) const {
        uint64_t rank_base = rank_ge(idx, symbol);
        return select_fw_ge(rank - rank_base, symbol);
    }

    SelectResult select_fw_gt(uint64_t idx, uint64_t rank, size_t symbol) const {
        uint64_t rank_base = rank_gt(idx, symbol);
        return select_fw_gt(rank - rank_base, symbol);
    }

    SelectResult select_fw_le(uint64_t idx, uint64_t rank, size_t symbol) const {
        uint64_t rank_base = rank_le(idx, symbol);
        return select_fw_le(rank - rank_base, symbol);
    }

    SelectResult select_fw_lt(uint64_t idx, uint64_t rank, size_t symbol) const {
        uint64_t rank_base = rank_lt(idx, symbol);
        return select_fw_lt(rank - rank_base, symbol);
    }

    SelectResult select_fw_eq(uint64_t idx, uint64_t rank, size_t symbol) const {
        uint64_t rank_base = rank_eq(idx, symbol);
        return select_fw_eq(rank - rank_base, symbol);
    }

    SelectResult select_fw_neq(uint64_t idx, uint64_t rank, size_t symbol) const {
        uint64_t rank_base = rank_neq(idx, symbol);
        return select_fw_neq(rank - rank_base, symbol);
    }



    SelectResult select_bw_eq(uint64_t rank, size_t symbol) const
    {
        uint64_t size = this->size();
        uint64_t full_rank = rank_eq(size, symbol);
        if (rank < full_rank) {
            return select_fw_eq(full_rank - rank - 1, symbol);
        }
        else {
            return SelectResult{size, full_rank};
        }
    }

    SelectResult select_bw_neq(uint64_t rank, size_t symbol) const
    {
        uint64_t size = this->size();
        uint64_t full_rank = rank_neq(size, symbol);
        if (rank < full_rank) {
            return select_fw_neq(full_rank - rank - 1, symbol);
        }
        else {
            return SelectResult{size, full_rank};
        }
    }

    SelectResult select_bw_lt(uint64_t rank, size_t symbol) const
    {
        uint64_t size = this->size();
        uint64_t full_rank = rank_lt(size, symbol);
        if (rank < full_rank) {
            return select_fw_lt(full_rank - rank - 1, symbol);
        }
        else {
            return SelectResult{size, full_rank};
        }
    }

    SelectResult select_bw_le(uint64_t rank, size_t symbol) const
    {
        uint64_t size = this->size();
        uint64_t full_rank = rank_le(size, symbol);
        if (rank < full_rank) {
            return select_fw_le(full_rank - rank - 1, symbol);
        }
        else {
            return SelectResult{size, full_rank};
        }
    }

    SelectResult select_bw_gt(uint64_t rank, size_t symbol) const
    {
        uint64_t size = this->size();
        uint64_t full_rank = rank_neq(size, symbol);
        if (rank < full_rank) {
            return select_fw_gt(full_rank - rank - 1, symbol);
        }
        else {
            return SelectResult{size, full_rank};
        }
    }

    SelectResult select_bw_ge(uint64_t rank, size_t symbol) const
    {
        uint64_t size = this->size();
        uint64_t full_rank = rank_neq(size, symbol);
        if (rank < full_rank) {
            return select_fw_ge(full_rank - rank - 1, symbol);
        }
        else {
            return SelectResult{size, full_rank};
        }
    }


    SelectResult select_bw_eq(uint64_t idx, uint64_t rank, size_t symbol) const
    {
        uint64_t full_rank = rank_eq(idx + 1, symbol);
        if (rank < full_rank) {
            return select_fw_eq(full_rank - rank - 1, symbol);
        }
        else {
            return SelectResult{idx + 1, full_rank};
        }
    }

    SelectResult select_bw_neq(uint64_t idx, uint64_t rank, size_t symbol) const
    {
        uint64_t full_rank = rank_neq(idx + 1, symbol);
        if (rank < full_rank) {
            return select_fw_neq(full_rank - rank - 1, symbol);
        }
        else {
            return SelectResult{idx + 1, full_rank};
        }
    }

    SelectResult select_bw_gt(uint64_t idx, uint64_t rank, size_t symbol) const
    {
        uint64_t full_rank = rank_gt(idx + 1, symbol);
        if (rank < full_rank) {
            return select_fw_gt(full_rank - rank - 1, symbol);
        }
        else {
            return SelectResult{idx + 1, full_rank};
        }
    }

    SelectResult select_bw_ge(uint64_t idx, uint64_t rank, size_t symbol) const
    {
        uint64_t full_rank = rank_ge(idx + 1, symbol);
        if (rank < full_rank) {
            return select_fw_ge(full_rank - rank - 1, symbol);
        }
        else {
            return SelectResult{idx + 1, full_rank};
        }
    }

    SelectResult select_bw_lt(uint64_t idx, uint64_t rank, size_t symbol) const
    {
        uint64_t full_rank = rank_lt(idx + 1, symbol);
        if (rank < full_rank) {
            return select_fw_lt(full_rank - rank - 1, symbol);
        }
        else {
            return SelectResult{idx + 1, full_rank};
        }
    }

    SelectResult select_bw_le(uint64_t idx, uint64_t rank, size_t symbol) const
    {
        uint64_t full_rank = rank_le(idx + 1, symbol);
        if (rank < full_rank) {
            return select_fw_le(full_rank - rank - 1, symbol);
        }
        else {
            return SelectResult{idx + 1, full_rank};
        }
    }




    uint64_t rank_eq(uint64_t idx, size_t symbol) const {
        return rank_fn(idx, symbol, RankEqFn());
    }

    uint64_t rank_lt(uint64_t idx, size_t symbol) const {
        return rank_fn(idx, symbol, RankLtFn());
    }

    uint64_t rank_le(uint64_t idx, size_t symbol) const {
        return rank_fn(idx, symbol, RankLeFn());
    }

    uint64_t rank_gt(uint64_t idx, size_t symbol) const {
        return rank_fn(idx, symbol, RankGtFn());
    }

    uint64_t rank_ge(uint64_t idx, size_t symbol) const {
        return rank_fn(idx, symbol, RankGeFn());
    }

    uint64_t rank_neq(uint64_t idx, size_t symbol) const {
        return rank_fn(idx, symbol, RankNeqFn());
    }


    uint64_t rank_eq(uint64_t start, size_t end, size_t symbol) const {
        uint64_t r1 = rank_eq(end, symbol);
        uint64_t r0 = rank_eq(start, symbol);
        return r1 - r0;
    }

    uint64_t rank_neq(uint64_t start, size_t end, size_t symbol) const {
        uint64_t r1 = rank_neq(end, symbol);
        uint64_t r0 = rank_neq(start, symbol);
        return r1 - r0;
    }

    uint64_t rank_lt(uint64_t start, size_t end, size_t symbol) const {
        uint64_t r1 = rank_lt(end, symbol);
        uint64_t r0 = rank_lt(start, symbol);
        return r1 - r0;
    }

    uint64_t rank_le(uint64_t start, size_t end, size_t symbol) const {
        uint64_t r1 = rank_le(end, symbol);
        uint64_t r0 = rank_le(start, symbol);
        return r1 - r0;
    }

    uint64_t rank_gt(uint64_t start, size_t end, size_t symbol) const {
        uint64_t r1 = rank_gt(end, symbol);
        uint64_t r0 = rank_gt(start, symbol);
        return r1 - r0;
    }

    uint64_t rank_ge(uint64_t start, size_t end, size_t symbol) const {
        uint64_t r1 = rank_ge(end, symbol);
        uint64_t r0 = rank_ge(start, symbol);
        return r1 - r0;
    }





    uint64_t rank_eq(size_t symbol) const {
        uint64_t size = this->size();
        return rank_eq(size, symbol);
    }

    uint64_t rank_neq(size_t symbol) const {
        uint64_t size = this->size();
        return rank_neq(size, symbol);
    }

    uint64_t rank_lt(size_t symbol) const {
        uint64_t size = this->size();
        return rank_lt(size, symbol);
    }

    uint64_t rank_le(size_t symbol) const {
        uint64_t size = this->size();
        return rank_le(size, symbol);
    }

    uint64_t rank_gt(size_t symbol) const {
        uint64_t size = this->size();
        return rank_gt(size, symbol);
    }

    uint64_t rank_ge(size_t symbol) const {
        uint64_t size = this->size();
        return rank_ge(size, symbol);
    }

    template <typename T>
    void ranks(size_t pos, Span<T> symbols) const
    {
        const Metadata* meta = this->metadata();
        size_t size = meta->size();

        MEMORIA_V1_ASSERT_TRUE(pos <= size);

        size_t atom_pos;
        uint64_t size_sum;
        uint64_t rank_sum;

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
            size_t size = run.full_run_length();
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



    uint64_t count_fw(uint64_t idx, size_t symbol) const
    {
        uint64_t rank = rank_neq(idx, symbol);
        uint64_t next_idx = select_fw_neq(rank, symbol).idx;
        return next_idx - idx;
    }

    uint64_t count_bw(uint64_t idx, size_t symbol) const
    {
        uint64_t rank = rank_neq(idx, symbol);
        if (rank > 0) {
            uint64_t next_idx = select_fw_neq(rank - 1, symbol).idx;
            return idx + 1 - next_idx;
        }
        else {
            return idx + 1;
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

        FieldFactory<int32_t>::serialize(buf, meta->size());
        FieldFactory<int32_t>::serialize(buf, meta->data_size());

        if (has_index())
        {
            MEMORIA_TRY_VOID(size_index()->serialize(buf));
            MEMORIA_TRY_VOID(sum_index()->serialize(buf));
        }

        FieldFactory<Value>::serialize(buf, symbols(), meta->data_size());

        return VoidResult::of();
    }


    template <typename DeserializationData>
    VoidResult deserialize(DeserializationData& buf) noexcept
    {
        MEMORIA_TRY_VOID(Base::deserialize(buf));

        Metadata* meta = this->metadata();

        FieldFactory<int32_t>::deserialize(buf, meta->size());
        FieldFactory<int32_t>::deserialize(buf, meta->data_size());

        if (has_index()) {
            MEMORIA_TRY_VOID(size_index()->deserialize(buf));
            MEMORIA_TRY_VOID(sum_index()->deserialize(buf));
        }

        FieldFactory<Value>::deserialize(buf, symbols(), meta->data_size());

        return VoidResult::of();
    }


    auto find_run(int32_t symbol_pos) const
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
        size_t atom_idx;
        size_t local_idx;
    };


    auto find_run(const Metadata* meta, size_t symbol_pos) const noexcept
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
                size_t block_offset = find_result.local_pos() * AtomsPerBlock;

                return locate_run(meta, block_offset, local_pos, find_result.prefix());
            }
        }
        else {
            //return Location(meta->data_size(), 0, 0, meta->data_size(), symbol_pos, RLESymbolsRun(), true);
            return Location{};
        }
    }

    Location locate_run(const Metadata* meta, size_t start, size_t local_idx, size_t size_base) const noexcept
    {
        auto ii = iterator(start);

        size_t sum{};
        while (!ii.is_eos())
        {
            SymbolsRunT run = ii.get();

            if (run) {
                size_t offset = local_idx - sum;
                if (local_idx > sum + run.full_run_length())
                {
                    sum += run.full_run_length();
                    ii.next();
                }
                else {
                    return Location{run, ii.idx(), offset};
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
        size_t last_atom_idx{};

        size_t remainder = meta->data_size() % SegmentSizeInAtoms;

        if (remainder == 0 && meta->data_size() > 0) {
            remainder = SegmentSizeInAtoms;
        }

        size_t segment_start = meta->data_size() - remainder;
        auto ii = iterator(segment_start);

        ii.for_each([&](const SymbolsRunT& run, size_t offset){
            last = run;
            last_atom_idx = offset;
        });

        if (last) {
            return Location{last, last_atom_idx, last.full_run_length()};
        }
        else {
            return Location{};
        }
    }

    struct FindInSymsResult {
        size_t run_idx;
        uint64_t local_offset;
        uint64_t offset;
    };

    FindInSymsResult find_in_syms(Span<const SymbolsRunT> runs, uint64_t pos) const
    {
        uint64_t offset{};
        size_t c;
        for (c = 0; c < runs.size(); c++)
        {
            uint64_t len = runs[c].full_run_length();
            if (pos < offset + len) {
                break;
            }
            else {
                offset += len;
            }
        }

        return FindInSymsResult{c, pos - offset, offset};
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
