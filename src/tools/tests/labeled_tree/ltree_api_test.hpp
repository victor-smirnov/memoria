// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_LABELEDTREE_API_TEST_HPP_
#define MEMORIA_TESTS_LABELEDTREE_API_TEST_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include "ltree_test_base.hpp"

#include <vector>
#include <algorithm>
#include <sstream>
#include <memory>

namespace memoria {

using namespace memoria::louds;

class LabeledTreeApiTest: public LabeledTreeTestBase {

    typedef LabeledTreeApiTest                                                  MyType;



    BigInt 	tree_size_ 	= 10000;
    Int 	iterations_ = 1000;
    Int 	nodes_ 		= 10;
    Int 	removes_	= 100;

    Int start_ = 0;
    Int rank_  = 4;

public:

    LabeledTreeApiTest(): LabeledTreeTestBase("Api")
    {
    	size_ = 20000;

    	MEMORIA_ADD_TEST_PARAM(tree_size_);
    	MEMORIA_ADD_TEST_PARAM(iterations_);
    	MEMORIA_ADD_TEST_PARAM(nodes_);
    	MEMORIA_ADD_TEST_PARAM(removes_);

    	MEMORIA_ADD_TEST_PARAM(start_);
    	MEMORIA_ADD_TEST_PARAM(rank_);


//        MEMORIA_ADD_TEST(testInsertBits);
//        MEMORIA_ADD_TEST(testFillTree);
        MEMORIA_ADD_TEST(testRemoveNodes);
    }

    virtual ~LabeledTreeApiTest() throw () {}

    void testInsertBits()
    {
    	Allocator allocator;
    	Ctr tree(&allocator);

    	try {
    		Iterator iter = tree.seek(0);

    		for (Int c = 0; c < size_; c++)
    		{
    			out()<<c<<std::endl;

    			if (getRandom(2))
    			{
    				UByte 	lbl1 = getRandom(256);
    				BigInt	lbl2 = getRandom(256);

    				tree.insertNode(iter, std::make_tuple(lbl1, lbl2));
    			}
    			else {
    				tree.insertZero(iter);
    			}

    			allocator.commit();

    			iter++;
    		}

    		allocator.commit();

    		StoreResource(allocator, "ltree", 0);
    	}
    	catch (...) {
    		this->dump_name_ =  Store(allocator);
    		throw;
    	}
    }

    void testFillTree()
    {
    	Allocator allocator;
    	Ctr tree(&allocator);

    	try {
    		TreeNode root = this->fillRandom(tree, size_, rank_);

    		allocator.commit();

    		StoreResource(allocator, "ftree", 0);

    		checkTree(tree, root);
    	}
    	catch (...) {
    		this->dump_name_ =  Store(allocator);
    		throw;
    	}
    }

    void removeNodes(Ctr& tree, TreeNode& tree_node, Int max_size)
    {
    	Int size = 0;
    	LoudsNode root = tree.seek(0).node();

    	removeNode(tree, root, tree_node, size, max_size);
    }

    bool removeNode(Ctr& tree, const LoudsNode& node, TreeNode& tree_node, Int& size, Int max_size)
    {
    	if (size < max_size)
    	{
    		for (Int c = 0; c < tree_node.children();)
    		{
    			LoudsNode child = tree.childNode(node, c);
    			if (removeNode(tree, child, tree_node.child(c), size, max_size))
    			{
    				tree_node.removeChild(c);
    			}
    			else {
    				c++;
    			}
    		}

    		if (tree_node.children() == 0)
    		{
    			if (getRandom(2) && tree.nodes() > 1)
    			{
    				tree.removeLeaf(node);
    				size++;

    				return true;
    			}
    		}
    	}

    	return false;
    }


    void testRemoveNodes()
    {
    	Allocator allocator;
    	Ctr tree(&allocator);

    	try {
    		TreeNode root = this->fillRandom(tree, size_, rank_);

    		StoreResource(allocator, "rtree_c", 0);

    		allocator.commit();

    		for (Int c = 0; c < iterations_ && tree.nodes() > 1; c++)
    		{
    			out()<<c<<std::endl;

    			removeNodes(tree, root, removes_);

    			checkTree(tree, root);

    			allocator.commit();
    		}

    		StoreResource(allocator, "rtree_d", 0);
    	}
    	catch (...) {
    		this->dump_name_ =  Store(allocator);
    		throw;
    	}
    }


    void assertIterator(Iterator& iter)
    {
    	AssertEQ(MA_SRC, iter.pos(), iter.gpos());

    	if (!(iter.isEof() || iter.isBof()))
    	{
    		AssertEQ(MA_SRC, iter.rank1(), iter.ranki(1));
    	}
    }
};

}

#endif
