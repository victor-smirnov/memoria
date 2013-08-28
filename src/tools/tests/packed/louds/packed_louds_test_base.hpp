
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PACKED_LOUDS_BASE_HPP_
#define MEMORIA_TESTS_PACKED_LOUDS_BASE_HPP_

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/core/packed/louds/packed_louds_tree.hpp>

#include <memory>

namespace memoria {

using namespace std;

class PackedLoudsTestBase: public TestTask {

protected:
    typedef PackedLoudsTree<LoudsTreeTypes<>>                                   LoudsTree;

public:

    PackedLoudsTestBase(StringRef name): TestTask(name)
    {}

    virtual ~PackedLoudsTestBase() throw() {}


    void checkTreeStructure(
            const LoudsTree* tree,
            const PackedLoudsNode& node,
            const PackedLoudsNode& parent,
            Int& count
        )
    {
        count++;

        if (node.idx() > 0)
        {
            PackedLoudsNode parent_node = tree->parent(node);

            AssertEQ(MA_SRC, parent.idx(), parent_node.idx());
        }

        PackedLoudsNode child = tree->first_child(node);

        if (child.idx() >= tree->size()) {
            tree->dump();
        }

        AssertLT(MA_SRC, child.idx(), tree->size());

        while (child != PackedLoudsNode() && !tree->isLeaf(child))
        {
            checkTreeStructure(tree, child, node, count);
            child = tree->right_sibling(child);
        }
    }

    void checkTreeStructure(const LoudsTree* tree, const PackedLoudsNode& node)
    {
        Int count = 0;

        if (node.idx() > 0)
        {
            const PackedLoudsNode parent = tree->parent(node);
            checkTreeStructure(tree, node, parent, count);
        }
        else {
            checkTreeStructure(tree, node, PackedLoudsNode(), count);
        }
    }

    void checkTreeStructure(const LoudsTree* tree, const PackedLoudsNode& node, const PackedLoudsNode& parent)
    {
        Int count = 0;
        checkTreeStructure(tree, node, parent, count);
    }

    void checkTreeStructure(const LoudsTree* tree)
    {
        if (tree->size() > 2)
        {
            Int count = 0;
            checkTreeStructure(tree, tree->root(), tree->root(), count);

            AssertEQ(MA_SRC, count, tree->rank1(tree->size() - 1));
        }
        else
        {
            AssertEQ(MA_SRC, tree->size(), 0);
        }
    }

    void traverseTreePaths(const LoudsTree* tree, function<void (const PackedLoudsNode&, Int)> fn, Int level = 0)
    {
        traverseTreePaths(tree, tree->root(), PackedLoudsNode(), fn, level);
    }

    void traverseTreePaths(
            const LoudsTree* tree,
            const PackedLoudsNode& node,
            const PackedLoudsNode& parent,
            function<void (const PackedLoudsNode&, Int)> fn, Int level = 0)
    {
        if (tree->isLeaf(node))
        {
            fn(parent, level - 1);
        }
        else
        {
            PackedLoudsNode child = tree->first_child(node);
            while (child != PackedLoudsNode() && !tree->isLeaf(child))
            {
                traverseTreePaths(tree, child, node, fn, level + 1);
                child = tree->right_sibling(child);
            }
        }
    }

    LoudsTree* createEmptyLouds(Int block_size = 64*1024)
    {
        PackedAllocator* alloc = T2T<PackedAllocator*>(malloc(block_size));
        alloc->init(block_size, 1);
        alloc->setTopLevelAllocator();

        return alloc->template allocateEmpty<LoudsTree>(0);
    }

    template <typename T>
    LoudsTree* createLouds(vector<T>& degrees)
    {
        LoudsTree* tree = createEmptyLouds();

        for (auto d: degrees)
        {
            tree->appendUDS(d);
        }

        tree->reindex();

        return tree;
    }

    LoudsTree* createRandomTree(Int size, Int max_children = 10)
    {
        LoudsTree* tree = createEmptyLouds();

        vector<Int> level;

        tree->appendUDS(1);

        level.push_back(1);

        Int node_count = 0;

        while (level.size() > 0)
        {
            vector<Int> next_level;

            for (Int parent_degree: level)
            {
                for (Int c = 0; c < parent_degree; c++)
                {
                    Int child_degree = getRandom(max_children);

                    if (child_degree + node_count > size)
                    {
                        child_degree = 0;
                    }

                    next_level.push_back(child_degree);
                    node_count += child_degree;

                    tree->appendUDS(child_degree);
                }

            }

            level = next_level;
        }

        tree->reindex();

        return tree;
    }
};


}


#endif
