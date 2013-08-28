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

    typedef VectorTreeTestBase                                                  Base;
    typedef VectorTreeRemoveTest                                                MyType;

    Int     iterations_     = 1000;
    Int     max_degree_     = 10;
    Int     remove_batch_   = 100;

public:

    VectorTreeRemoveTest(): VectorTreeTestBase("Remove")
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
        LoudsNode root = tree.seek(0).node();

        removeNode(tree, root, tree_node, size, max_size);
    }

    bool removeNode(Ctr& tree, const LoudsNode& node, TreeNode& tree_node, Int& size, Int max_size)
    {
        if (size < max_size)
        {
            for (Int c = 0; c < tree_node.children();)
            {
                LoudsNode child = tree.child(node, c).node();
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
        Allocator allocator;
        Ctr tree(&allocator);

        try {
            TreeNode root = this->fillRandom(tree, size_, max_degree_);

            allocator.commit();

            StoreResource(allocator, "rtree_c", 0);

            for (Int c = 0; c < iterations_ && tree.nodes() > 1; c++)
            {
                out()<<c<<std::endl;

                removeNodes(tree, root, remove_batch_);

                forceCheck(allocator, MA_SRC);

                checkTree(tree, root);

                allocator.commit();
            }

            StoreResource(allocator, "rtree_d", 0);
        }
        catch (...) {
            this->dump_name_ =  Store(allocator);
            throw;
        }
    }
};

}

#endif
