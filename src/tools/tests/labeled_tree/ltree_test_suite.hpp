
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once


#include "../tests_inc.hpp"

#include "ltree_iter_test.hpp"
#include "ltree_create_test.hpp"
#include "ltree_remove_test.hpp"

#include <vector>

namespace memoria {
namespace v1 {

using namespace std;

class LabeledTreeTestSuite: public TestSuite {

public:

    LabeledTreeTestSuite(): TestSuite("LabeledTree")
    {
        registerTask(new LabeledTreeIterTest());
        registerTask(new LabeledTreeCreateTest());
        registerTask(new LabeledTreeRemoveTest());
    }

};

}}