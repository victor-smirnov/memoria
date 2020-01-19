
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

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/core/tools/louds_tree.hpp>

#include "staticlouds_test_base.hpp"

#include <memory>

namespace memoria {

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

    virtual ~StaticLoudsSubtreeTest() noexcept {}


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
