
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_BIT_VECTOR_BITVECTOR_TEST_SUITE_HPP_
#define MEMORIA_TESTS_BIT_VECTOR_BITVECTOR_TEST_SUITE_HPP_

#include "../tests_inc.hpp"

#include "bitmap_test.hpp"
#include "bitvector_test.hpp"

#include <vector>

namespace memoria {

using namespace memoria::vapi;
using namespace std;

class BitVectorTestSuite: public TestSuite {

public:

    BitVectorTestSuite(): TestSuite("BitVectorSuite")
    {
    	registerTask(new BitmapTest<>("Bitmap"));
//        registerTask(new BitVectorTest<false>("Sparse"));
    }

};

}


#endif

