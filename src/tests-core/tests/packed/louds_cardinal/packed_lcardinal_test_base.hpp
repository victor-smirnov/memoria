
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

#include <memoria/core/packed/louds/packed_louds_cardinal_tree.hpp>
#include <memoria/core/packed/tools/packed_struct_ptrs.hpp>

#include <memory>

namespace memoria {

class PackedLoudsCardinalTreeTestBase: public TestTask {

protected:
    struct CardinalTreeTypes {
        static const size_t BitsPerLabel = 8;
    };

    using Tree = PackedLoudsCardinalTree<CardinalTreeTypes>;
    using TreePtr = PkdStructSPtr<Tree>;


    typedef typename Tree::LoudsTree                                            LoudsTree;

public:

    PackedLoudsCardinalTreeTestBase(StringRef name): TestTask(name)
    {}

    virtual ~PackedLoudsCardinalTreeTestBase() noexcept {}

    TreePtr createEmptyTree(size_t block_size = 1024*1024)
    {
        return MakeSharedPackedStructByBlock<Tree>(block_size);
    }

    void traverseTreePaths(const TreePtr& ctree, function<void (const PackedLoudsNode&, size_t)> fn, size_t level = 0)
    {
        auto tree = ctree->tree();
        traverseTreePaths(tree, tree->root(), PackedLoudsNode(), fn, level);
    }

    void traverseTreePaths(
            const LoudsTree* tree,
            const PackedLoudsNode& node,
            const PackedLoudsNode& parent,
            function<void (const PackedLoudsNode&, size_t)> fn, size_t level = 0)
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

};


}
