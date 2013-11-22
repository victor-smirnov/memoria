
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_VECTOR_DBLMAP_TEST_SUITE_HPP_
#define MEMORIA_TESTS_VECTOR_DBLMAP_TEST_SUITE_HPP_

#include "../tests_inc.hpp"

#include "dblmap_create_test.hpp"
#include "dblmap_remove_test.hpp"

#include "dblmap2_test.hpp"

namespace memoria {

class DblMapTestSuite: public TestSuite {

public:

    DblMapTestSuite(): TestSuite("DblMapSuite")
    {
//        registerTask(new DblMapCreateTest<DblMap<BigInt, BigInt>>("Create"));
        registerTask(new DblMapCreateTest<DblMrkMap2<BigInt, BigInt, 1>>("Mrk.Create"));
//
//        registerTask(new DblMapRemoveTest<DblMap<BigInt, BigInt>>("Remove"));
        registerTask(new DblMapRemoveTest<DblMrkMap2<BigInt, BigInt, 1>>("Mrk.Remove"));

//    	registerTask(new DblMap2Test("Mrk.Misc"));
    }

};

}


#endif

