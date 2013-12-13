
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_SEQUENCE_SEQUENCE_TEST_BASE_HPP_
#define MEMORIA_TESTS_SEQUENCE_SEQUENCE_TEST_BASE_HPP_

#include <memoria/memoria.hpp>
#include <memoria/tools/profile_tests.hpp>

#include <memoria/core/packed/wrappers/symbol_sequence.hpp>


#include <vector>

namespace memoria {

using namespace memoria::vapi;
using namespace std;


template <Int BitsPerSymbol, bool Dense = true>
class SequenceTestBase: public SPTestTask {
    typedef SequenceTestBase<BitsPerSymbol, Dense>                              MyType;
    typedef SPTestTask                                                          Base;

protected:

    typedef typename SCtrTF<Sequence<BitsPerSymbol, Dense> >::Type              Ctr;
    typedef typename Ctr::Iterator                                              Iterator;
    typedef typename Ctr::Accumulator                                           Accumulator;
    typedef typename Ctr::ID                                                    ID;

    typedef PackedFSESequence<BitsPerSymbol>                                    PackedSeq;

    typedef SymbolsBuffer<BitsPerSymbol>                                        MemBuffer;

    static const Int Symbols                                                    = 1<<BitsPerSymbol;

    String dump_name_;

public:
    SequenceTestBase(StringRef name): Base(name)
    {
        this->size_ = 100000;

        MEMORIA_ADD_TEST_PARAM(dump_name_)->state();
    }

    PackedSeq fillRandom(Ctr& ctr, Int size)
    {
        PackedSeq seq(size, (BitsPerSymbol == 8) ? 10 : 1, 1);

        seq.insert(0, size, [](){
            return getRandom(Symbols);
        });

        auto iter = ctr.Begin();

        BigInt t0 = getTimeInMillis();

        for (Int c = 0; c <size; c++)
        {
            iter.insert(seq[c]);
        }

        BigInt t1 = getTimeInMillis();

        this->out()<<"Sequence creation time: "<<FormatTime(t1 - t0)<<std::endl;

        return seq;
    }

    virtual MemBuffer createBuffer(Int size, Int symbol)
    {
        MemBuffer data(size);
        for (SizeT c = 0; c < size; c++)
        {
            data.put(symbol);
        }

        data.reset();

        return data;
    }

    virtual MemBuffer createRandomBuffer(Int size)
    {
        MemBuffer data(size);
        for (SizeT c = 0; c < size; c++)
        {
            data.put(getRandom(1 << BitsPerSymbol));
        }

        data.reset();

        return data;
    }

    virtual void compareBuffers(const MemBuffer& src, const MemBuffer& tgt, const char* source)
    {
        AssertEQ(source, src.size(), tgt.size(), SBuf()<<"buffer sizes are not equal");

        for (size_t c = 0; c < src.size(); c++)
        {
            typename MemBuffer::value_type v1 = src[c];
            typename MemBuffer::value_type v2 = tgt[c];

            AssertEQ(source, v1, v2, [=](){return SBuf()<<"c="<<c;});
        }
    }

};



}


#endif
