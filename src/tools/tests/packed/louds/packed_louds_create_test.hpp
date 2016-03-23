
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

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
        MEMORIA_ASSERT_TRUE(tree->tree_size() >= 1);

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