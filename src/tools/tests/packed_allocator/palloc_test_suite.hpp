
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PALLOC_TEST_SUITE_HPP_
#define MEMORIA_TESTS_PALLOC_TEST_SUITE_HPP_

#include "../tests_inc.hpp"

#include "palloc_test.hpp"
#include "palloc_cxseq_test.hpp"
#include "palloc_cxmultiseq_test.hpp"
#include "palloc_bitvector_test.hpp"
#include "palloc_wtree_test.hpp"

namespace memoria {

using namespace memoria::vapi;
using namespace std;

class PackedAllocatorTestSuite: public TestSuite {

public:

	PackedAllocatorTestSuite(): TestSuite("PackedAllocatorSuite")
    {
		registerTask(new PackedAllocatorTest());
		registerTask(new PackedCxSequenceTest());
		registerTask(new PackedCxMultiSequenceTest());
		registerTask(new PackedBitVectorTest());
		registerTask(new PackedWaveletTreeTest());
    }

};

}


#endif

