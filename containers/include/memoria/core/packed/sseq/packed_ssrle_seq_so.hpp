
// Copyright 2021-2025 Victor Smirnov
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

#include <memoria/core/types.hpp>

#include <memoria/core/packed/sseq/ssrleseq/ssrleseq_reindex_fn.hpp>
#include <memoria/profiles/common/block_operations.hpp>
#include <memoria/core/tools/bitmap_select.hpp>

#include <memoria/core/packed/tools/packed_allocator_types.hpp>

#include <tuple>

namespace memoria {

template <typename ExtData, typename PkdStruct>
class PackedSSRLESeqSO {
    ExtData* ext_data_;
    PkdStruct* data_;
    std::tuple<> index_ext_data_;

    using MyType = PackedSSRLESeqSO;

    using SumIndexT = typename PkdStruct::SumIndex;
    using SizeIndexT = typename PkdStruct::SizeIndex;

public:
    using RunTraits = typename PkdStruct::RunTraits;

    using SumIndexSO  = typename SumIndexT::SparseObject;
    using SizeIndexSO = typename SizeIndexT::SparseObject;

    using SeqSizeT      = typename PkdStruct::SeqSizeT;
    using SymbolT       = typename PkdStruct::SymbolT;
    using SymbolsRunT   = typename PkdStruct::SymbolsRunT;
    using SumValueT     = typename PkdStruct::SumValueT;
    using RunSizeT      = typename PkdStruct::RunSizeT;
    using CodeUnitT     = typename PkdStruct::CodeUnitT;
    using Metadata      = typename PkdStruct::Metadata;
    using Iterator      = typename PkdStruct::Iterator;

    using UpdateState = PkdStructUpdate<MyType>;

    static constexpr SymbolT AlphabetSize       = PkdStruct::AlphabetSize;
    static constexpr size_t  AtomsPerBlock      = PkdStruct::AtomsPerBlock;
    static constexpr size_t  SegmentSizeInAtoms = PkdStruct::SegmentSizeInAtoms;
    static constexpr size_t  SYMBOLS            = PkdStruct::SYMBOLS;



    using PkdStructT = PkdStruct;

    struct SelectResult {
        SeqSizeT idx;
        SeqSizeT rank;

        SelectResult dec_rank(const SeqSizeT& rr) {
            rank -= rr;
            return *this;
        }

        SelectResult inc_rank(const SeqSizeT& rr) {
            rank += rr;
            return *this;
        }
    };



    PackedSSRLESeqSO() : ext_data_(), data_() {}
    PackedSSRLESeqSO(ExtData* ext_data, PkdStruct* data) :
        ext_data_(ext_data), data_(data)
    {}

    void setup()  {
        ext_data_ = nullptr;
        data_ = nullptr;
    }

    void setup(ExtData* ext_data, PkdStruct* data)  {
        ext_data_ = ext_data;
        data_ = data;
    }

    void setup(ExtData* ext_data)  {
        ext_data_ = ext_data;
    }

    void setup(PkdStruct* data)  {
        data_ = data;
    }

    SumIndexSO sum_index() {
        return SumIndexSO{&index_ext_data_, data_->sum_index()};
    }

    SumIndexSO sum_index() const {
        return SumIndexSO{const_cast<ExtData*>(&index_ext_data_), const_cast<SumIndexT*>(data_->sum_index())};
    }

    SizeIndexSO size_index() {
        return SizeIndexSO{ &index_ext_data_, data_->size_index()};
    }

    SizeIndexSO size_index() const {
        return SizeIndexSO{const_cast<ExtData*>(&index_ext_data_), const_cast<SizeIndexT*>(data_->size_index())};
    }


    operator bool() const  {
        return data_ != nullptr;
    }

    const ExtData* ext_data() const  {return ext_data_;}
    ExtData* ext_data()  {return ext_data_;}

    const PkdStruct* data() const  {return data_;}
    PkdStruct* data()  {return data_;}

    size_t size() const  {
        return (size_t)data_->size();
    }

    SymbolT access(SeqSizeT pos) const
    {
        RunSizeT offset{};
        Iterator ii = do_access(pos, offset);

        return ii.get().symbol(offset);
    }

    void clear() {
        return data_->clear();
    }

    auto iterator() const {
        return data_->iterator();
    }

    auto iterator(size_t unit_pos) const {
        return data_->iterator(unit_pos);
    }


    void check() const
    {
        if (data_->has_size_index()) {
            size_index().check();
        }

        if (data_->has_sum_index()) {
            sum_index().check();
        }

        if (data_->has_size_index() || data_->has_sum_index()) {
            ssrleseq::ReindexFn<MyType> reindex_fn;
            return reindex_fn.check(*this);
        }
    }

    void reindex(bool compactify = true) {
        return do_reindex(Optional<Span<SymbolsRunT>>{}, compactify);
    }

    auto sum(size_t symbol) const  {
        return (uint64_t)rank_eq(symbol);
    }


    SeqSizeT rank(SeqSizeT pos, SymbolT symbol, SeqOpType op_type) const {
        switch (op_type) {
            case SeqOpType::EQ : return rank_eq(pos, symbol);
            case SeqOpType::NEQ: return rank_neq(pos, symbol);
            case SeqOpType::LT : return rank_lt(pos, symbol);
            case SeqOpType::LE : return rank_le(pos, symbol);
            case SeqOpType::GT : return rank_gt(pos, symbol);
            case SeqOpType::GE : return rank_ge(pos, symbol);

            default: MEMORIA_MAKE_GENERIC_ERROR("Unsupported SeqOpType: {}", (size_t)op_type).do_throw();
        }
    }

    SeqSizeT rank(SeqSizeT start, SeqSizeT end, SymbolT symbol, SeqOpType op_type) const {
        switch (op_type) {
            case SeqOpType::EQ : return rank_eq(start, end, symbol);
            case SeqOpType::NEQ: return rank_neq(start, end, symbol);
            case SeqOpType::LT : return rank_lt(start, end, symbol);
            case SeqOpType::LE : return rank_le(start, end, symbol);
            case SeqOpType::GT : return rank_gt(start, end, symbol);
            case SeqOpType::GE : return rank_ge(start, end, symbol);

            default: MEMORIA_MAKE_GENERIC_ERROR("Unsupported SeqOpType: {}", (size_t)op_type).do_throw();
        }
    }

    SeqSizeT rank(SymbolT symbol, SeqOpType op_type) const {
        switch (op_type) {
            case SeqOpType::EQ : return rank_eq(symbol);
            case SeqOpType::NEQ: return rank_neq(symbol);
            case SeqOpType::LT : return rank_lt(symbol);
            case SeqOpType::LE : return rank_le(symbol);
            case SeqOpType::GT : return rank_gt(symbol);
            case SeqOpType::GE : return rank_ge(symbol);

            default: MEMORIA_MAKE_GENERIC_ERROR("Unsupported SeqOpType: {}", (size_t)op_type).do_throw();
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
        const auto* meta = data_->metadata();
        SeqSizeT size = meta->size();

        MEMORIA_V1_ASSERT_TRUE(pos <= size);

        size_t atom_pos;
        SeqSizeT size_sum;
        SeqSizeT rank_sum;

        if (data_->has_index()) {
            SizeIndexSO index = size_index();
            auto res = index.find_fw_gt(0, pos);
            atom_pos = res.local_pos() * AtomsPerBlock;
            size_sum = res.prefix();

            SumIndexSO sums  = this->sum_index();

            for (size_t c = 0; c < AlphabetSize; c++) {
                symbols[c] += (T)sums.sum(c, res.local_pos());
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

    SelectResult select_fw(SeqSizeT rank, SymbolT symbol, SeqOpType op_type) const {
        switch (op_type) {
            case SeqOpType::EQ : return select_fw_eq(rank, symbol);
            case SeqOpType::NEQ: return select_fw_neq(rank, symbol);
            case SeqOpType::LT : return select_fw_lt(rank, symbol);
            case SeqOpType::LE : return select_fw_le(rank, symbol);
            case SeqOpType::GT : return select_fw_gt(rank, symbol);
            case SeqOpType::GE : return select_fw_ge(rank, symbol);
            case SeqOpType::EQ_NLT : return select_fw_eq_nlt(rank, symbol);

            default: MEMORIA_MAKE_GENERIC_ERROR("Unsupported SeqOpType: {}", (size_t)op_type).do_throw();
        }
    }

    SelectResult select_fw(SeqSizeT idx, SeqSizeT rank, SymbolT symbol, SeqOpType op_type) const {
        switch (op_type) {
            case SeqOpType::EQ : return select_fw_eq(idx, rank, symbol);
            case SeqOpType::NEQ: return select_fw_neq(idx, rank, symbol);
            case SeqOpType::LT : return select_fw_lt(idx, rank, symbol);
            case SeqOpType::LE : return select_fw_le(idx, rank, symbol);
            case SeqOpType::GT : return select_fw_gt(idx, rank, symbol);
            case SeqOpType::GE : return select_fw_ge(idx, rank, symbol);
            case SeqOpType::EQ_NLT : return select_fw_eq_nlt(idx, rank, symbol);

            default: MEMORIA_MAKE_GENERIC_ERROR("Unsupported SeqOpType: {}", (size_t)op_type).do_throw();
        }
    }

    SelectResult select_bw(SeqSizeT rank, SymbolT symbol, SeqOpType op_type) const {
        switch (op_type) {
            case SeqOpType::EQ : return select_bw_eq(rank, symbol);
            case SeqOpType::NEQ: return select_bw_neq(rank, symbol);
            case SeqOpType::LT : return select_bw_lt(rank, symbol);
            case SeqOpType::LE : return select_bw_le(rank, symbol);
            case SeqOpType::GT : return select_bw_gt(rank, symbol);
            case SeqOpType::GE : return select_bw_ge(rank, symbol);

            default: MEMORIA_MAKE_GENERIC_ERROR("Unsupported SeqOpType: {}", (size_t)op_type).do_throw();
        }
    }

    SelectResult select_bw(SeqSizeT idx, SeqSizeT rank, SymbolT symbol, SeqOpType op_type) const {
        switch (op_type) {
            case SeqOpType::EQ : return select_bw_eq(idx, rank, symbol);
            case SeqOpType::NEQ: return select_bw_neq(idx, rank, symbol);
            case SeqOpType::LT : return select_bw_lt(idx, rank, symbol);
            case SeqOpType::LE : return select_bw_le(idx, rank, symbol);
            case SeqOpType::GT : return select_bw_gt(idx, rank, symbol);
            case SeqOpType::GE : return select_bw_ge(idx, rank, symbol);

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
        return select_fw_ge(rank + rank_base, symbol).dec_rank(rank_base);
    }

    SelectResult select_fw_gt(SeqSizeT idx, SeqSizeT rank, SymbolT symbol) const {
        SeqSizeT rank_base = rank_gt(idx, symbol);
        return select_fw_gt(rank + rank_base, symbol).dec_rank(rank_base);
    }

    SelectResult select_fw_le(SeqSizeT idx, SeqSizeT rank, SymbolT symbol) const {
        SeqSizeT rank_base = rank_le(idx, symbol);
        return select_fw_le(rank + rank_base, symbol).dec_rank(rank_base);
    }

    SelectResult select_fw_lt(SeqSizeT idx, SeqSizeT rank, SymbolT symbol) const {
        SeqSizeT rank_base = rank_lt(idx, symbol);
        return select_fw_lt(rank + rank_base, symbol).dec_rank(rank_base);
    }

    SelectResult select_fw_eq(SeqSizeT idx, SeqSizeT rank, SymbolT symbol) const {
        SeqSizeT rank_base = rank_eq(idx, symbol);
        return select_fw_eq(rank + rank_base, symbol).dec_rank(rank_base);
    }

    SelectResult select_fw_neq(SeqSizeT idx, SeqSizeT rank, SymbolT symbol) const {
        SeqSizeT rank_base = rank_neq(idx, symbol);
        return select_fw_neq(rank + rank_base, symbol).dec_rank(rank_base);
    }


    SelectResult select_fw_eq_nlt(SeqSizeT idx, SeqSizeT rank, SymbolT symbol) const
    {
        SelectResult res_eq = select_fw_eq(idx, rank, symbol);
        if (symbol > 0) {
            SelectResult res_lt = select_fw_lt(idx, rank, symbol);

            if (res_lt.idx < res_eq.idx) {
                return res_lt;
            }
        }

        return res_eq;
    }

    SelectResult select_fw_eq_nlt(SeqSizeT rank, SymbolT symbol) const
    {
        SelectResult res_eq = select_fw_eq(rank, symbol);
        SelectResult res_lt = select_fw_lt(rank, symbol);

        if (symbol > 0) {
            if (res_lt.idx < res_eq.idx) {
                return res_lt;
            }
        }

        return res_eq;
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


    void generateDataEvents(IBlockDataEventHandler* handler) const
    {
        handler->startGroup("PACKED_RLE_SEQUENCE");
        auto meta = data_->metadata();

        handler->value("SIZE", &meta->size());
        handler->value("CODE_UNITS", &meta->code_units());

        handler->startGroup("INDEXES");
        if (data_->has_size_index()) {
            size_index().generateDataEvents(handler);
        }
        if (data_->has_sum_index()) {
            sum_index().generateDataEvents(handler);
        }
        handler->endGroup();

        handler->startGroup("SYMBOL RUNS", size());

        auto iter = this->iterator();

        size_t offset{};
        iter.for_each([&](const SymbolsRunT& run){
            U8String str = format_u8("{} :: {}", offset, run);
            handler->value("RUN", BlockValueProviderT<U8String>(str));
            offset += run.full_run_length();
        });

        handler->endGroup();
        handler->endGroup();
    }



    struct InsertStepUS: PkdStructUpdateStepBase<MyType> {
        std::vector<SymbolsRunT> runs;
        size_t new_code_units;
        SeqSizeT size;
    };

    PkdUpdateStatus prepare_insert(SeqSizeT idx, UpdateState& update_state, Span<const SymbolsRunT> runs)
    {
        std::vector<SymbolsRunT> left;
        std::vector<SymbolsRunT> right;

        SeqSizeT size = this->size();

        if (idx <= size)
        {
            split_buffer(left, right, idx);

            append_all(left, runs);
            append_all(left, to_const_span(right));

            compactify_runs(left);

            InsertStepUS* step = update_state.template make_new<InsertStepUS>();

            step->runs = std::move(left);
            step->new_code_units = RunTraits::compute_size(step->runs, 0);
            step->size = count_symbols(runs);

            size_t required_block_size = PkdStruct::compute_block_size(step->new_code_units * sizeof(CodeUnitT));
            size_t existing_block_size = data_->block_size();

            return update_state.allocator_state()->inc_allocated(existing_block_size, required_block_size);
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Range check in SSRLE sequence insert: idx={}, size={}", idx, size).do_throw();
        }
    }

    size_t get_code_units_num(const UpdateState& update_state) const {
        const InsertStepUS* step = update_state.template step<InsertStepUS>();
        return step->new_code_units;
    }


    void commit_insert(SeqSizeT idx, UpdateState& update_state, Span<const SymbolsRunT> runs)
    {
        InsertStepUS* step = update_state.template step<InsertStepUS>();

        data_->resize_block(PkdStruct::SYMBOLS, step->new_code_units * sizeof(CodeUnitT));

        Span<CodeUnitT> atoms = data_->symbols_block();
        RunTraits::write_segments_to(step->runs, atoms, 0);

        Metadata* meta = data_->metadata();
        meta->set_code_units(step->new_code_units);
        meta->add_size(step->size);

        do_reindex(to_span(step->runs));
    }


    PkdUpdateStatus prepare_update(SeqSizeT idx, UpdateState& update_state, Span<const SymbolsRunT> runs)
    {
        std::vector<SymbolsRunT> left;
        std::vector<SymbolsRunT> right;

        SeqSizeT size = this->size();

        if (idx <= size)
        {
            SeqSizeT run_size = count_symbols(runs);

            auto syms = iterator().as_vector();
            remove_block(left, right, syms, idx, idx + run_size);

            append_all(left, runs);
            append_all(left, to_const_span(right));

            compactify_runs(left);

            InsertStepUS* step = update_state.template make_new<InsertStepUS>();

            step->runs = std::move(left);
            step->new_code_units = RunTraits::compute_size(step->runs, 0);

            size_t required_block_size = PkdStruct::compute_block_size(step->new_code_units * sizeof(CodeUnitT));
            size_t existing_block_size = data_->block_size();

            return update_state.allocator_state()->inc_allocated(existing_block_size, required_block_size);
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Range check in SSRLE sequence update: idx={}, size={}", idx, size).do_throw();
        }
    }


    void commit_update(SeqSizeT idx, UpdateState& update_state, Span<const SymbolsRunT> runs) {
        commit_insert(idx, update_state, runs);
    }




    struct RemoveStepUS: PkdStructUpdateStepBase<MyType> {
        std::vector<SymbolsRunT> runs;
        size_t new_code_units;
        SeqSizeT size;
    };



    PkdUpdateStatus prepare_remove(SeqSizeT start, SeqSizeT end, UpdateState& update_state) const
    {
        auto meta = data_->metadata();

        MEMORIA_ASSERT(start, <=, end);
        MEMORIA_ASSERT(end, <=, meta->size());

        if (end > start)
        {
            std::vector<SymbolsRunT> runs = iterator().as_vector();

            RemoveStepUS* step = update_state.template make_new<RemoveStepUS>();
            remove_block(step->runs, to_const_span(runs), start, end);

            compactify_runs(step->runs);

            step->new_code_units = RunTraits::compute_size(step->runs, 0);

            size_t required_block_size = PkdStruct::compute_block_size(step->new_code_units * sizeof(CodeUnitT));
            size_t existing_block_size = data_->block_size();

            step->size = end - start;

            return update_state.allocator_state()->inc_allocated(existing_block_size, required_block_size);
        }

        return PkdUpdateStatus::SUCCESS;
    }


    void commit_remove(SeqSizeT start, SeqSizeT end, UpdateState& update_state, bool compactify = false)
    {
        auto meta = data_->metadata();
        RemoveStepUS* step = update_state.template step<RemoveStepUS>();

        if (step)
        {
            data_->resize_block(PkdStruct::SYMBOLS, step->new_code_units * sizeof(CodeUnitT));

            Span<CodeUnitT> atoms = data_->symbols_block();
            RunTraits::write_segments_to(step->runs, atoms, 0);

            meta->set_code_units(step->new_code_units);
            meta->sub_size(step->size);

            return do_reindex(to_span(step->runs));
        }
    }

    void compactify()
    {
        std::vector<SymbolsRunT> syms = this->iterator().as_vector();

        compactify_runs(syms);

        size_t new_code_units = RunTraits::compute_size(syms, 0);

        auto new_block_size = PackedAllocatable::round_up_bytes(new_code_units * sizeof(CodeUnitT));
        data_->resize_block(PkdStruct::SYMBOLS, new_block_size);

        Span<CodeUnitT> atoms = data_->symbols();
        RunTraits::write_segments_to(syms, atoms, 0);

        return reindex();
    }

    void compactify_runs(Span<SymbolsRunT> runs) const
    {
        RunTraits::compactify_runs(runs);
    }

    void compactify_runs(std::vector<SymbolsRunT>& runs) const {
        RunTraits::compactify_runs(runs);
    }



    // ========================================= Node ================================== //
    void split_to(MyType& other, SeqSizeT idx)
    {
        Metadata* meta = data_->metadata();

        if (idx < meta->size())
        {
            Metadata* other_meta = other.data_->metadata();

            auto location = find_run(idx);

            auto result = RunTraits::split(location.run, location.local_symbol_idx);

            size_t adjustment = location.run.size_in_units();
            std::vector<SymbolsRunT> right_runs = this->iterator(location.unit_idx + adjustment).as_vector();

            size_t left_code_units = RunTraits::compute_size(result.left.span(), location.unit_idx);
            data_->resize_block(PkdStruct::SYMBOLS, left_code_units * sizeof(CodeUnitT));

            Span<CodeUnitT> left_syms = data_->symbols_block();
            RunTraits::write_segments_to(result.left.span(), left_syms, location.unit_idx);

            size_t right_atoms_size = RunTraits::compute_size(result.right.span(), right_runs);
            other.data_->resize_block(PkdStruct::SYMBOLS, right_atoms_size * sizeof(CodeUnitT));

            Span<CodeUnitT> right_syms = other.data_->symbols_block();
            size_t right_code_units = RunTraits::write_segments_to(result.right.span(), right_runs, right_syms);

            other_meta->set_size(meta->size() - idx);
            other_meta->set_code_units(right_code_units);

            meta->set_size(idx);
            meta->set_code_units(left_code_units);

            other.reindex();

            return reindex();
        }
        else if (idx > meta->size()) {
            MEMORIA_MAKE_GENERIC_ERROR("Split index is out of range: {} :: {}", idx, meta->size()).do_throw();
        }
    }

    struct MergeWithUS: PkdStructUpdateStepBase<MyType> {
        std::vector<SymbolsRunT> syms;
        size_t new_code_units;

        void apply(MyType& me) {}
    };


    PkdUpdateStatus prepare_merge_with(const MyType& other, UpdateState& update_state) const
    {
        MergeWithUS* step = update_state.template make_new<MergeWithUS>();

        step->syms = other.iterator().as_vector();
        iterator().read_to(step->syms);
        compactify_runs(step->syms);

        step->new_code_units = RunTraits::compute_size(step->syms, 0);

        size_t required_block_size = PkdStruct::compute_block_size(step->new_code_units * sizeof(CodeUnitT));
        size_t existing_block_size = other.data()->block_size();

        return update_state.allocator_state()->inc_allocated(existing_block_size, required_block_size);
    }

    void commit_merge_with(MyType& other, UpdateState& update_state) const
    {
        auto meta       = data_->metadata();
        auto other_meta = other.data_->metadata();

        MergeWithUS* step = update_state.template step<MergeWithUS>();

        auto new_block_size = step->new_code_units * sizeof(CodeUnitT);
        other.data_->resize_block(PkdStruct::SYMBOLS, new_block_size);

        Span<CodeUnitT> atoms = other.data_->symbols_block();
        RunTraits::write_segments_to(step->syms, atoms, 0);

        other_meta->add_size(meta->size());
        other_meta->set_code_units(step->new_code_units);

        return other.do_reindex(to_span(step->syms));
    }




    template <typename IOSubstream>
    PkdUpdateStatus prepare_insert_io_substream(
            SeqSizeT at,
            const IOSubstream& buffer,
            SeqSizeT start,
            SeqSizeT size,
            UpdateState& update_state
    )
    {
        std::vector<SymbolsRunT> syms = buffer.symbol_runs(start, size);
        return prepare_insert(at, update_state, syms);
    }


    template <typename IOSubstream>
    size_t commit_insert_io_substream(SeqSizeT at, const IOSubstream& buffer, SeqSizeT start, SeqSizeT size, UpdateState& update_state)
    {
        Span<const SymbolsRunT> syms;
        commit_insert(at, update_state, syms);

        return at + size;
    }


    auto find_run(SeqSizeT symbol_pos) const
    {
        return find_run(data_->metadata(), symbol_pos);
    }

    std::vector<SymbolsRunT> symbol_runs(SeqSizeT pos, SeqSizeT length) const
    {
        Location loc = find_run(pos);

        std::vector<SymbolsRunT> runs{};

        if (loc.run)
        {
            RunSizeT start = loc.local_symbol_idx;
            RunSizeT run_len = loc.run.full_run_length();
            RunSizeT remainder = run_len - start;

            if (SeqSizeT{remainder} >= length) {
                loc.run.extract_to(runs, loc.run, start, length);
            }
            else {
                loc.run.extract_to(runs, loc.run, start, remainder);

                Iterator ii = iterator(loc.unit_idx + loc.run.size_in_units());

                SeqSizeT sum{remainder};
                ii.do_while([&](const SymbolsRunT& run){
                    SeqSizeT size = run.full_run_length();
                    if (length < sum + size)
                    {
                        run.extract_to(runs, run, 0, length - sum);
                        return false;
                    }
                    else {
                        sum += size;
                        runs.push_back(run);
                        return true;
                    }
                });
            }
        }

        return runs;
    }

    MMA_MAKE_UPDATE_STATE_METHOD

private:

    struct RankEqFn {
        SumValueT sum_fn(const SumIndexSO& index, size_t idx, SymbolT symbol) const {
            return index.sum(symbol, idx);
        }

        SumValueT sum_all_fn(const SumIndexSO index, SymbolT symbol) const {
            return index.sum(symbol);
        }

        SumValueT rank_fn(const SymbolsRunT& run, SeqSizeT idx, SymbolT symbol) const {
            return run.rank_eq(idx, symbol);
        }

        SumValueT full_rank_fn(const SymbolsRunT& run, SymbolT symbol) const {
            return run.full_rank_eq(symbol);
        }
    };



    struct RankLtFn {
        SumValueT sum_fn(const SumIndexSO& index, size_t idx, size_t symbol) const
        {
            SumValueT sum{};
            for (size_t c = 0; c < symbol; c++) {
                sum += index.sum(c, idx);
            }
            return sum;
        }

        SumValueT sum_all_fn(const SumIndexSO& index, SymbolT symbol) const {
            return sum_fn(index, index.size(), symbol);
        }

        SumValueT rank_fn(const SymbolsRunT& run, SeqSizeT idx, SymbolT symbol) const {
            return run.rank_lt(idx, symbol);
        }

        SumValueT full_rank_fn(const SymbolsRunT& run, SymbolT symbol) const {
            return run.full_rank_lt(symbol);
        }
    };

    struct RankLeFn {
        SumValueT sum_fn(const SumIndexSO& index, size_t idx, SymbolT symbol) const
        {
            SumValueT sum{};
            for (size_t c = 0; c <= symbol; c++) {
                sum += index.sum(c, idx);
            }
            return sum;
        }

        SumValueT sum_all_fn(const SumIndexSO& index, SymbolT symbol) const {
            return sum_fn(index, index.size(), symbol);
        }

        SumValueT rank_fn(const SymbolsRunT& run, SeqSizeT idx, SymbolT symbol) const {
            return run.rank_le(idx, symbol);
        }

        SumValueT full_rank_fn(const SymbolsRunT& run, SymbolT symbol) const {
            return run.full_rank_le(symbol);
        }
    };

    struct RankGtFn {
        SumValueT sum_fn(const SumIndexSO& index, size_t idx, size_t symbol) const
        {
            SumValueT sum{};
            for (size_t c = symbol + 1; c < AlphabetSize; c++) {
                sum += index.sum(c, idx);
            }
            return sum;
        }

        SumValueT sum_all_fn(const SumIndexSO& index, SymbolT symbol) const {
            return sum_fn(index, index.size(), symbol);
        }

        SumValueT rank_fn(const SymbolsRunT& run, SeqSizeT idx, SymbolT symbol) const {
            return run.rank_gt(idx, symbol);
        }

        SumValueT full_rank_fn(const SymbolsRunT& run, SymbolT symbol) const {
            return run.full_rank_gt(symbol);
        }
    };

    struct RankGeFn {
        SumValueT sum_fn(const SumIndexSO& index, size_t idx, size_t symbol) const
        {
            SumValueT sum{};
            for (size_t c = symbol; c < AlphabetSize; c++) {
                sum += index.sum(c, idx);
            }
            return sum;
        }

        SumValueT sum_all_fn(const SumIndexSO& index, SymbolT symbol) const {
            return sum_fn(index, index.size(), symbol);
        }

        SumValueT rank_fn(const SymbolsRunT& run, SeqSizeT idx, SymbolT symbol) const {
            return run.rank_ge(idx, symbol);
        }

        SumValueT full_rank_fn(const SymbolsRunT& run, SymbolT symbol) const {
            return run.full_rank_ge(symbol);
        }
    };

    struct RankNeqFn {
        SumValueT sum_fn(const SumIndexSO& index, size_t idx, size_t symbol) const
        {
            SumValueT sum{};
            for (size_t c = 0; c < AlphabetSize; c++) {
                sum += c != symbol ? index.sum(c, idx) : SeqSizeT{0};
            }
            return sum;
        }

        SumValueT sum_all_fn(const SumIndexSO& index, SymbolT symbol) const {
            return sum_fn(index, index.size(), symbol);
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

        const auto* meta = data_->metadata();
        SeqSizeT size = meta->size();

        MEMORIA_ASSERT(pos, <=, size);
        MEMORIA_ASSERT(symbol, <, AlphabetSize);

        size_t unit_pos;
        SeqSizeT size_sum;
        SeqSizeT rank_sum;

        if (data_->has_index()) {
            SizeIndexSO index = size_index();

            if (pos < size)
            {
                auto res = index.find_fw_gt(0, pos);
                unit_pos = res.local_pos() * AtomsPerBlock;
                size_sum = res.prefix();

                SumIndexSO sums  = sum_index();
                rank_sum = fn.sum_fn(sums, res.local_pos(), symbol);
            }
            else {
                SumIndexSO sums  = sum_index();
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


    using FindResult = typename SumIndexSO::FindResult;

    struct SelectFwEqFn {
        FindResult index_fn(const SumIndexSO& index, SeqSizeT rank, SymbolT symbol) const {
            return index.find_fw_ge(symbol, rank);
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
        FindResult index_fn(const SumIndexSO& index, SeqSizeT rank, SymbolT symbol) const
        {
            SeqSizeT sum{};
            size_t size = index.size();
            size_t c;

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
        SeqSizeT sum(const SumIndexSO& index, size_t idx, size_t symbol) const {
            SeqSizeT sum{};
            for (size_t c = 0; c < symbol; c++) {
                sum += index.value(c, idx);
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
        SeqSizeT sum(const SumIndexSO& index, size_t idx, size_t symbol) const {
            SeqSizeT sum{};
            for (size_t c = 0; c <= symbol; c++) {
                sum += index.value(c, idx);
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
        SeqSizeT sum(const SumIndexSO& index, size_t idx, size_t symbol) const {
            SeqSizeT sum{};
            for (size_t c = symbol + 1; c < AlphabetSize; c++) {
                sum += index.value(c, idx);
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
        SeqSizeT sum(const SumIndexSO& index, size_t idx, size_t symbol) const {
            SeqSizeT sum{};
            for (size_t c = symbol; c < AlphabetSize; c++) {
                sum += index.value(c, idx);
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
        SeqSizeT sum(const SumIndexSO& index, size_t idx, size_t symbol) const {
            SeqSizeT sum{};
            for (size_t c = 0; c < AlphabetSize; c++) {
                sum += c != symbol ? index.value(c, idx) : SeqSizeT{0};
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
        MEMORIA_V1_ASSERT_TRUE(symbol < AlphabetSize);

        size_t unit_pos;
        SeqSizeT rank_sum;
        SeqSizeT size_sum;

        if (data_->has_index()) {
            SumIndexSO sums = sum_index();

            auto res = fn.index_fn(sums, rank, symbol);
            if (res.idx() < sums.size())
            {
                unit_pos = res.idx() * AtomsPerBlock;
                rank_sum = res.prefix();

                SizeIndexSO sizes  = size_index();
                size_sum = sizes.sum(0, res.idx());
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

        auto ii = iterator(unit_pos);

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


    SeqSizeT count_symbols(Span<const SymbolsRunT> runs)
    {
        SeqSizeT sum{};
        for (const auto& run: runs) {
            sum += run.full_run_length();
        }
        return sum;
    }


    template <typename T1, typename T2>
    void append_all(std::vector<T1>& vv, Span<T2> span) const {
        vv.insert(vv.end(), span.begin(), span.end());
    }

    void split_buffer(
            std::vector<SymbolsRunT>& left,
            std::vector<SymbolsRunT>& right,
            SeqSizeT pos) const
    {
        std::vector<SymbolsRunT> runs = iterator().as_vector();
        return split_buffer(to_span(runs), left, right, pos);
    }

    void split_buffer(
            Span<const SymbolsRunT> runs,
            std::vector<SymbolsRunT>& left,
            std::vector<SymbolsRunT>& right,
            SeqSizeT pos) const
    {
        auto res = find_in_syms(runs, pos);

        if (res.run_idx < runs.size())
        {
            auto s_res = runs[res.run_idx].split(res.local_offset);

            append_all(left, runs.subspan(0, res.run_idx));
            append_all(left, s_res.left.span());

            append_all(right, s_res.right.span());

            if (res.run_idx + 1 < runs.size()) {
                append_all(right, runs.subspan(res.run_idx + 1));
            }
        }
        else {
            for (size_t c = 0; c < runs.size(); c++) {
                left.push_back(runs[c]);
            }
        }
    }


    MMA_PKD_OOM_SAFE
    void do_reindex(
            Optional<Span<SymbolsRunT>> data,
            bool compactify = false
    )
    {
        ssrleseq::ReindexFn<MyType> reindex_fn;
        return reindex_fn.reindex(*this, data, compactify);
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

    Iterator do_access(SeqSizeT pos, RunSizeT& offset) const
    {
        const Metadata* meta = data_->metadata();
        SeqSizeT size = meta->size();

        MEMORIA_ASSERT(pos, <, size);

        size_t unit_pos;
        SeqSizeT sum;

        if (data_->has_size_index()) {
            SizeIndexSO index = size_index();
            auto res = index.find_fw_gt(0, pos);

            if (res.local_pos() < index.size())
            {
                unit_pos = res.local_pos() * AtomsPerBlock;
                sum = res.prefix();
            }
            else {
                MEMORIA_MAKE_GENERIC_ERROR(
                    "SSRLESequence index: position {} is out of index rage: {}", pos, index.sum(0)
                ).do_throw();
            }
        }
        else {
            unit_pos = 0;
            sum = SeqSizeT{};
        }

        auto ii = iterator(unit_pos);

        SymbolsRunT tgt;
        ii.do_while([&](const SymbolsRunT& run) {
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
            offset = pos - sum;
            return ii;
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Invalid SSRLE sequence").do_throw();
        }
    }

    struct Location {
        SymbolsRunT run;
        size_t unit_idx;
        RunSizeT local_symbol_idx;
    };


    auto find_run(const Metadata* meta, SeqSizeT symbol_pos) const
    {
        if (symbol_pos < meta->size())
        {
            if (!data_->has_size_index())
            {
                return locate_run(meta, 0, symbol_pos, 0);
            }
            else
            {
                auto find_result = size_index().find_fw_gt(0, symbol_pos);

                uint64_t local_pos  = symbol_pos - find_result.prefix();
                size_t block_offset = find_result.local_pos() * AtomsPerBlock;

                return locate_run(meta, block_offset, local_pos, find_result.prefix());
            }
        }
        else {
            //return Location(meta->code_units(), 0, 0, meta->code_units(), symbol_pos, RLESymbolsRun(), true);
            return Location{};
        }
    }

    Location locate_run(const Metadata* meta, size_t start, SeqSizeT local_idx, size_t size_base) const
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

    Location locate_last_run(const Metadata* meta) const
    {
        SymbolsRunT last;
        size_t last_unit_idx{};

        size_t remainder = meta->code_units() % SegmentSizeInAtoms;

        if (remainder == 0 && meta->code_units() > 0) {
            remainder = SegmentSizeInAtoms;
        }

        size_t segment_start = meta->code_units() - remainder;
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


    void remove_block(std::vector<SymbolsRunT>& dst_left, std::vector<SymbolsRunT>& dst_right, Span<const SymbolsRunT> runs, SeqSizeT start, SeqSizeT end) const
    {
        auto meta = data_->metadata();

        if (end < meta->size()) {
            if (start > SeqSizeT{0}) {
                std::vector<SymbolsRunT> right1;
                split_buffer(runs, dst_left, right1, start);

                std::vector<SymbolsRunT> left2;
                split_buffer(right1, left2, dst_right, end - start);
            }
            else {
                std::vector<SymbolsRunT> left;
                split_buffer(runs, left, dst_right, end);
            }
        }
        else {
            if (start > SeqSizeT{0}) {
                std::vector<SymbolsRunT> right;
                split_buffer(runs, dst_left, right, start);
            }
        }
    }

    void remove_block(std::vector<SymbolsRunT>& dst, Span<const SymbolsRunT> runs, SeqSizeT start, SeqSizeT end) const
    {
        std::vector<SymbolsRunT> right;
        remove_block(dst, right, runs, start, end);

        if (right.size()) {
            append_all(dst, to_span(right));
        }
    }

};



}
