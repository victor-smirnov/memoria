
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_SEQUENCE_SEQUENCE_CREATE_TEST_HPP_
#define MEMORIA_TESTS_SEQUENCE_SEQUENCE_CREATE_TEST_HPP_

#include <memoria/memoria.hpp>
#include <memoria/tools/tests.hpp>

#include "sequence_test_base.hpp"

#include <vector>

namespace memoria {

using namespace memoria::vapi;
using namespace std;

template <Int BitsPerSymbol, bool Dense = true>
class SequenceCreateTest: public SequenceTestBase<BitsPerSymbol, Dense> {
	typedef SequenceCreateTest<BitsPerSymbol, Dense> 							MyType;
	typedef SequenceTestBase<BitsPerSymbol, Dense> 								Base;

	typedef typename Base::Allocator											Allocator;
	typedef typename Base::Iterator												Iterator;
	typedef typename Base::Ctr													Ctr;

public:

	SequenceCreateTest(StringRef name): Base(name)
	{
		MEMORIA_ADD_TEST(testCreate);
	}

	void testCreate()
	{
		Allocator allocator;

		Ctr ctr(&allocator);

		allocator.commit();

		try {
			for (Int c = 0; c < this->size_; c++)
			{
				Int bit1 = getRandom(2);
				Int idx  = getRandom(c + 1);

				this->out()<<c<<" "<<idx<<std::endl;

				ctr.insert(idx , bit1);

				Int bit2 = ctr.symbol(idx);

				AssertEQ(MA_SRC, bit1, bit2);
			}

			AssertEQ(MA_SRC, ctr.size(), this->size_);

			allocator.commit();

			this->StoreAllocator(allocator, this->getResourcePath("alloc1.dump"));


			BigInt size = ctr.size();

			for (Int c = 0; c < size; c++)
			{
				Int idx = getRandom(size - c);

				ctr.remove(idx);

				AssertEQ(MA_SRC, ctr.size(), size - c - 1);
			}


			allocator.commit();

			this->StoreAllocator(allocator, this->getResourcePath("alloc2.dump"));
		}
		catch (...) {
			Base::dump_name_ = Base::Store(allocator);
			throw;
		}
	}
};



}


#endif
