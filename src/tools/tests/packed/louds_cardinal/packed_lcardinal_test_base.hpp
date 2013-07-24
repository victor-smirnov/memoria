
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PACKED_C_LOUDS_BASE_HPP_
#define MEMORIA_TESTS_PACKED_C_LOUDS_BASE_HPP_

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/core/packed2/packed_louds_cardinal_tree.hpp>

#include <memory>

namespace memoria {

using namespace std;

class PackedLoudsCardinalTreeTestBase: public TestTask {

protected:
	struct CardinalTreeTypes {
		static const Int BitsPerLabel = 8;
	};

	typedef PackedLoudsCardinalTree<CardinalTreeTypes>							Tree;
	typedef typename Tree::LoudsTree											LoudsTree;

public:

    PackedLoudsCardinalTreeTestBase(StringRef name): TestTask(name)
    {}

    virtual ~PackedLoudsCardinalTreeTestBase() throw() {}

    Tree* createEmptyTree(Int block_size = 1024*1024)
    {
    	PackedAllocator* alloc = T2T<PackedAllocator*>(malloc(block_size));
    	alloc->init(block_size, 1);

    	return alloc->template allocateEmpty<Tree>(0);
    }

    void traverseTreePaths(const Tree* ctree, function<void (const PackedLoudsNode&, Int)> fn, Int level = 0)
    {
    	auto tree = ctree->tree();
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

};


}


#endif
