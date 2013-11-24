
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_VTREE_BASE_HPP_
#define MEMORIA_TESTS_VTREE_BASE_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/profile_tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/core/tools/labeled_tree.hpp>

#include <vector>
#include <algorithm>
#include <sstream>
#include <memory>
#include <tuple>

namespace memoria {

using memoria::tools::LblTreeNode;

class VectorTreeTestBase: public SPTestTask {

    typedef VectorTreeTestBase                                                  MyType;

protected:


    typedef typename SCtrTF<VTree>::Type                                        Ctr;

    typedef typename Ctr::Iterator                                              Iterator;
    typedef typename Ctr::Value                                                 Value;

    String dump_name_;

    typedef LblTreeNode<std::vector<Value>, UByte, BigInt>                      TreeNode;

    static const Int LabelNumber                                                = 2;

    Int max_data_size_ = 10;

public:

    VectorTreeTestBase(StringRef name): SPTestTask(name)
    {
    	Ctr::initMetadata();

        MEMORIA_ADD_TEST_PARAM(max_data_size_);
        MEMORIA_ADD_TEST_PARAM(dump_name_)->state();
    }

    virtual ~VectorTreeTestBase() throw () {}

    TreeNode createRandomLabeledTree(Int size, Int node_degree = 10)
    {
        TreeNode root;

        Int tree_size = 1;
        createRandomLabeledTree(root, tree_size, size, node_degree);

        return root;
    }

    TreeNode fillRandom(Ctr& tree, Int size, Int max_degree = 10)
    {
        TreeNode tree_node = createRandomLabeledTree(size, max_degree);

        tree.prepare();

        LoudsNode root = tree.seek(0).node();

        insertNode(tree, root, tree_node);

        return tree_node;
    }

    void insertNode(Ctr& tree, const LoudsNode& node, const TreeNode& tree_node)
    {
        auto iter = tree.seek(node.node());

        auto first_child = tree.insert(iter, std::get<0>(tree_node.labels()));

        iter.insert(tree_node.data());

//      assertTreeNode(tree, node, tree_node);

        for (Int c = 0; c < tree_node.children(); c++)
        {
            insertNode(tree, first_child, tree_node.child(c));
            first_child++;
        }
    }


    void checkTree(Ctr& tree, TreeNode& root_node)
    {
        Int size = 1;
        auto root = tree.seek(0).node();

        checkTree(tree, root, root_node, size);

        AssertEQ(MA_SRC, size, tree.nodes());
    }





    void checkTreeStructure(Ctr& tree, const LoudsNode& node, LoudsNode parent)
    {
        BigInt count = 0;
        checkTreeStructure(tree, node, parent, count);
    }

    void checkTreeStructure(Ctr& tree)
    {
        if (tree.bitmap_size() > 2)
        {
            BigInt count = 0;
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
        auto root = tree.seek(0).node();

        traverseTree(tree, root, fn);
    }

    void traverseTree(const TreeNode& node, std::function<void (const TreeNode& node)> fn)
    {
        fn(node);

        for (Int c = 0; c < node.children(); c++)
        {
            traverseTree(node.child(c), fn);
        }
    }

private:




    void createRandomLabeledTree(TreeNode& node, Int& size, Int max_size, Int max_degree, Int level = 0)
    {
        Int degree = level > 0 ? getRandom(max_degree) : max_degree;

        std::get<0>(node.labels())  = getRandom(256);

        Int data_size               = getRandom(max_data_size_);
        std::get<1>(node.labels())  = data_size;

        std::vector<Value> data(data_size);

        for (auto& v: data)
        {
            v = getRandom(256);
        }

        node.data() = data;


        if (level < 32)
        {
            for (Int c = 0; c < degree && size < max_size; c++)
            {
                TreeNode& child = node.appendChild();

                size++;
                createRandomLabeledTree(child, size, max_size, max_degree, level + 1);
            }
        }
    }


    void checkTreeStructure(Ctr& tree, const LoudsNode& node, const LoudsNode& parent, BigInt& count)
    {
        count++;

        if (node.node() > 0)
        {
            BigInt parentIdx = tree.parent(node).node().node();
            AssertEQ(MA_SRC, parentIdx, parent.node());
        }

        Iterator children = tree.children(node);

        while (children.next_sibling())
        {
            checkTreeStructure(tree, children.node(), node, count);
        }
    }





    void assertTreeNode(Ctr& ctr, const LoudsNode& node, const TreeNode& tree_node)
    {
        auto labels = ctr.tree().labels(node);
        AssertEQ(MA_SRC, labels, tree_node.labels());

        auto vtree_node = ctr.seek(node.node());
        auto data = vtree_node.read();

        AssertEQ(MA_SRC, data.size(), tree_node.data().size());

        try {
            for (UInt c = 0; c < data.size(); c++)
            {
                AssertEQ(MA_SRC, data[c], tree_node.data()[c], SBuf()<<c<<" "<<node.node());
            }
        }
        catch (...)
        {
            vtree_node = ctr.seek(node.node());

            vtree_node.tree_iter().dump();
            vtree_node.vector_iter().dump();

            data = vtree_node.read();

            throw;
        }
    }

    void checkTree(Ctr& tree, const LoudsNode& node, const TreeNode& tree_node, Int& size)
    {
        assertTreeNode(tree, node, tree_node);

        Iterator children = tree.children(node);

        Int child_idx = 0;
        while (children.next_sibling())
        {
            AssertLE(MA_SRC, child_idx, tree_node.children());
            checkTree(tree, children.node(), node, tree_node.child(child_idx), tree_node, size);

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
            Int& size
    )
    {
        assertTreeNode(tree, node, tree_node);

        size++;

        BigInt parentIdx = tree.parent(node).node().node();
        AssertEQ(MA_SRC, parentIdx, parent.node());

        Iterator children = tree.children(node);

        Int child_idx = 0;
        while (children.next_sibling())
        {
            AssertLE(MA_SRC, child_idx, tree_node.children());
            checkTree(tree, children.node(), node, tree_node.child(child_idx), tree_node, size);

            child_idx++;
        }

        AssertEQ(MA_SRC, child_idx, tree_node.children());
    }


    void traverseTree(Ctr& tree, const LoudsNode& node, std::function<void (LoudsNode)> fn)
    {
        fn(node);

        Iterator children = tree.children(node);

        while(children.next_sibling())
        {
            traverseTree(tree, children.node(), fn);
        }
    }

};

}

#endif
