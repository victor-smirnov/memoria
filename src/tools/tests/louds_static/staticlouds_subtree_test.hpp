
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/tools/tests.hpp>
#include <memoria/v1/tools/tools.hpp>

#include <memoria/v1/core/tools/louds_tree.hpp>

#include "staticlouds_test_base.hpp"

#include <memory>

namespace memoria {

using namespace std;

class StaticLoudsSubtreeTest: public StaticLoudsTestBase {

    typedef StaticLoudsSubtreeTest                                              MyType;

public:

    StaticLoudsSubtreeTest(): StaticLoudsTestBase("Subtree")
    {
        MEMORIA_ADD_TEST(testGetSubtree);

        MEMORIA_ADD_TEST(testInsertAtNode);
        MEMORIA_ADD_TEST(testInsertAtLeaf);

        MEMORIA_ADD_TEST(testRemoveSubtree);

    }

    virtual ~StaticLoudsSubtreeTest() throw() {}


    void testGetSubtree()
    {
        LoudsTree tree = createRandomTree(100000);

        size_t nodes = tree.rank1();

        for (size_t c = 1; c <= nodes; c++)
        {
            size_t node = tree.select1(c);

            LoudsTree subtree = tree.getSubtree(node);

            checkTreeStructure(subtree);
        }
    }

    void testInsertAtNode()
    {
        LoudsTree tgt_tree = createRandomTree(1000);

        size_t tgt_nodes = tgt_tree.rank1();

        for (size_t c = 2; c < tgt_nodes; c++)
        {
            LoudsTree src_tree = createRandomTree(10000);

            size_t insert_at = tgt_tree.select1(c);

            out()<<c<<" "<<insert_at<<" parent:"<<tgt_tree.parent(insert_at)<<endl;

            LoudsTree tmp = tgt_tree;

            tmp.insertAt(insert_at, src_tree);

            AssertEQ(MA_SRC, tmp.rank1(), tgt_nodes + src_tree.rank1());
            AssertEQ(MA_SRC, tmp.size(),  tgt_tree.size() + src_tree.size() - 1);

            checkTreeStructure(tmp);
        }
    }


    void testInsertAtLeaf()
    {
        LoudsTree tgt_tree = createRandomTree(1000);

        size_t tgt_nodes = tgt_tree.rank0();

        for (size_t c = 1; c <= tgt_nodes; c++)
        {
            size_t insert_at = tgt_tree.select0(c);

            if (tgt_tree[insert_at - 1] == 0)
            {
                LoudsTree src_tree = createRandomTree(10000);

                out()<<c<<" "<<insert_at<<" parent:"<<tgt_tree.parent(insert_at)<<endl;

                LoudsTree tmp = tgt_tree;

                tmp.insertAt(insert_at, src_tree);

                AssertEQ(MA_SRC, tmp.rank1(), tgt_tree.rank1() + src_tree.rank1() - 1);
                AssertEQ(MA_SRC, tmp.size(),  tgt_tree.size() + src_tree.size() - 3);

                checkTreeStructure(tmp);
            }
        }
    }


    void testRemoveSubtree()
    {
        LoudsTree tree = createRandomTree(1000);

        size_t tgt_nodes = tree.rank1();

        for (size_t c = 1; c <= tgt_nodes; c++)
        {
            LoudsTree tgt_tree = tree;

            size_t remove_at = tgt_tree.select1(c);

            out()<<c<<" "<<remove_at<<" parent:"<<tree.parent(remove_at)<<endl;

            size_t subtree_size = tgt_tree.getSubtreeSize(remove_at);
            tgt_tree.removeSubtree(remove_at);

            checkTreeStructure(tgt_tree);

            AssertEQ(MA_SRC, tgt_tree.rank1(), tree.rank1() - subtree_size);
        }
    }
};


}
