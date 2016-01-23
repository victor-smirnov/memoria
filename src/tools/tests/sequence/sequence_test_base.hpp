
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_SEQUENCE_SEQUENCE_TEST_BASE_HPP_
#define MEMORIA_TESTS_SEQUENCE_SEQUENCE_TEST_BASE_HPP_

#include <memoria/memoria.hpp>
#include <memoria/tools/profile_tests.hpp>

#include <memoria/core/packed/wrappers/symbol_sequence.hpp>
#include <memoria/core/packed/tools/packed_struct_ptrs.hpp>

#include <vector>

namespace memoria {

using namespace memoria::vapi;
using namespace std;


template <Int BitsPerSymbol, bool Dense = true>
class SequenceTestBase: public SPTestTask {
    typedef SequenceTestBase<BitsPerSymbol, Dense>                              MyType;
    typedef SPTestTask                                                          Base;

protected:

    typedef typename DCtrTF<Sequence<BitsPerSymbol, Dense> >::Type              Ctr;
    typedef typename Ctr::Iterator                                              Iterator;
    typedef typename Ctr::BranchNodeEntry                                           BranchNodeEntry;
    typedef typename Ctr::ID                                                    ID;

    typedef PackedFSESequence<BitsPerSymbol>                                    PackedSeq;

    typedef SymbolsBuffer<BitsPerSymbol>                                        MemBuffer;


    using PSeqTypes  = typename PkdFSSeqTF<BitsPerSymbol>::Type;
    using PackedSeq1 = PkdFSSeq<PSeqTypes>;
    using PackedSeq1Ptr = PkdStructSPtr<PackedSeq1>;


    using Seq = PkdFSSeq<PSeqTypes>;

    static const Int Symbols                                                    = 1<<BitsPerSymbol;

    String dump_name_;

public:
    SequenceTestBase(StringRef name): Base(name)
    {
        Ctr::initMetadata();

        this->size_ = 10000000;

        MEMORIA_ADD_TEST_PARAM(dump_name_)->state();
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

    PackedSeq1Ptr fillRandom(Ctr& ctr, Int size)
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
				symbols[c] = this->getRandom();
			}

			return buf;
        });

        BigInt t1 = getTimeInMillis();

        auto iter = ctr.Begin();

		using Provider = seq_dense::SymbolSequenceInputProvider<Ctr>;
		Provider provider(ctr, seq->symbols(), 0, 4000000);

		ctr.insert(iter, provider);

        BigInt t2 = getTimeInMillis();

        this->out()<<"Sequence creation time: "<<FormatTime(t1 - t0)<<" "<<FormatTime(t2 - t1)<<std::endl;

        return seq;
    }
};



}


#endif
