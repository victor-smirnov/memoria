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

#include <memoria/v1/memoria.hpp>

#include <memoria/v1/tools/tests.hpp>
#include <memoria/v1/tools/tools.hpp>

#include "ltree_test_base.hpp"

#include <vector>
#include <algorithm>
#include <sstream>
#include <memory>

namespace memoria {
namespace v1 {

class LabeledTreeRemoveTest: public LabeledTreeTestBase {

    typedef LabeledTreeTestBase                                                 Base;
    typedef LabeledTreeRemoveTest                                               MyType;

    int32_t     iterations_     = 200;
    int32_t     max_degree_     = 10;
    int32_t     remove_batch_   = 100;

public:

    LabeledTreeRemoveTest(): LabeledTreeTestBase("Remove")
    {
        size_ = 40000;

        MEMORIA_ADD_TEST_PARAM(iterations_);
        MEMORIA_ADD_TEST_PARAM(max_degree_);
        MEMORIA_ADD_TEST_PARAM(remove_batch_);

        MEMORIA_ADD_TEST(testRemoveNodes);
    }

    virtual ~LabeledTreeRemoveTest() throw () {}


    void removeNodes(Ctr& tree, TreeNode& tree_node, int32_t max_size)
    {
        int32_t size = 0;
        LoudsNode root = tree.seek(0)->node();

        removeNode(tree, root, tree_node, size, max_size);
    }

    bool removeNode(Ctr& tree, const LoudsNode& node, TreeNode& tree_node, int32_t& size, int32_t max_size)
    {
        if (size < max_size)
        {
            for (int32_t c = 0; c < tree_node.children();)
            {
                LoudsNode child = tree.child(node, c)->node();
                if (removeNode(tree, child, tree_node.child(c), size, max_size))
                {
                    tree_node.removeChild(c);
                }
                else {
                    c++;
                }
            }

            if (tree_node.children() == 0)
            {
                if (getRandom(2) && tree.nodes() > 1)
                {
                    tree.removeLeaf(node);
                    size++;

                    return true;
                }
            }
        }

        return false;
    }


    void testRemoveNodes()
    {
        auto snp = branch();

        UUID ctr_name;
        int64_t nodes;
        TreeNode root;

        {
            auto tree = create<CtrName>(snp);
            ctr_name = tree->name();

            root = fillRandom(*tree.get(), size_, max_degree_);

            nodes = tree->nodes();

            check(MA_SRC);
            commit();
        }

        out()<<"Tree created"<<endl;

        for (int32_t c = 0; c < iterations_ && nodes > 1; c++)
        {
            out()<<c<<std::endl;

            snp = branch();

            auto tree = find<CtrName>(snp, ctr_name);

            removeNodes(*tree.get(), root, remove_batch_);

            nodes = tree->nodes();

            check(MA_SRC);

            checkTree(*tree.get(), root);

            commit();
        }
    }
};

}}
