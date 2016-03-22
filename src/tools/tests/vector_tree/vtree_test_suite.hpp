
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "../tests_inc.hpp"

#include "vtree_create_test.hpp"
#include "vtree_remove_test.hpp"

#include <vector>

namespace memoria {

using namespace std;

class VTreeTestSuite: public TestSuite {

public:

    VTreeTestSuite(): TestSuite("VTree")
    {
        registerTask(new VectorTreeCreateTest());
        registerTask(new VectorTreeRemoveTest());
    }
};

}

