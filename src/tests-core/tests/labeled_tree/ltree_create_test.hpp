
// Copyright 2013 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


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

class LabeledTreeCreateTest: public LabeledTreeTestBase {

    typedef LabeledTreeTestBase                                                 Base;
    typedef LabeledTreeCreateTest                                               MyType;

    int32_t     max_degree_ = 10;
    int32_t     iterations_ = 1;

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

        ctr_check_tree(*tree.get(), root);

        commit();
    }
};

}
