
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

#include <memoria/v1/containers/labeled_tree/ltree_factory.hpp>

#include "../prototype/bt/bt_test_base.hpp"

#include <vector>
#include <algorithm>
#include <sstream>
#include <memory>
#include <tuple>

namespace memoria {
namespace v1 {

using tools::LblTreeNode;

using LTreeCtrName = LabeledTree<
        FLabel<uint16_t>,
        VLabel<int64_t, Granularity::Bit, Indexed::Yes>
    >;

class LabeledTreeTestBase: public BTTestBase<LTreeCtrName, PersistentInMemAllocator<>, DefaultProfile<>> {

    using Base   = BTTestBase<LTreeCtrName, PersistentInMemAllocator<>, DefaultProfile<>>;
    using MyType = LabeledTreeTestBase;

protected:

    using typename Base::CtrName;
    using typename Base::Ctr;
    using typename Base::IteratorPtr;

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
    using Base::getRandom;
    using Base::getBIRandom;


    typedef LblTreeNode<EmptyType, uint16_t, int64_t>                              TreeNode;

    typedef typename Ctr::Types::LabelsTuple                                    LabelsTuple;

    static const int32_t LabelNumber                                                = 2;

public:

    LabeledTreeTestBase(StringRef name): Base(name)
    {
        Ctr::initMetadata();
    }

    virtual ~LabeledTreeTestBase() throw () {}

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

        auto iter = tree.seek(0);
        tree.insertZero(*iter.get());

        LoudsNode root = tree.seek(0)->node();

        insertNode(tree, root, tree_node);

        return tree_node;
    }

    void insertNode(Ctr& tree, const LoudsNode& node, const TreeNode& tree_node)
    {
        auto first_child = tree.newNodeAt(node, tree_node.labels());

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

        checkTree(tree, root, root_node, size, 0);

        AssertEQ(MA_SRC, size, tree.nodes());
    }





    void checkTreeStructure(Ctr& tree, const LoudsNode& node, LoudsNode parent)
    {
        int64_t count = 0;
        checkTreeStructure(tree, node, parent, count);
    }

    void checkTreeStructure(Ctr& tree)
    {
        if (tree.size() > 2)
        {
            int64_t count = 0;
            checkTreeStructure(tree, LoudsNode(0, 1, 1), LoudsNode(0, 1, 1), count);

            AssertEQ(MA_SRC, count, tree.nodes());
        }
        else
        {
            AssertEQ(MA_SRC, tree.size(), 0);
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


    void checkRawRanksSelects(Ctr& tree)
    {
        auto iter = tree.seek(0);
        auto size = tree.sizes()[0];

        for (int64_t c = 0; c < size; c++)
        {
            int64_t r0 = 0, r1 = 0;

            auto rank_iter = iter->clone();

            rank_iter->raw_rank(r0, r1, c);

            auto rr0 = tree.rank0(c - 1);
            auto rr1 = tree.rank1(c - 1);

            AssertEQ(MA_SRC, r0, rr0);
            AssertEQ(MA_SRC, r1, rr1);

            if (r0 > 0 && r1 > 0)
            {
                auto select0_iter = iter;
                auto select1_iter = iter;

                select0_iter->raw_select(0, r0);
                select1_iter->raw_select(1, r1);

                auto pps0 = tree.select0(r0)->pos();
                auto pps1 = tree.select1(r1)->pos();

                AssertEQ(MA_SRC, select0_iter->pos(), pps0);
                AssertEQ(MA_SRC, select1_iter->pos(), pps1);
            }
        }
    }

    void assertIterator(const char* msg, const IteratorPtr& iter)
    {
        AssertEQ(msg, iter->pos(), iter->gpos());

        if (!(iter->isEof() || iter->isBof()))
        {
            AssertEQ(msg, iter->rank1(), iter->ranki(1));
        }
    }

private:




    void createRandomLabeledTree(TreeNode& node, int32_t& size, int32_t max_size, int32_t max_degree, int32_t level = 0)
    {
        int32_t degree = level > 0 ? getRandom(max_degree) : max_degree;

        std::get<0>(node.labels()) = getRandom(256);
        std::get<1>(node.labels()) = getRandom(256);

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
            int64_t parentIdx = tree.parent(node)->pos();
            AssertEQ(MA_SRC, parentIdx, parent.node());
        }

        auto children = tree.children(node);

//      LoudsNodeRange children = tree.children(node);

//      AssertNEQ(MA_SRC, node.node(), children.node());

//      for (LoudsNode child = children.first(); child < children.last(); child++)
//      {
//          checkTreeStructure(tree, child, node, count);
//      }

        while (children->next_sibling())
        {
            checkTreeStructure(tree, children->node(), node, count);
        }
    }





    void assertTreeNode(Ctr& ctr, const LoudsNode& node, const TreeNode& tree_node)
    {
        auto labels = ctr.labels(node);

        if (labels != tree_node.labels()) {
            cout<<node<<" "<<labels<<" "<<tree_node<<endl;
        }

        AssertEQ(MA_SRC, labels, tree_node.labels());
    }

    void checkTree(Ctr& tree, const LoudsNode& node, const TreeNode& tree_node, int32_t& size, int32_t level)
    {
        assertTreeNode(tree, node, tree_node);

        auto children = tree.children(node);

        int32_t child_idx = 0;

        while (children->next_sibling())
        {
            AssertLT(MA_SRC, child_idx, tree_node.children());
            checkTree(tree, children->node(), node, tree_node.child(child_idx), tree_node, size, level+1);

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
            int32_t& size,
            int32_t level

    ){
        assertTreeNode(tree, node, tree_node);

        size++;

        int64_t parentIdx = tree.parent(node)->pos();
        AssertEQ(MA_SRC, parentIdx, parent.node());

        auto children = tree.children(node);

        int32_t child_idx = 0;
        while (children->next_sibling())
        {
            AssertLT(MA_SRC, child_idx, tree_node.children());
            checkTree(tree, children->node(), node, tree_node.child(child_idx), tree_node, size, level + 1);

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
