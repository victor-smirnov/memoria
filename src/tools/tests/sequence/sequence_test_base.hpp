
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_SEQUENCE_SEQUENCE_TEST_BASE_HPP_
#define MEMORIA_TESTS_SEQUENCE_SEQUENCE_TEST_BASE_HPP_

#include <memoria/memoria.hpp>
#include <memoria/tools/tests.hpp>



#include <vector>

namespace memoria {

using namespace memoria::vapi;
using namespace std;


template <Int BitsPerSymbol, bool Dense = true>
class SequenceTestBase: public SPTestTask {
	typedef SequenceTestBase<BitsPerSymbol, Dense> 								MyType;
	typedef SPTestTask															Base;

protected:

	typedef typename SCtrTF<Sequence<BitsPerSymbol, Dense> >::Type              Ctr;
	typedef typename Ctr::Iterator                                              Iterator;
	typedef typename Ctr::Accumulator                                           Accumulator;
	typedef typename Ctr::ID                                                    ID;

	String dump_name_;

public:
	SequenceTestBase(StringRef name): Base(name)
	{
		MEMORIA_ADD_TEST_PARAM(dump_name_)->state();
	}

	void fillRandom(Ctr& ctr, Int size)
	{

	}



};



}


#endif
