// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_LABELEDTREE_ITER_TEST_HPP_
#define MEMORIA_TESTS_LABELEDTREE_ITER_TEST_HPP_

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

class LabeledTreeIterTest: public LabeledTreeTestBase {

    typedef LabeledTreeIterTest                                                  MyType;

    Int iterations_ = 1000;
    Int max_degree_ = 4;

public:

    LabeledTreeIterTest(): LabeledTreeTestBase("Iter")
    {
    	size_ = 20000;

    	MEMORIA_ADD_TEST_PARAM(iterations_);
    	MEMORIA_ADD_TEST_PARAM(max_degree_);

        MEMORIA_ADD_TEST(testIteratorCache);
    }

    virtual ~LabeledTreeIterTest() throw () {}

    void testIteratorCache()
    {
    	Allocator allocator;
    	Ctr ctr(&allocator);

    	this->fillRandom(ctr, size_, max_degree_);

    	allocator.commit();

    	StoreResource(allocator, "iter", 0);

    	auto skip_iter = ctr.seek(0);

    	assertIterator(skip_iter);

    	Int nodes = ctr.nodes();

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

    		Int node = getRandom(nodes / 2) + nodes / 2 - 1;
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
