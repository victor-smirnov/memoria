
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_STATICLOUDS_SUBTREE_HPP_
#define MEMORIA_TESTS_STATICLOUDS_SUBTREE_HPP_

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/core/tools/louds_tree.hpp>

#include "staticlouds_test_base.hpp"

#include <memory>

namespace memoria {

using namespace std;

class StaticLoudsSubtreeTest: public StaticLoudsTestBase {

    typedef StaticLoudsSubtreeTest 												MyType;



public:

    StaticLoudsSubtreeTest(): StaticLoudsTestBase("Subtree")
    {
//    	MEMORIA_ADD_TEST(testSubtree);
//    	MEMORIA_ADD_TEST(testSubtree1);
    	MEMORIA_ADD_TEST(testInsertAt);
    }

    virtual ~StaticLoudsSubtreeTest() throw() {}


    void testSubtree()
    {
    	LoudsTree tree = createRandomTree(100000);

    	size_t nodes = tree.rank1();

    	for (size_t c = 1; c <= nodes; c++)
    	{
    		size_t node = tree.select1(c);

    		LoudsTree subtree = tree.getSubtree(node);

    		checkTreeStructure(subtree);
    	}
    }

    void testSubtree1()
    {
    	LoudsTree tree = createRandomTree(100);

    	tree.dump();

    	LoudsTree tree1 = tree.getSubtree(0);

    	tree1.dump();

    	tree.traverseSubtree(0, [](size_t left, size_t right) {
    		cout<<left<<" "<<right<<endl;
    	});
    }

    void testInsertAt()
    {
    	LoudsTree tgt_tree = createRandomTree(1000);
    	LoudsTree src_tree = createRandomTree(200);

    	size_t tgt_nodes = tgt_tree.rank1();

    	for (size_t c = 2; c < tgt_nodes; c++)
    	{
    		size_t insert_at = tgt_tree.select1(c);

    		LoudsTree tmp = tgt_tree;

    		tmp.insertAt(insert_at, src_tree);

    		checkTreeStructure(tmp);
    	}
    }

};


}


#endif
