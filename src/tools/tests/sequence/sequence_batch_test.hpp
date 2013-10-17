
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_SEQUENCE_BATCH_TEST_HPP_
#define MEMORIA_TESTS_SEQUENCE_BATCH_TEST_HPP_

#include <memoria/memoria.hpp>
#include <memoria/tools/tests.hpp>

#include "../shared/sequence_create_test_base.hpp"

#include <vector>

namespace memoria {

using namespace memoria::vapi;
using namespace std;

template <Int BitsPerSymbol, bool Dense = true>
class SequenceBatchTest: public SequenceCreateTestBase<
    Sequence<BitsPerSymbol, Dense>,
    SymbolsBuffer<BitsPerSymbol>
>
{
    typedef SequenceBatchTest<BitsPerSymbol, Dense>                            	MyType;
    typedef MyType                                                              ParamType;

    typedef SequenceCreateTestBase<
    			Sequence<BitsPerSymbol, Dense>,
    			SymbolsBuffer<BitsPerSymbol>
    >                                                                           Base;

    typedef typename Base::Ctr                                                  Ctr;
    typedef typename Base::Iterator                                             Iterator;
    typedef typename Ctr::Accumulator                                           Accumulator;
    typedef typename Base::ID                                                   ID;

    typedef SymbolsBuffer<BitsPerSymbol>                                       	MemBuffer;

public:
    SequenceBatchTest(StringRef name):
        Base(name)
    {
        Base::max_block_size_ = 1024*40;
        Base::size_           = 1024*1024*2;
    }

    virtual MemBuffer createBuffer(Int size)
    {
        MemBuffer data(size);
        for (SizeT c = 0; c < size; c++)
        {
            data.put(c);
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

    virtual Iterator seek(Ctr& array, BigInt pos)
    {
        return array.seek(pos);
    }

    virtual void insert(Iterator& iter, MemBuffer& data)
    {
        iter.insert(data);
    }

    virtual void read(Iterator& iter, MemBuffer& data)
    {
    	iter.read(data);
    }

    virtual void remove(Iterator& iter, BigInt size) {
        iter.remove(size);
    }

    virtual void skip(Iterator& iter, BigInt offset)
    {
        iter.skip(offset);
    }

    virtual BigInt getPosition(Iterator& iter)
    {
        return iter.pos();
    }

    virtual BigInt getLocalPosition(Iterator& iter)
    {
        return iter.key_idx();
    }

    virtual BigInt getSize(Ctr& array)
    {
        return array.size();
    }

    ostream& out() {
        return Base::out();
    }

    virtual void checkIteratorPrefix(Iterator& iter, const char* source)
    {
    }
};



}


#endif
