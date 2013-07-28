
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

		ctr.insert(0, 1);

	}
};



}


#endif
