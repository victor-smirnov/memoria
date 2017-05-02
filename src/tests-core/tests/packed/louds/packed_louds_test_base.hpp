
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

#include <memoria/v1/core/packed/louds/packed_louds_tree.hpp>
#include <memoria/v1/core/packed/tools/packed_struct_ptrs.hpp>

#include <memory>

namespace memoria {
namespace v1 {

using namespace std;

class PackedLoudsTestBase: public TestTask {

protected:
    using LoudsTree = PackedLoudsTree<LoudsTreeTypes<>>;

    using LoudsTreePtr = PkdStructSPtr<LoudsTree>;

public:

    PackedLoudsTestBase(StringRef name): TestTask(name)
    {}

    virtual ~PackedLoudsTestBase() noexcept {}


    void checkTreeStructure(
            const LoudsTree* tree,
            const PackedLoudsNode& node,
            const PackedLoudsNode& parent,
            int32_t& count
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
        int32_t count = 0;

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
        int32_t count = 0;
        checkTreeStructure(tree, node, parent, count);
    }

    void checkTreeStructure(const LoudsTree* tree)
    {
        if (tree->size() > 2)
        {
            int32_t count = 0;
            checkTreeStructure(tree, tree->root(), tree->root(), count);

            AssertEQ(MA_SRC, count, tree->rank1(tree->size() - 1));
        }
        else
        {
            AssertEQ(MA_SRC, tree->size(), 0);
        }
    }

    void traverseTreePaths(const LoudsTree* tree, function<void (const PackedLoudsNode&, int32_t)> fn, int32_t level = 0)
    {
        traverseTreePaths(tree, tree->root(), PackedLoudsNode(), fn, level);
    }

    void traverseTreePaths(
            const LoudsTree* tree,
            const PackedLoudsNode& node,
            const PackedLoudsNode& parent,
            function<void (const PackedLoudsNode&, int32_t)> fn, int32_t level = 0)
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

    LoudsTreePtr createEmptyLouds(int32_t block_size = 64*1024)
    {
        return MakeSharedPackedStructByBlock<LoudsTree>(block_size);
    }

    template <typename T>
    LoudsTreePtr createLouds(vector<T>& degrees)
    {
        auto tree = createEmptyLouds();

        for (auto d: degrees)
        {
            tree->appendUDS(d);
        }

        tree->reindex();

        return tree;
    }

    LoudsTreePtr createRandomTree(int32_t size, int32_t max_children = 10)
    {
        auto tree = createEmptyLouds();

        vector<int32_t> level;

        tree->appendUDS(1);

        level.push_back(1);

        int32_t node_count = 0;

        while (level.size() > 0)
        {
            vector<int32_t> next_level;

            for (int32_t parent_degree: level)
            {
                for (int32_t c = 0; c < parent_degree; c++)
                {
                    int32_t child_degree = getRandom(max_children);

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


}}