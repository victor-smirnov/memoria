
// Copyright Victor Smirnov 2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PACKED_MAXTREE_TEST_SUITE_HPP_
#define MEMORIA_TESTS_PACKED_MAXTREE_TEST_SUITE_HPP_

#include "../../tests_inc.hpp"
#include "../maxtree/packed_maxtree_buffer_test.hpp"
#include "../maxtree/packed_maxtree_find_test.hpp"
#include "../maxtree/packed_maxtree_misc_test.hpp"

namespace memoria {

using namespace std;

class PackedMaxTreeTestSuite: public TestSuite {

public:

    PackedMaxTreeTestSuite(): TestSuite("Packed.MaxTree")
    {
//    	registerTask(new PackedMaxTreeMiscTest<PkdFMTreeT<BigInt, 1>>("Misc.1.FSM"));
//        registerTask(new PackedMaxTreeFindTest<PkdFMTreeT<BigInt, 1>>("Find.4.FSM"));

        registerTask(new PackedMaxTreeMiscTest<PkdVBMTreeT<BigInteger>>("Misc.1.VBM"));
        registerTask(new PackedMaxTreeFindTest<PkdVBMTreeT<BigInteger>>("Find.1.VBM"));
    }

};

}


#endif

