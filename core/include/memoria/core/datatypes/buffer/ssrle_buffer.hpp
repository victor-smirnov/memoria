
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

#include <memoria/core/packed/sseq/packed_ssrle_seq.hpp>
#include <memoria/core/exceptions/exceptions.hpp>
#include <memoria/core/memory/malloc.hpp>
#include <memoria/core/packed/tools/packed_struct_ptrs.hpp>

#include <memoria/core/hermes/traits.hpp>

#include <functional>

namespace memoria {

template <size_t AlphabetSize>
class IOSSRLEBuffer {

    using SeqSizeT = uint64_t;
    using SymbolT  = size_t;

    static constexpr SymbolT BITS_PER_SYMBOL = BitsPerSymbolConstexpr(AlphabetSize);

    using RunTraits = SSRLERunTraits<BITS_PER_SYMBOL>;
    using RunT      = SSRLERun<BITS_PER_SYMBOL>;
    using CodeUnitT = typename RunTraits::CodeUnitT;
    using RunSizeT  = typename RunTraits::RunSizeT;

    static_assert (AlphabetSize >= 2 && AlphabetSize <= 256, "");
    static constexpr size_t Bps = BitsPerSymbolConstexpr(AlphabetSize);

    using SeqT = PkdSSRLESeqT<AlphabetSize, 256, true>;
    using SeqSO = typename SeqT::SparseObject;
    using HolderT = PkdStructHolder<SeqT>;

    std::unique_ptr<HolderT> holder_;
    SeqSO sequence_so_;

    ArenaBuffer<RunT> runs_buf_;


public:
    IOSSRLEBuffer()
    {
        size_t size = SeqT::empty_size();
        holder_ = HolderT::make_empty_unique(size);
        sequence_so_ = holder_->get_so();
    }

    IOSSRLEBuffer(IOSSRLEBuffer&&) = delete;
    IOSSRLEBuffer(const IOSSRLEBuffer&) = delete;

    virtual ~IOSSRLEBuffer() noexcept = default;

    virtual bool is_indexed() const {
        return true;
    }

    virtual SymbolT alphabet_size() const {
        return AlphabetSize;
    }

    virtual bool is_const() const {
        return false;
    }

    virtual SymbolT symbol(SeqSizeT idx) const {
        assert_indexed();
        return sequence_so_.access(idx);
    }

    virtual SeqSizeT size() const {
        assert_indexed();
        return sequence_so_.size();
    }


    virtual void rank_to(SeqSizeT idx, Span<SeqSizeT> values) const
    {
        assert_indexed();

        for (SeqSizeT& vv: values) {
            vv = SeqSizeT{};
        }

        sequence_so_.ranks(idx, values);
    }

    virtual void check()
    {
        assert_indexed();
        sequence_so_.check();
    }

    virtual void reindex()
    {
        size_t buf_size = runs_buf_.size();
        if (buf_size)
        {
            auto update_state = sequence_so_.make_update_state();
            auto size = sequence_so_.size();
            PkdUpdateStatus status = sequence_so_.prepare_insert(size, update_state.first, runs_buf_.span());
            if (!is_success(status))
            {
                size_t code_units = sequence_so_.get_code_units_num(update_state.first);
                create_empty_sequence(code_units);
            }

            sequence_so_.commit_insert(size, update_state.first, runs_buf_.span());
            runs_buf_.clear();
        }
    }

    virtual void reset() {
        sequence_so_.data()->reset();
        runs_buf_.clear();
    }

    virtual void clear() {
        reset();
    }

    virtual void reset(hermes::HermesCtr&) {
        reset();
    }

    virtual void clear(hermes::HermesCtr&) {
        reset();
    }

    virtual void dump(std::ostream& out) const
    {
        assert_indexed();
        DumpStruct(sequence_so_, out);
    }

    virtual const std::type_info& sequence_type() const {
        return typeid(SeqT);
    }

    void append(Span<const RunT> runs)
    {
        if (runs_buf_.size()) {
            if (runs.size() == 1) {
                auto res = RunTraits::insert(runs_buf_.head(), runs[0], runs_buf_.head().full_run_length());
                auto span = res.span();
                runs_buf_.head() = span[0];
                for (size_t c = 1; c < span.size(); c++) {
                    runs_buf_.push_back(span[c]);
                }
            }
            else {
                runs_buf_.push_back(runs);
            }
        }
        else {
            runs_buf_.push_back(runs);
        }
    }

    virtual void append_run(SymbolT symbol, size_t size)
    {
        RunT run{1, symbol, size};
        append(Span<const RunT>{&run, 1});
    }

    Span<const CodeUnitT> code_units() const {
        assert_indexed();
        return sequence_so_.data()->symbols();
    }

    std::vector<RunT> symbol_runs(SeqSizeT start, SeqSizeT size) const {
        assert_indexed();
        return sequence_so_.symbol_runs(start, size);
    }


    void configure(const hermes::HermesCtr& ctr, size_t stream, size_t substream) {}

    void reset_state() {
        reset();
    }

private:

    void create_empty_sequence(size_t code_units)
    {
        size_t block_size = SeqT::compute_block_size(code_units * 2);
        holder_ = HolderT::make_empty_unique(block_size);
        sequence_so_ = holder_->get_so();
    }

    void assert_indexed() const {
        if (MMA_UNLIKELY(runs_buf_.size())) {
            MEMORIA_MAKE_GENERIC_ERROR("SSRLE Buffer has temporary runs and needs reindexing");
        }
    }
};



}
