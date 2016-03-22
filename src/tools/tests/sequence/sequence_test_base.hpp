
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/memoria.hpp>
#include <memoria/tools/profile_tests.hpp>

#include <memoria/containers/seq_dense/seqd_factory.hpp>

#include <memoria/core/packed/wrappers/symbol_sequence.hpp>
#include <memoria/core/packed/tools/packed_struct_ptrs.hpp>
#include <memoria/core/tools/isymbols.hpp>

#include "../prototype/btss/btss_test_base.hpp"

#include <vector>

namespace memoria {

using namespace std;


template <Int BitsPerSymbol, bool Dense = true>
class SequenceTestBase: public BTTestBase<Sequence<BitsPerSymbol, Dense>, PersistentInMemAllocator<>, DefaultProfile<>> {

    using MyType = SequenceTestBase<BitsPerSymbol, Dense>;

    using Base   = BTTestBase<Sequence<BitsPerSymbol, Dense>, PersistentInMemAllocator<>, DefaultProfile<>>;

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

    static const Int Symbols                                                    = 1<<BitsPerSymbol;

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

    UUID ctr_name_;

public:
    SequenceTestBase(StringRef name): Base(name)
    {
        Ctr::initMetadata();

        size_ = 10000000;

        MEMORIA_ADD_TEST_PARAM(ctr_name_)->state();
    }

    PackedSeq1Ptr createEmptyPackedSeq(Int size)
    {
        Int block_size;

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

    PackedSeq1Ptr fillRandomSeq(Ctr& ctr, Int size)
    {
        PackedSeq1Ptr seq = createEmptyPackedSeq(size);

        using SymbolsBuffer = SmallSymbolBuffer<BitsPerSymbol>;

        BigInt t0 = getTimeInMillis();

        seq->fill_with_buf(0, size, [this](Int len) {

            SymbolsBuffer buf;
            Int limit = len > buf.capacity() ? buf.capacity() : len;

            buf.resize(limit);

            auto symbols = buf.symbols();

            for (Int c = 0; c < SymbolsBuffer::BufSize; c++)
            {
                symbols[c] = getRandom();
            }

            return buf;
        });

        BigInt t1 = getTimeInMillis();

        auto iter = ctr.begin();

        using Provider = seq_dense::SymbolSequenceInputProvider<Ctr>;
        Provider provider(ctr, seq->symbols(), 0, 4000000);

        ctr.insert(*iter.get(), provider);

        BigInt t2 = getTimeInMillis();

        this->out() << "Sequence creation time: " << FormatTime(t1 - t0) << " " << FormatTime(t2 - t1) << std::endl;

        return seq;
    }
};



}
