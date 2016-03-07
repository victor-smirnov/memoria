// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_VTREE_REMOVE_TEST_HPP_
#define MEMORIA_TESTS_VTREE_REMOVE_TEST_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include "vtree_test_base.hpp"

#include <vector>
#include <algorithm>
#include <sstream>
#include <memory>

namespace memoria {

using namespace memoria::louds;

class VectorTreeRemoveTest: public VectorTreeTestBase {

    using Base      = VectorTreeTestBase;
    using MyType    = VectorTreeRemoveTest;

    Int     iterations_     = 1000;
    Int     max_degree_     = 10;
    Int     remove_batch_   = 100;

public:

    VectorTreeRemoveTest(): Base("Remove")
    {
        size_ = 20000;

        MEMORIA_ADD_TEST_PARAM(iterations_);
        MEMORIA_ADD_TEST_PARAM(max_degree_);
        MEMORIA_ADD_TEST_PARAM(remove_batch_);

        MEMORIA_ADD_TEST(testRemoveNodes);
    }

    virtual ~VectorTreeRemoveTest() throw () {}


    void removeNodes(Ctr& tree, TreeNode& tree_node, Int max_size)
    {
        Int size = 0;
        LoudsNode root = tree.seek(0)->node();

        removeNode(tree, root, tree_node, size, max_size);
    }

    bool removeNode(Ctr& tree, const LoudsNode& node, TreeNode& tree_node, Int& size, Int max_size)
    {
        if (size < max_size)
        {
            for (Int c = 0; c < tree_node.children();)
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

        auto tree = create<CtrName>(snp);

        TreeNode root = fillRandom(*tree.get(), size_, max_degree_);

        commit();

        auto ctr_name = tree->name();

        BigInt tree_nodes = tree->nodes();

        tree.reset();

        for (Int c = 0; c < iterations_ && tree_nodes; c++)
        {
            out()<<c<<std::endl;

            snp = branch();

            tree = find<CtrName>(snp, ctr_name);

            removeNodes(*tree.get(), root, remove_batch_);

            check(MA_SRC);

            checkTree(*tree.get(), root);
        }
    }
};

}

#endif
