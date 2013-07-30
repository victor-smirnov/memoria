
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_SEQUENCE_SEQUENCE_RANK_TEST_HPP_
#define MEMORIA_TESTS_SEQUENCE_SEQUENCE_RANK_TEST_HPP_

#include <memoria/memoria.hpp>
#include <memoria/tools/tests.hpp>

#include "sequence_test_base.hpp"

#include <vector>

namespace memoria {


template <Int BitsPerSymbol, bool Dense = true>
class SequenceRankTest: public SequenceTestBase<BitsPerSymbol, Dense> {

	typedef SequenceCreateTest<BitsPerSymbol, Dense> 							MyType;
	typedef SequenceTestBase<BitsPerSymbol, Dense> 								Base;

	typedef typename Base::Allocator											Allocator;
	typedef typename Base::Iterator												Iterator;
	typedef typename Base::Ctr													Ctr;

    Int ctr_name_;


public:
    SequenceRankTest(StringRef name):
        Base(name)
    {
        Base::size_ = 2048*1024;
        MEMORIA_ADD_TEST_PARAM(ctr_name_)->state();


    }


};




}


#endif
