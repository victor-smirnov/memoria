// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_LOUDS_LOUDS_API_TEST_HPP_
#define MEMORIA_TESTS_LOUDS_LOUDS_API_TEST_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include <vector>
#include <algorithm>
#include <sstream>
#include <memory>

namespace memoria {

using namespace memoria::louds;

class LoudsApiTest: public SPTestTask {

    typedef LoudsApiTest                                                       	MyType;

    typedef typename SCtrTF<LOUDS>::Type 										Ctr;
    typedef typename Ctr::Iterator 												Iterator;

public:

    LoudsApiTest(): SPTestTask("API")
    {
        MEMORIA_ADD_TEST(runTest);
    }

    virtual ~LoudsApiTest() throw () {}

    size_t createRandomLouds(Ctr& tree, size_t size, size_t max_children = 10)
    {
    	size_t nodes_count = 1;

    	Iterator iter = tree.begin();

    	iter.insertDegree(1);

    	iter.insertDegree(1);

    	size_t last_nodes = 1;

    	while (nodes_count <= size)
    	{
    		size_t count = 0;

    		for (size_t c = 0; c < last_nodes; c++)
    		{
    			size_t children = getRandom(max_children);

    			if (last_nodes == 1 && children == 0)
    			{
    				while ((children = getRandom(max_children)) == 0);
    			}

    			if (nodes_count + children <= size)
    			{
    				iter.insertDegree(children);
    				count += children;
    				nodes_count += children;
    			}
    			else {
    				goto exit;
    			}
    		}

    		last_nodes = count;
    	}

    	exit:

    	BigInt remainder = 2 * (nodes_count + 1) + 1 - tree.ctr().size();

    	iter.insertZeroes(remainder);

    	return nodes_count;
    }


    void traverseTree(Ctr& tree, BigInt nodeIdx, BigInt parentIdx, BigInt& count)
    {
    	count++;

    	BigInt parent = tree.parentNode(nodeIdx).node();
    	AssertEQ(MA_SRC, parentIdx, parent);

    	LoudsNodeRange children = tree.children(nodeIdx);

    	AssertNEQ(MA_SRC, nodeIdx, children.node());

    	for (BigInt child = children.first(); child < children.last(); child++)
    	{
    		traverseTree(tree, child, nodeIdx, count);
    	}
    }


    void runTest()
    {
    	for (Int c = 1; c <= 10; c++)
    	{
    		Allocator allocator;

    		Ctr ctr(&allocator);

    		BigInt t0 = getTimeInMillis();

    		BigInt count0 = createRandomLouds(ctr, 100000 * c);

    		BigInt t1 = getTimeInMillis();

    		BigInt count1 = 0;

    		traverseTree(ctr, 2, 0, count1);

    		BigInt t2 = getTimeInMillis();

    		AssertEQ(MA_SRC, count0, count1);

    		out()<<"TreeSize: "<<count0<<" Tree Build Time: "<<FormatTime(t1-t0)<<", Traverse Time: "<<FormatTime(t2-t1)<<endl;
    	}
    }


};

}

#endif
