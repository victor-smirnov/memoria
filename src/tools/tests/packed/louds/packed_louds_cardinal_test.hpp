
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


public:

    PackedLoudsCardinalTest(): PackedLoudsTestBase("Cardinal")
    {
    	MEMORIA_ADD_TEST(testCreateCardinalTree);
    }

    virtual ~PackedLoudsCardinalTest() throw() {}

    CardinalTree* createCardinalTree(Int block_size = 64*1024)
    {
    	PackedAllocator* alloc = T2T<PackedAllocator*>(malloc(block_size));

    	alloc->init(block_size, 1);

    	CardinalTree* tree = alloc->template allocateEmpty<CardinalTree>(0);

    	return tree;
    }

    UBigInt buildPath(PackedLoudsNode node, Int level, const CardinalTree* ctree)
    {
    	const LoudsTree* tree 		= ctree->tree();
    	const LabelArray* labels	= ctree->labels();

    	UBigInt path = 0;

    	for (Int l = level - 1; l >= 0; l--)
    	{
    		UBigInt label = labels->value(node.rank1() - 1);

    		path |= label << (8 * l);

    		node = tree->parent(node);
    	}

    	return path;
    }

    void checkTreeContent(const CardinalTree* tree, set<UBigInt>& paths)
    {
    	traverseTreePaths(tree->tree(), [this, tree, &paths](const PackedLoudsNode& node, Int level) {
    		AssertEQ(MA_SRC, level, 4);
    		UBigInt path = buildPath(node, level, tree);
    		AssertTrue(MA_SRC, paths.find(path) != paths.end());
    	});
    }

    void testCreateCardinalTree()
    {
    	CardinalTree* tree = createCardinalTree();
    	PARemover remover(tree);

    	tree->prepare();

    	auto fn = [](const PackedLoudsNode& node, Int label, Int level){};

    	set<UBigInt> paths;

    	for (Int c = 0; c < 1000; c++)
    	{
    		UInt path = getRandom();

    		out()<<c<<" "<<hex<<path<<dec<<endl;

    		paths.insert(path);

    		tree->insert_path(path, 4, fn);

    		checkTreeStructure(tree->tree());
    		checkTreeContent(tree, paths);
    	}

    	out()<<"Free space in the tree: "<<tree->free_space()<<endl;
    }
};


}


#endif
