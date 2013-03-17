
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PACKEDLOUDS_CARDINAL_HPP_
#define MEMORIA_TESTS_PACKEDLOUDS_CARDINAL_HPP_

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include "packed_louds_test_base.hpp"
#include <memoria/core/packed2/packed_louds_cardinal_tree.hpp>

#include <memory>

namespace memoria {

using namespace std;

class PackedLoudsCardinalTest: public PackedLoudsTestBase {

	typedef PackedLoudsTestBase													Base;
    typedef PackedLoudsCardinalTest 											MyType;

    struct CardinalTreeTypes {
    	static const Int BitsPerLabel = 8;
    };

    typedef PackedLoudsCardinalTree<CardinalTreeTypes>							CardinalTree;

    typedef typename CardinalTree::LoudsTree 									LoudsTree;
    typedef typename CardinalTree::LabelArray									LabelArray;

    typedef shared_ptr<CardinalTree>											CardinalTreePtr;

public:

    PackedLoudsCardinalTest(): PackedLoudsTestBase("Cardinal")
    {
    	MEMORIA_ADD_TEST(testCreateCardinalTree);
    }

    virtual ~PackedLoudsCardinalTest() throw() {}

    CardinalTreePtr createCardinalTree(Int nodes, Int free_space = 0)
    {
    	free_space = PackedAllocator::roundDownBytesToAlignmentBlocks(free_space);

    	Int block_size  = CardinalTree::block_size(nodes);

    	CardinalTree* tree = T2T<CardinalTree*>(malloc(block_size + free_space));

    	tree->init(block_size, nodes);

    	tree->enlarge(free_space);

    	return CardinalTreePtr(tree, free);
    }

    UBigInt buildPath(PackedLoudsNode node, Int level, const CardinalTreePtr& tree_ptr)
    {
    	const LoudsTree* tree 		= tree_ptr->tree();
    	const LabelArray* labels	= tree_ptr->labels();

    	UBigInt path = 0;

    	for (Int l = level - 1; l >= 0; l--)
    	{
    		UBigInt label = labels->value(node.rank1() - 1);

    		path |= label << (8 * l);

    		node = tree->parent(node);
    	}

    	return path;
    }

    void checkTreeContent(const CardinalTreePtr& tree_ptr, set<UBigInt>& paths)
    {
    	traverseTreePaths(tree_ptr->tree(), [this, &tree_ptr, &paths](const PackedLoudsNode& node, Int level) {
    		AssertEQ(MA_SRC, level, 4);
    		UBigInt path = buildPath(node, level, tree_ptr);
    		AssertTrue(MA_SRC, paths.find(path) != paths.end());
    	});
    }

    void testCreateCardinalTree()
    {
    	CardinalTreePtr tree_ptr = createCardinalTree(100, 4096*20);
    	tree_ptr->prepare();

    	auto fn = [](const PackedLoudsNode& node, Int label, Int level){};

    	set<UBigInt> paths;

    	for (Int c = 0; c < 1000; c++)
    	{
    		UInt path = getRandom();

    		out()<<c<<" "<<hex<<path<<dec<<endl;

    		paths.insert(path);

    		tree_ptr->insert_path(path, 4, fn);

    		checkTreeStructure(tree_ptr->tree());
    		checkTreeContent(tree_ptr, paths);
    	}

    	tree_ptr->dump(out());
    	cout<<tree_ptr->free_space()<<endl;
    }
};


}


#endif
