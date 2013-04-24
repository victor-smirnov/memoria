
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_VECTOR_ITER_TEST_HPP_
#define MEMORIA_TESTS_VECTOR_ITER_TEST_HPP_

#include <memoria/memoria.hpp>
#include <memoria/tools/tests.hpp>

#include <vector>

namespace memoria {

using namespace memoria::vapi;
using namespace std;

template <typename T>
class VectorIteratorTest: public SPTestTask {
    typedef VectorIteratorTest                                                  MyType;
    typedef SPTestTask                                                          Base;


    typedef typename SCtrTF<Vector<T>>::Type                                    Ctr;
    typedef typename Ctr::Iterator                                              Iterator;

    typedef typename Ctr::Accumulator                                           Accumulator;

    Int distance_ = 100;


public:
    VectorIteratorTest(StringRef name):
        Base(name)
    {
        MEMORIA_ADD_TEST_PARAM(distance_);

//    	MEMORIA_ADD_TEST(runSkipFwTest);
    	MEMORIA_ADD_TEST(runSkipBwTest);
    }

    void checkIteratorPrefix(Iterator& iter, const char* source)
    {
    	Accumulator prefixes;
    	iter.ComputePrefix(prefixes);

    	if (iter.prefixes() != prefixes)
    	{
    		iter.dump(out());
    		throw TestException(source, SBuf()<<"Invalid prefix value. Iterator: "<<iter.prefixes()<<" Actual: "<<prefixes);
    	}
    }

    void runSkipFwTest()
    {
    	Allocator allocator;
    	Ctr ctr(&allocator);

    	vector<T> buf(100000);

    	ctr<<buf;

    	for (Int start = 0; start < ctr.size(); start += 500)
    	{
    		for (Int c = 0; c < ctr.size() - start; c++)
    		{
    			Iterator i = ctr.seek(start);

    			Int distance = i.skip(c);

    			checkIteratorPrefix(i, MA_SRC);

    			AssertEQ(MA_SRC, c + start, i.pos());
    			AssertEQ(MA_SRC, c, distance);
    		}
    	}

    	for (Int c = 1; c < ctr.size(); c++)
    	{
    		Iterator i = ctr.seek(c);

    		Int distance = i.skip(ctr.size());

    		checkIteratorPrefix(i, MA_SRC);

    		AssertEQ(MA_SRC, i.pos(), ctr.size());
    		AssertEQ(MA_SRC, distance, ctr.size() - c);
    	}
    }

    void runSkipBwTest()
    {
    	Allocator allocator;
    	Ctr ctr(&allocator);

    	vector<T> buf(100000);

    	ctr<<buf;

    	for (Int start = ctr.size() - 1; start > 0; start -= 500)
    	{
    		for (Int c = 0; c < start; c++)
    		{
    			Iterator i = ctr.seek(start);

    			Int distance = i.skip(-c);

    			checkIteratorPrefix(i, MA_SRC);

    			AssertEQ(MA_SRC, start - c, i.pos());
    			AssertEQ(MA_SRC, c, distance);
    		}
    	}

    	for (Int c = 0; c < ctr.size(); c++)
    	{
    		Iterator i = ctr.seek(c);

    		Int distance = i.skip(-ctr.size());

    		checkIteratorPrefix(i, MA_SRC);

    		AssertEQ(MA_SRC, i.pos(), -1);
    		AssertEQ(MA_SRC, distance, c);
    	}
    }
};



}


#endif
