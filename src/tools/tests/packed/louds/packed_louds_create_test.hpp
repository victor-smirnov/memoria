
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

#include <memoria/v1/tools/tests.hpp>
#include <memoria/v1/tools/tools.hpp>

#include "packed_louds_test_base.hpp"

#include <memory>

namespace memoria {
namespace v1 {

using namespace std;

class PackedLoudsCreateTest: public PackedLoudsTestBase {

    typedef PackedLoudsTestBase                                                 Base;
    typedef PackedLoudsCreateTest                                               MyType;

public:

    PackedLoudsCreateTest(): PackedLoudsTestBase("Create")
    {
        MEMORIA_ADD_TEST(testCreateRandom);
        MEMORIA_ADD_TEST(testRemoveAll);
    }

    virtual ~PackedLoudsCreateTest() throw() {}


    void testCreateRandom()
    {
        auto tree = createRandomTree(100000);

        Int nodes = tree->rank1();

        for (Int c = 1; c <= nodes; c++)
        {
            PackedLoudsNode node(tree->select1(c), c);

            checkTreeStructure(tree.get(), node);
        }
    }


    PackedLoudsNode findRandomLeaf(const LoudsTree* tree)
    {
        MEMORIA_V1_ASSERT_TRUE(tree->tree_size() >= 1);

        return findRandomLeaf(tree, tree->root());
    }

    PackedLoudsNode findRandomLeaf(const LoudsTree* tree, const PackedLoudsNode& node)
    {
        auto children = tree->children(node);

        if (children.length() > 0)
        {
            Int child = getRandom(children.length());

            return findRandomLeaf(tree, children.node(child));
        }
        else {
            return node;
        }
    }


    void testRemoveAll()
    {
        auto tree = createRandomTree(5000);

        while (tree->tree_size() > 1)
        {
            this->out()<<tree->tree_size()<<std::endl;

            auto leaf = findRandomLeaf(tree.get());
            tree->removeLeaf(leaf);

            checkTreeStructure(tree.get());
        }
    }
};


}}