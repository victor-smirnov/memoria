
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_SEQUENCE_SEQUENCE_CREATE_TEST_HPP_
#define MEMORIA_TESTS_SEQUENCE_SEQUENCE_CREATE_TEST_HPP_

#include <memoria/memoria.hpp>
#include <memoria/tools/tests.hpp>

#include "../shared/sequence_create_test_base.hpp"

#include <vector>

namespace memoria {

using namespace memoria::vapi;
using namespace std;

template <Int BitsPerSymbol, bool Dense = true>
class SequenceCreateTest: public SequenceCreateTestBase <
									Sequence<BitsPerSymbol, Dense>,
									SymbolSequence<BitsPerSymbol>
						  >
{
    typedef SequenceCreateTest<BitsPerSymbol, Dense>                            MyType;
    typedef MyType                                                              ParamType;
    typedef SymbolSequence<BitsPerSymbol>										MemBuffer;

    typedef SequenceCreateTestBase <
				Sequence<BitsPerSymbol, Dense>,
				SymbolSequence<BitsPerSymbol>
    		>                                                          			Base;

    typedef typename Base::Ctr               									Ctr;
    typedef typename Base::Iterator                                             Iterator;
    typedef typename Base::Accumulator                                          Accumulator;
    typedef typename Base::ID                                                   ID;
    typedef typename Ctr::Value                                                 Symbol;

    typedef typename Ctr::ElementType											T;

public:
    SequenceCreateTest(StringRef name):
        Base(name)
    {
    	Base::size_ 			= 32 * 1024 * 1024;
    	Base::max_block_size_ 	= 1024 * 128;
    	Base::check_size_ 		= 1000;
    }

    virtual Iterator seek(Ctr& array, BigInt pos) {
    	return array.seek(pos);
    }

    virtual void insert(Iterator& iter, MemBuffer& data)
    {
    	auto src = data.source();
    	iter.insert(src);
    }

    virtual void read(Iterator& iter, MemBuffer& data)
    {
    	auto tgt = data.target();
    	iter.read(tgt);
    }

    virtual void skip(Iterator& iter, BigInt offset)
    {
    	iter.skip(offset);
    }

    virtual BigInt getSize(Ctr& ctr) {
    	return ctr.size();
    }

    virtual BigInt getPosition(Iterator& iter) {
    	return iter.pos();
    }

    virtual MemBuffer createBuffer(Int size)
    {
    	MemBuffer data(size);

    	data.resize(size);

    	data.fillCells([](UBigInt& cell) {
    		cell = 0;
    	});

        return data;
    }

    virtual MemBuffer createRandomBuffer(Int size)
    {
    	MemBuffer data(size);

    	data.resize(size);

    	data.fillCells([](UBigInt& cell) {
    		cell = getBIRandom();
    	});

    	return data;
    }

    ostream& out() {
    	return Base::out();
    }



    virtual void remove(Iterator& iter, BigInt size)
    {
    	iter.remove(size);
    }
};




}


#endif
