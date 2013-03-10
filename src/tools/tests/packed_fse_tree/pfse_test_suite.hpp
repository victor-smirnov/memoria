
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PFSE_TEST_SUITE_HPP_
#define MEMORIA_TESTS_PFSE_TEST_SUITE_HPP_

#include "../tests_inc.hpp"


#include "pfse_init_test.hpp"
#include "pfse_create_test.hpp"
#include "pfse_find_test.hpp"

namespace memoria {

using namespace memoria::vapi;
using namespace std;

class PackedFSETestSuite: public TestSuite {

public:

    PackedFSETestSuite(): TestSuite("PackedFSETreeSuite")
    {
        registerTask(new PackedFSETreeInitTest());

    	registerTask(new PackedFSETreeCreateTest<32, 32>());
    	registerTask(new PackedFSETreeCreateTest<15, 25>());
    	registerTask(new PackedFSETreeCreateTest<44, 125>());
    	registerTask(new PackedFSETreeCreateTest<11, 18>());

    	registerTask(new PackedFSETreeFindTest<32, 32>());
    	registerTask(new PackedFSETreeFindTest<11, 33>());
    	registerTask(new PackedFSETreeFindTest<44, 125>());
    }

};

}


#endif

