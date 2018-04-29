
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

#include <memoria/v1/tools/profile_tests.hpp>
#include <memoria/v1/tools/tools.hpp>

#include <memoria/v1/core/tools/labeled_tree.hpp>

#include <memoria/v1/containers/vector_tree/vtree_factory.hpp>

#include "../prototype/bt/bt_test_base.hpp"

#include <algorithm>
#include <sstream>
#include <memory>
#include <tuple>

namespace memoria {
namespace v1 {

using v1::tools::LblTreeNode;

class VectorTreeTestBase: public BTTestBase<VTree, PersistentInMemAllocator<>, DefaultProfile<>> {

    using MyType = VectorTreeTestBase;
    using Base   = BTTestBase<VTree, PersistentInMemAllocator<>, DefaultProfile<>>;

protected:

    using typename Base::Ctr;
    using typename Base::Iterator;
    using typename Base::CtrName;

    using Base::commit;
    using Base::drop;
    using Base::branch;
    using Base::allocator;
    using Base::snapshot;
    using Base::check;
    using Base::out;
    using Base::size_;
    using Base::storeAllocator;
    using Base::isReplayMode;
    using Base::getResourcePath;

    using Value = typename Ctr::Value;



    typedef LblTreeNode<std::vector<Value>, int8_t, int64_t>                      TreeNode;

    static const int32_t LabelNumber                                                = 2;

    int32_t max_data_size_ = 10;

public:

    VectorTreeTestBase(StringRef name): Base(name)
    {
        MEMORIA_ADD_TEST_PARAM(max_data_size_);
    }

    virtual ~VectorTreeTestBase() throw () {}

    TreeNode createRandomLabeledTree(int32_t size, int32_t node_degree = 10)
    {
        TreeNode root;

        int32_t tree_size = 1;
        createRandomLabeledTree(root, tree_size, size, node_degree);

        return root;
    }

    TreeNode fillRandom(Ctr& tree, int32_t size, int32_t max_degree = 10)
    {
        TreeNode tree_node = createRandomLabeledTree(size, max_degree);

        tree.prepare();

        LoudsNode root = tree.seek(0)->node();

        insertNode(tree, root, tree_node);

        return tree_node;
    }

    void insertNode(Ctr& tree, const LoudsNode& node, const TreeNode& tree_node)
    {
        auto iter = tree.seek(node.node());

        auto first_child = tree.insert(*iter.get(), std::get<0>(tree_node.labels()));

        iter->insert(tree_node.data());

//      assertTreeNode(tree, node, tree_node);

        for (int32_t c = 0; c < tree_node.children(); c++)
        {
            insertNode(tree, first_child, tree_node.child(c));
            first_child++;
        }
    }


    void checkTree(Ctr& tree, TreeNode& root_node)
    {
        int32_t size = 1;
        auto root = tree.seek(0)->node();

        checkTree(tree, root, root_node, size);

        AssertEQ(MA_SRC, size, tree.nodes());
    }





    void checkTreeStructure(Ctr& tree, const LoudsNode& node, LoudsNode parent)
    {
        int64_t count = 0;
        checkTreeStructure(tree, node, parent, count);
    }

    void checkTreeStructure(Ctr& tree)
    {
        if (tree.bitmap_size() > 2)
        {
            int64_t count = 0;
            checkTreeStructure(tree, LoudsNode(0, 1, 1), LoudsNode(0, 1, 1), count);

            AssertEQ(MA_SRC, count, tree.nodes());
        }
        else
        {
            AssertEQ(MA_SRC, tree.bitmap_size(), 0);
        }
    }

    void traverseTree(Ctr& tree, std::function<void (LoudsNode node)> fn)
    {
        auto root = tree.seek(0)->node();

        traverseTree(tree, root, fn);
    }

    void traverseTree(const TreeNode& node, std::function<void (const TreeNode& node)> fn)
    {
        fn(node);

        for (int32_t c = 0; c < node.children(); c++)
        {
            traverseTree(node.child(c), fn);
        }
    }

private:




    void createRandomLabeledTree(TreeNode& node, int32_t& size, int32_t max_size, int32_t max_degree, int32_t level = 0)
    {
        int32_t degree = level > 0 ? getRandom(max_degree) : max_degree;

        std::get<0>(node.labels())  = getRandom(256);

        int32_t data_size               = getRandom(max_data_size_);
        std::get<1>(node.labels())  = data_size;

        std::vector<Value> data(data_size);

        for (auto& v: data)
        {
            v = getRandom(256);
        }

        node.data() = data;


        if (level < 32)
        {
            for (int32_t c = 0; c < degree && size < max_size; c++)
            {
                TreeNode& child = node.appendChild();

                size++;
                createRandomLabeledTree(child, size, max_size, max_degree, level + 1);
            }
        }
    }


    void checkTreeStructure(Ctr& tree, const LoudsNode& node, const LoudsNode& parent, int64_t& count)
    {
        count++;

        if (node.node() > 0)
        {
            auto parentIdx = tree.parent(node)->node().node();
            AssertEQ(MA_SRC, parentIdx, parent.node());
        }

        auto children = tree.children(node);

        while (children->next_sibling())
        {
            checkTreeStructure(tree, children->node(), node, count);
        }
    }





    void assertTreeNode(Ctr& ctr, const LoudsNode& node, const TreeNode& tree_node)
    {
        auto labels = ctr.tree()->labels(node);
        AssertEQ(MA_SRC, labels, tree_node.labels());

        auto vtree_node = ctr.seek(node.node());
        auto data = vtree_node->read();

        AssertEQ(MA_SRC, data.size(), tree_node.data().size());

        try {
            for (uint32_t c = 0; c < data.size(); c++)
            {
                AssertEQ(MA_SRC, data[c], tree_node.data()[c], SBuf()<<c<<" "<<node.node());
            }
        }
        catch (...)
        {
            vtree_node = ctr.seek(node.node());

            vtree_node->tree_iter().dump();
            vtree_node->vector_iter().dump();

            data = vtree_node->read();

            throw;
        }
    }

    void checkTree(Ctr& tree, const LoudsNode& node, const TreeNode& tree_node, int32_t& size)
    {
        assertTreeNode(tree, node, tree_node);

        auto children = tree.children(node);

        int32_t child_idx = 0;
        while (children->next_sibling())
        {
            AssertLE(MA_SRC, child_idx, tree_node.children());
            checkTree(tree, children->node(), node, tree_node.child(child_idx), tree_node, size);

            child_idx++;
        }

        AssertEQ(MA_SRC, child_idx, tree_node.children());
    }

    void checkTree(
            Ctr& tree,
            const LoudsNode& node,
            const LoudsNode& parent,
            const TreeNode& tree_node,
            const TreeNode& tree_parent,
            int32_t& size
    )
    {
        assertTreeNode(tree, node, tree_node);

        size++;

        auto parentIdx = tree.parent(node)->node().node();
        AssertEQ(MA_SRC, parentIdx, parent.node());

        auto children = tree.children(node);

        int32_t child_idx = 0;
        while (children->next_sibling())
        {
            AssertLE(MA_SRC, child_idx, tree_node.children());
            checkTree(tree, children->node(), node, tree_node.child(child_idx), tree_node, size);

            child_idx++;
        }

        AssertEQ(MA_SRC, child_idx, tree_node.children());
    }


    void traverseTree(Ctr& tree, const LoudsNode& node, std::function<void (LoudsNode)> fn)
    {
        fn(node);

        auto children = tree.children(node);

        while(children->next_sibling())
        {
            traverseTree(tree, children->node(), fn);
        }
    }

};

}}
