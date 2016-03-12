
// Copyright Victor Smirnov 2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_MULTIMAP_TEST_SUITE_HPP_
#define MEMORIA_TESTS_MULTIMAP_TEST_SUITE_HPP_


#include "../tests_inc.hpp"

#include "multimap_create_test.hpp"
#include "multimap_remove_test.hpp"

#include <vector>

namespace memoria {

using namespace std;

class MultiMapTestSuite: public TestSuite {

public:

    MultiMapTestSuite(): TestSuite("MMapSuite")
    {
        registerTask(new MultiMapCreateTest<Map<String, Vector<BigInt>>>("Create.S.I"));
        registerTask(new MultiMapCreateTest<Map<double, Vector<BigInt>>>("Create.D.I"));
        registerTask(new MultiMapCreateTest<Map<BigInt, Vector<BigInt>>>("Create.I.I"));
        registerTask(new MultiMapCreateTest<Map<BigInt, Vector<BigInt>>>("Create.I.S"));
        registerTask(new MultiMapCreateTest<Map<UUID,   Vector<BigInt>>>("Create.U.I"));

        registerTask(new MultiMapRemoveTest<Map<String, Vector<BigInt>>>("Remove.S.I"));
        registerTask(new MultiMapRemoveTest<Map<double, Vector<BigInt>>>("Remove.D.I"));
        registerTask(new MultiMapRemoveTest<Map<BigInt, Vector<BigInt>>>("Remove.I.I"));
        registerTask(new MultiMapRemoveTest<Map<BigInt, Vector<BigInt>>>("Remove.I.S"));
        registerTask(new MultiMapRemoveTest<Map<UUID,   Vector<BigInt>>>("Remove.U.I"));
    }

};

}


#endif

