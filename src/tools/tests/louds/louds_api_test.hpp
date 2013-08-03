// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_LOUDS_LOUDS_API_TEST_HPP_
#define MEMORIA_TESTS_LOUDS_LOUDS_API_TEST_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include "louds_test_base.hpp"

#include <vector>
#include <algorithm>
#include <sstream>
#include <memory>

namespace memoria {

using namespace memoria::louds;

class LoudsApiTest: public LoudsTestBase {

    typedef LoudsApiTest                                                       	MyType;

    vector<char> degrees_ = {1,4,2,2,2,2,0,0,2,1,0,0,2,0,0,0,1,1,0,0,0};

    BigInt 	tree_size_ 	= 10000;
    Int 	iterations_ = 1000;
    Int 	nodes_ 		= 10;

    Int start_ = 0;
    Int rank_  = 1;

public:

    LoudsApiTest(): LoudsTestBase("Api")
    {
    	MEMORIA_ADD_TEST_PARAM(tree_size_);
    	MEMORIA_ADD_TEST_PARAM(iterations_);
    	MEMORIA_ADD_TEST_PARAM(nodes_);

    	MEMORIA_ADD_TEST_PARAM(start_);
    	MEMORIA_ADD_TEST_PARAM(rank_);

//    	MEMORIA_ADD_TEST(runParent);
        MEMORIA_ADD_TEST(testIteratorCache);
    }

    virtual ~LoudsApiTest() throw () {}

    void runParent()
    {
    	Allocator allocator;
    	Ctr tree(&allocator);

    	createLouds(tree, degrees_);

    	auto iter = tree.select1(1);

    	checkTreeStructure(tree);

    	BigInt count = tree.nodes();

    	AssertEQ(MA_SRC, count, tree.getSubtreeSize(tree.rootNode()));
    }

    void assertIterator(Iterator& iter)
    {
    	AssertEQ(MA_SRC, iter.pos(), iter.gpos());

    	if (!(iter.isEof() || iter.isBof()))
    	{
    		AssertEQ(MA_SRC, iter.rank1(), iter.ranki(1));
    	}
    }

    void testIteratorCache()
    {

    	Allocator allocator;
    	Ctr ctr(&allocator);

    	Int nodes = createRandomLouds(ctr, tree_size_ , nodes_);

    	allocator.commit();

    	StoreResource(allocator, "iter", 0);

    	auto skip_iter = ctr.seek(0);

    	assertIterator(skip_iter);

    	out()<<"Forward skip"<<std::endl;
    	while (!skip_iter.isEof())
    	{
    		skip_iter++;
    		assertIterator(skip_iter);
    	}
    	out()<<std::endl;

    	out()<<"Backward skip"<<std::endl;
    	while (!skip_iter.isBegin())
    	{
    		if (skip_iter.pos() == 0)
    		{
    			int a = 0; a++;
    		}

    		skip_iter--;
    		assertIterator(skip_iter);
    	}
    	out()<<std::endl;

    	out()<<"Forward skip"<<std::endl;
    	while (!skip_iter.isEof())
    	{
    		skip_iter++;
    		assertIterator(skip_iter);
    	}
    	out()<<std::endl;

    	out()<<"Backward skip"<<std::endl;
    	while (!skip_iter.isBegin())
    	{
    		skip_iter--;
    		assertIterator(skip_iter);
    	}
    	out()<<std::endl;

    	out()<<"Random forward select/skip"<<std::endl;
    	for (Int c = 0; c < iterations_; c++)
    	{
    		out()<<"FW: "<<c<<std::endl;

    		Int node = getRandom(nodes / 2);
    		auto iter = ctr.select1(node);

    		assertIterator(iter);

    		Int skip = getRandom(nodes / 2 - 1);

    		auto iter_select0 	= iter;
    		auto iter_select1 	= iter;

    		auto iter_skip 		= iter;

    		iter_select0.selectFw(skip, 0);
    		assertIterator(iter_select0);

    		iter_select1.selectFw(skip, 1);
    		assertIterator(iter_select1);

    		iter_skip.skipFw(skip * 2);
    		assertIterator(iter_skip);
    	}
    	out()<<std::endl;

    	out()<<"Random backward select/skip"<<std::endl;
    	for (Int c = 0; c < iterations_; c++)
    	{
    		out()<<"BW: "<<c<<std::endl;

    		Int node = getRandom(nodes / 2) + node / 2 - 1;
    		auto iter = ctr.select1(node);

    		assertIterator(iter);

    		Int skip = getRandom(nodes / 2);

    		auto iter_select0 	= iter;
    		auto iter_select1 	= iter;

    		auto iter_skip 		= iter;

    		iter_select0.selectBw(skip, 0);
    		assertIterator(iter_select0);

    		iter_select1.selectBw(skip, 1);
    		assertIterator(iter_select1);

    		iter_skip.skipBw(skip * 2);
    		assertIterator(iter_skip);
    	}
    	out()<<std::endl;

    	out()<<"Random forward/backward rank"<<std::endl;
    	for (Int c = 0; c < iterations_; c++)
    	{
    		out()<<"Rank: "<<c<<std::endl;

    		Int node = getRandom(ctr.size() / 2);
    		auto iter = ctr.seek(node);

    		assertIterator(iter);

    		Int skip = getRandom(nodes / 2 - 1);

    		auto iter_rankfw0 	= iter;
    		auto iter_rankfw1 	= iter;

    		iter_rankfw0.rank(skip, 0);
    		assertIterator(iter_rankfw0);

    		iter_rankfw0.rank(-skip, 0);
    		assertIterator(iter_rankfw0);

    		iter_rankfw1.rank(skip, 1);
    		assertIterator(iter_rankfw1);

    		iter_rankfw1.rank(-skip, 1);
    		assertIterator(iter_rankfw1);
    	}
    }
};

}

#endif
