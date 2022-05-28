
// Copyright 2022 Victor Smirnov
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

#include <memoria/tests/tests.hpp>
#include <memoria/tests/assertions.hpp>

#include <memoria/core/packed/sseq/packed_ssrle_seq.hpp>
#include <memoria/core/packed/tools/packed_struct_ptrs.hpp>

#include <memoria/reactor/reactor.hpp>

#include <memoria/api/sequence/sequence_api.hpp>

#include "../prototype/bt/bt_test_base.hpp"
#include "../packed/sequence/ssrle/ssrleseq_test_base.hpp"

#include <memory>
#include <array>

namespace memoria {
namespace tests {


template <
    size_t AlphabetSize_,
    bool Use64BitSize,
    typename ProfileT = CoreApiProfile,
    typename StoreT   = IMemoryStorePtr<ProfileT>
>
class SequenceTestBase:
        public BTTestBase<Sequence<AlphabetSize_>, ProfileT, StoreT>,
        public SymbolsRunTools<AlphabetSize_, Use64BitSize>
{
    using Base = BTTestBase<Sequence<AlphabetSize_>, ProfileT, StoreT>;
    using ToolsBase = SymbolsRunTools<AlphabetSize_, Use64BitSize>;

    using MyType = PackedSSRLESequenceTestBase<AlphabetSize_, Use64BitSize>;

protected:

    static constexpr size_t SIZE_INDEX_BLOCK = 128;
    static constexpr size_t Bps = BitsPerSymbolConstexpr(AlphabetSize_);
    static constexpr size_t AlphabetSize = AlphabetSize_;

    using Seq    = PkdSSRLESeqT<AlphabetSize, 256, Use64BitSize>;
    using SeqSO  = typename Seq::SparseObject;

    using SeqPtr = std::shared_ptr<PkdStructHolder<Seq>>;

    using CtrSizeT = ApiProfileCtrID<ProfileT>;

    size_t size_{32768};

    using SymbolsRunT   = SSRLERun<Bps>;
    using RunTraits     = SSRLERunTraits<Bps>;
    using CodeUnitT     = typename RunTraits::CodeUnitT;
    using SeqSizeT      = typename Seq::SeqSizeT;
    using RunSizeT      = typename Seq::RunSizeT;
    using SymbolT       = typename Seq::SymbolT;


    using Base::snapshot_;

public:
    using typename Base::CtrID;


    MMA_STATE_FILEDS(size_);

    using ToolsBase::push_back;
    using Base::out;

    SequenceTestBase(): ToolsBase(this) {}

    SeqSO get_so(SeqPtr ptr) const {
        return ptr->get_so();
    }



    SeqPtr make_empty_sequence(size_t syms_block_size = 1024*1024) const
    {
        size_t block_size = Seq::compute_block_size(syms_block_size * 2);
        return PkdStructHolder<Seq>::make_empty(block_size);
    }



    SeqPtr make_sequence(Span<const SymbolsRunT> span, size_t capacity_multiplier = 1) const
    {
        size_t num_atoms = RunTraits::compute_size(span);

        SeqPtr ptr = make_empty_sequence(num_atoms * capacity_multiplier);
        SeqSO seq = get_so(ptr);

        auto update_state = seq.make_update_state();
        assert_success(seq.prepare_insert(0, update_state.first, span));
        seq.commit_insert(0, update_state.first, span);

        seq.check();

        return ptr;
    }

    template <typename T>
    void assertIndexCorrect(const char* src, const T& seq)
    {
        try {
            seq->check();
        }
        catch (Exception& e) {
            out()<<"Sequence structure check failed"<<std::endl;
            seq->dump(out());
            throw e;
        }
    }

    template <typename T>
    void assertEmpty(const T& seq)
    {
        assert_equals(0, seq->size());
        assert_equals(false, seq->has_index());
    }

    auto create_sequence_ctr(Span<const SymbolsRunT> runs, size_t times = 1)
    {
        CtrID ctr_id = CtrID::make_random();
        auto ctr = create<Sequence<AlphabetSize>>(snapshot_, Sequence<AlphabetSize>{}, ctr_id);

        for (size_t s = 0; s < times; s++) {
            ctr->insert(0, runs);
        }

        return ctr;
    }
};



}}
