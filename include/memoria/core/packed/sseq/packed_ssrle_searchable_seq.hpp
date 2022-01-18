
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
        psize_t size_;
        psize_t data_size_;
    public:
        psize_t& size()                 {return size_;}
        const psize_t& size() const     {return size_;}

        psize_t& data_size()                 {return data_size_;}
        const psize_t& data_size() const     {return data_size_;}
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

    psize_t& size() noexcept {return metadata()->size();}
    const psize_t& size() const noexcept {return metadata()->size();}

    psize_t& data_size() noexcept {return metadata()->data_size();}
    const psize_t& data_size() const noexcept {return metadata()->data_size();}



//    static constexpr RLESymbolsRun decode_run(uint64_t value) noexcept
//    {
//        return rleseq::DecodeRun<Symbols>(value);
//    }

//    static constexpr uint64_t encode_run(int32_t symbol, uint64_t length) noexcept
//    {
//        return rleseq::EncodeRun<Symbols, MaxRunLength>(symbol, length);
//    }


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

        int32_t symbols_block_capacity_aligned = PackedAllocatable::roundUpBytesToAlignmentBlocks(symbols_block_capacity);

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
        // other sections are empty at this moment
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
    VoidResult reindex() noexcept
    {
        ssrleseq::ReindexFn<MyType> reindex_fn;
        return reindex_fn.reindex(*this);
    }

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
                if (res) {
                    return do_append(meta, loc.atom_idx, len, res.get().span());
                }
                else {
                    return MEMORIA_MAKE_GENERIC_ERROR("Can't insert run {} into {} at ", runs[0], loc.run, loc.run.full_run_length());
                }
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

public:

    VoidResult insert(size_t idx, Span<const SymbolsRunT> runs, bool compactify = true) noexcept
    {
        if (runs.size() > 0)
        {
            std::vector<SymbolsRunT> syms;

            Iterator ii = iterator();
            size_t offset{};

            auto rr = ii.for_each([&](const SymbolsRunT& run) -> VoidResult {
                size_t len = run.full_run_length();

                if (idx < offset || idx > offset + len) {
                    syms.push_back(run);
                }
                else if (runs.size() == 1)
                {
                    auto res = RunTraits::insert(run, runs[0], idx - offset);
                    if (res) {
                        for (SymbolsRunT rr: res.get().span()) {
                            syms.push_back(rr);
                        }
                    }
                    else {
                        return MEMORIA_MAKE_GENERIC_ERROR("Can't insert run {} into {} at ", runs[0], run, idx - offset);
                    }
                }
                else {
                    auto res = RunTraits::split(run, idx - offset);
                    if (res) {
                        auto& split = res.get();

                        for (SymbolsRunT rr: split.left.span()) {
                            syms.push_back(rr);
                        }

                        for (SymbolsRunT rr: runs) {
                            syms.push_back(rr);
                        }

                        for (SymbolsRunT rr: split.right.span()) {
                            syms.push_back(rr);
                        }
                    }
                    else {
                        return MEMORIA_MAKE_GENERIC_ERROR("Can't split run {} at ", run, idx - offset);
                    }
                }

                offset += len;

                return VoidResult::of();
            });

            MEMORIA_RETURN_IF_ERROR(rr);

            if (compactify) {
                compactify_runs(syms);
            }

            size_t new_data_size = RunTraits::compute_size(syms, 0);

            auto new_block_size = PackedAllocatable::roundUpBytesToAlignmentBlocks(new_data_size * sizeof(AtomT));
            MEMORIA_TRY_VOID(this->resizeBlock(SYMBOLS, new_block_size));

            Span<AtomT> atoms = symbols();
            RunTraits::write_segments_to(syms, atoms, 0);

            Metadata* meta = metadata();
            meta->data_size() = new_data_size;
            meta->size() = count_symbols(syms);

            MEMORIA_TRY_VOID(shrink_to_data());
            return reindex();
        }
        else {
            return VoidResult::of();
        }
    }

    VoidResult removeSpace(size_t start, size_t end) noexcept {
        return remove(start, end);
    }

    VoidResult remove(size_t start, size_t end, bool compactify = false) noexcept
    {
        auto meta = this->metadata();

        MEMORIA_ASSERT_RTN(start, <=, end);
        MEMORIA_ASSERT_RTN(end, <=, meta->size());

        Iterator ii = iterator();

        std::vector<SymbolsRunT> runs;

        size_t offset{};

        auto is_outer = [&](size_t len){
            return offset + len < start || offset > end;
        };

        auto is_single_run = [&](size_t len){
            return (start < offset + len) && (end <= offset + len);
        };

        auto is_left_range = [&](size_t len){
            return (start >= offset) && (start < offset + len);
        };

        auto is_right_range = [&](size_t len){
            return (end >= offset) && (end <= offset + len);
        };

        auto process_run = [&](const SymbolsRunT& run, size_t start0, size_t end0) -> VoidResult {
            auto res = RunTraits::remove(run, start0, end0);
            if (res) {
                for (auto rr: res.get().span()) {
                    runs.push_back(rr);
                }

                return VoidResult::of();
            }
            else {
                return MEMORIA_MAKE_GENERIC_ERROR("Can remove symbols from run {} {} {}", run, start0, end0);
            }
        };


        while (!ii.is_eos())
        {
            SymbolsRunT run = ii.get();

            if (run)
            {
                size_t len = run.full_run_length();
                if (is_outer(len)) {
                    runs.push_back(run);
                }
                else if (is_single_run(len))
                {
                    MEMORIA_TRY_VOID(process_run(run, start - offset, end - offset));
                }
                else if (is_left_range(len))
                {
                    MEMORIA_TRY_VOID(process_run(run, start - offset, len));
                }
                else if (is_right_range(len))
                {
                    MEMORIA_TRY_VOID(process_run(run, 0, end - offset));
                }
                else {
                    // inner run, just skip it.
                }

                offset += len;
            }
            else if (run.is_padding()) {
                ii.next(run.run_length());
            }
            else {
                break;
            }
        }

        if (compactify)
        {
            return this->compactify();
        }
        else {
            MEMORIA_TRY_VOID(shrink_to_data());
            return reindex();
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

            auto split = RunTraits::split(location.run, location.local_idx);

            if (split)
            {
                auto& result = split.get();

                size_t adjustment = 1;
                std::vector<SymbolsRunT> right_runs = this->iterator(location.atom_idx + adjustment).as_vector();

                size_t left_data_size = RunTraits::compute_size(result.left.span(), location.atom_idx);

                auto new_left_size = PackedAllocatable::roundUpBytesToAlignmentBlocks(left_data_size * sizeof(AtomT));
                MEMORIA_TRY_VOID(this->resizeBlock(SYMBOLS, new_left_size));

                Span<AtomT> left_syms = symbols();
                RunTraits::write_segments_to(result.left, left_syms, location.atom_idx);

                size_t right_atoms_size = RunTraits::compute_size(result.right.span(), right_runs);

                auto right_block_size = MyType::block_size(right_atoms_size * sizeof(AtomT));
                other->init_bs(right_block_size);

                Span<AtomT> right_syms = other->symbols();
                size_t right_data_size = RunTraits::write_segments_to(result.right.span(), right_runs, right_syms);

                other_meta->size() = meta->size() - idx;
                other_meta->data_size() = right_data_size;

                meta->size() = idx;
                meta->data_size() = new_left_size;

                return reindex();
            }
            else {
                return MEMORIA_MAKE_GENERIC_ERROR("Invalid Symbols Run Split: {} {}", location.run, location.local_idx);
            }
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
        this->iterator().read_to(syms);

        compactify_runs(syms);

        size_t new_data_size = RunTraits::compute_size(syms, 0);

        auto new_block_size = PackedAllocatable::roundUpBytesToAlignmentBlocks(new_data_size * sizeof(AtomT));
        MEMORIA_TRY_VOID(this->resizeBlock(SYMBOLS, new_block_size));

        Span<AtomT> atoms = symbols();
        RunTraits::write_segments_to(syms, atoms, 0);

        return other->reindex();
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

//    uint64_t rank(int32_t end, int32_t symbol) const
//    {
//        auto meta = this->metadata();
//        int32_t size = meta->size();

//        MEMORIA_V1_ASSERT_TRUE(end >= 0);
//        MEMORIA_V1_ASSERT_TRUE(symbol >= 0 && symbol < Symbols);

//        if (has_index())
//        {
//            const SumIndex* sum_index  = this->sum_index();

//            if (end < size)
//            {
//                auto location       = find_run(end);
//                auto block          = location.data_pos() / ValuesPerBranch;
//                auto block_start    = block * ValuesPerBranch;

//                auto rank_base      = sum_index->sum(symbol, block);

//                auto block_offset = offsets()[block];

//                auto local_rank = block_rank(meta, block_start + block_offset, end - location.block_base(), symbol);

//                return local_rank + rank_base;
//            }
//            else {
//                return sum_index->sum(symbol);
//            }
//        }
//        else {
//            return block_rank(meta, 0, end, symbol);
//        }
//    }

//    SelectResult selectFW(uint64_t rank, int32_t symbol) const
//    {
//        auto meta    = this->metadata();
//        auto symbols = this->symbols();

//        MEMORIA_V1_ASSERT_TRUE(symbol >= 0 && symbol < Symbols);

//        if (has_index())
//        {
//            const SumIndex* sum_index   = this->sum_index();
//            auto find_result            = sum_index->find_ge(symbol, rank);
//            int32_t blocks                  = sum_index->size();

//            if (find_result.local_pos() < blocks)
//            {
//                auto block_start   = find_result.local_pos() * ValuesPerBranch;
//                auto block_offset  = offsets()[find_result.local_pos()];
//                uint64_t local_rank = rank - find_result.prefix();

//                auto block_size_start  = this->size_index()->sum(0, find_result.local_pos());

//                auto result = block_select(meta, symbols, block_start + block_offset, local_rank, block_size_start, symbol);

//                result.rank() += find_result.prefix();

//                return result;
//            }
//            else {
//                return SelectResult(meta->size(), find_result.prefix(), false);
//            }
//        }
//        else {
//            return block_select(meta, symbols, 0, rank, 0, symbol);
//        }
//    }

    SelectResult selectBW(uint64_t rank, int32_t symbol) const
    {
        return selectBW(size(), rank, symbol);
    }


    SelectResult selectFW(int32_t pos, uint64_t rank, int32_t symbol) const
    {
        MEMORIA_ASSERT(rank, >=, 0);
        MEMORIA_ASSERT(pos, >=, -1);

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

        MEMORIA_ASSERT(rank, >=, 1);
        MEMORIA_ASSERT(pos, >=, 0);
        MEMORIA_ASSERT(pos, <=, meta->size());

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



//    rleseq::CountResult countFW(int32_t start_pos) const
//    {
//        auto location = find_run(start_pos);
//        return block_count_fw(metadata(), symbols(), location);
//    }

//    rleseq::CountResult countBW(int32_t start_pos) const
//    {
//        MEMORIA_ASSERT(start_pos, >=, 0);
//        return block_count_bw(metadata(), symbols(), start_pos);
//    }

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
