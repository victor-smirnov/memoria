
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


#include <memoria/containers/seq_dense/seqd_factory.hpp>

#include <memoria/core/packed/wrappers/symbol_sequence.hpp>
#include <memoria/core/packed/tools/packed_struct_ptrs.hpp>
#include <memoria/core/tools/isymbols.hpp>

#include "../prototype/btss/btss_test_base.hpp"

#include <vector>

namespace memoria {
namespace tests {

template <int32_t BitsPerSymbol, bool Dense = true>
class SequenceTestBase: public BTTestBase<Sequence<BitsPerSymbol, Dense>, InMemAllocator<>, DefaultProfile<>> {

    using MyType = SequenceTestBase<BitsPerSymbol, Dense>;

    using Base   = BTTestBase<Sequence<BitsPerSymbol, Dense>, InMemAllocator<>, DefaultProfile<>>;

protected:
    using CtrName = Sequence<BitsPerSymbol, Dense>;

    typedef typename DCtrTF<Sequence<BitsPerSymbol, Dense> >::Type              Ctr;
    typedef typename Ctr::Iterator                                              Iterator;
    typedef typename Ctr::BranchNodeEntry                                       BranchNodeEntry;
    typedef typename Ctr::ID                                                    ID;

    typedef PackedFSESequence<BitsPerSymbol>                                    PackedSeq;

    typedef SymbolsBuffer<BitsPerSymbol>                                        MemBuffer;


    using PSeqTypes  = typename PkdFSSeqTF<BitsPerSymbol>::Type;
    using PackedSeq1 = PkdFSSeq<PSeqTypes>;
    using PackedSeq1Ptr = PkdStructSPtr<PackedSeq1>;


    using Seq = PkdFSSeq<PSeqTypes>;

    static const int32_t Symbols                                                    = 1<<BitsPerSymbol;

    using Base::commit;
    using Base::drop;
    using Base::branch;
    using Base::allocator;
    using Base::snapshot;
    using Base::check;
    using Base::out;
    using Base::size_;
    using Base::storeAllocator;
    using Base::isReplayMode;
    using Base::getResourcePath;
    using Base::getRandom;
    using Base::getBIRandom;

    UUID ctr_name_;

public:
    SequenceTestBase()
    {
        Ctr::initMetadata();

        size_ = 10000000;

        MEMORIA_ADD_TEST_PARAM(ctr_name_)->state();
    }

    PackedSeq1Ptr createEmptyPackedSeq(int32_t size)
    {
        int32_t block_size;

        if (BitsPerSymbol == 1) {
            block_size = PackedSeq1::estimate_block_size(size, 1, 1);
        }
        else if (BitsPerSymbol == 4) {
            block_size = PackedSeq1::estimate_block_size(size, 1, 1);
        }
        else {
            block_size = PackedSeq1::estimate_block_size(size, 3, 2);
        }

        return MakeSharedPackedStructByBlock<PackedSeq1>(block_size);
    }

    PackedSeq1Ptr fillRandomSeq(Ctr& ctr, int32_t size)
    {
        PackedSeq1Ptr seq = createEmptyPackedSeq(size);

        using SymbolsBuffer = SmallSymbolBuffer<BitsPerSymbol>;

        int64_t t0 = getTimeInMillis();

        seq->fill_with_buf(0, size, [this](int32_t len) {

            SymbolsBuffer buf;
            int32_t limit = len > buf.capacity() ? buf.capacity() : len;

            buf.resize(limit);

            auto symbols = buf.symbols();

            for (int32_t c = 0; c < SymbolsBuffer::BufSize; c++)
            {
                symbols[c] = getBIRandom();
            }

            return buf;
        });

        int64_t t1 = getTimeInMillis();

        auto iter = ctr.begin();

        using Provider = seq_dense::SymbolSequenceInputProvider<Ctr>;
        Provider provider(ctr, seq->symbols(), 0, size);

        iter->bulk_insert(provider);

        int64_t t2 = getTimeInMillis();

        this->out() << "Sequence creation time: " << FormatTime(t1 - t0) << " " << FormatTime(t2 - t1) << std::endl;

        check(MA_SRC);

        return seq;
    }

    auto rank(const PackedSeq1* seq, int32_t start, int32_t end, int32_t symbol)
    {
        int64_t rank = 0;

        for (int32_t c = start; c < end; c++)
        {
            rank += seq->symbol(c) == symbol;
        }

        return rank;
    }
};



}}
