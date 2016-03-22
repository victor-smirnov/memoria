
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include "ltree_test_base.hpp"

#include <vector>
#include <algorithm>
#include <sstream>
#include <memory>

namespace memoria {

using namespace memoria::louds;

class LabeledTreeCreateTest: public LabeledTreeTestBase {

    typedef LabeledTreeTestBase                                                 Base;
    typedef LabeledTreeCreateTest                                               MyType;

    Int     max_degree_ = 10;
    Int     iterations_ = 1;

public:

    LabeledTreeCreateTest(): LabeledTreeTestBase("Create")
    {
        size_ = 100000;

        MEMORIA_ADD_TEST_PARAM(max_degree_);
        MEMORIA_ADD_TEST_PARAM(iterations_);

        MEMORIA_ADD_TEST(testFillTree);
    }

    virtual ~LabeledTreeCreateTest() throw () {}

    void testFillTree()
    {
        auto snp = branch();

        auto tree = create<CtrName>(snp);

        tree->setNewPageSize(512);

        TreeNode root = fillRandom(*tree.get(), size_, max_degree_);

        check(MA_SRC);

        checkTree(*tree.get(), root);

        commit();
    }
};

}
