
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_LOUDS_BASE_HPP_
#define MEMORIA_TESTS_LOUDS_BASE_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/profile_tests.hpp>
#include <memoria/tools/tools.hpp>

#include <vector>
#include <algorithm>
#include <sstream>
#include <memory>

namespace memoria {

using namespace memoria::louds;

class LoudsTestBase: public SPTestTask {

    typedef LoudsTestBase                                                     	MyType;

protected:
    typedef typename SCtrTF<LOUDS>::Type 										Ctr;
    typedef typename Ctr::Iterator 												Iterator;

public:

    LoudsTestBase(StringRef name): SPTestTask(name)
    {}

    virtual ~LoudsTestBase() throw () {}

    BigInt createRandomLouds(Ctr& tree, size_t size, size_t max_children = 10)
    {
    	Iterator iter = tree.begin();
    	iter.insertDegree(1);

    	vector<BigInt> level;
    	level.push_back(1);

    	size_t node_count = 0;

    	while (level.size() > 0)
    	{
    		vector<BigInt> next_level;

    		for (size_t parent_degree: level)
    		{
    			for (size_t c = 0; c < parent_degree; c++)
    			{
    				size_t child_degree = getRandom(max_children);

    				if (child_degree + node_count > size)
    				{
    					child_degree = 0;
    				}

    				next_level.push_back(child_degree);
    				node_count += child_degree;

    				iter.insertDegree(child_degree);
    			}

    		}

    		level = next_level;
    	}

//    	size_t nodes_count = 1;
//
//    	Iterator iter = tree.begin();
//
//    	iter.insertDegree(1);
//
//    	size_t last_nodes = 1;
//
//    	while (nodes_count <= size)
//    	{
//    		size_t count = 0;
//
//    		for (size_t c = 0; c < last_nodes; c++)
//    		{
//    			size_t children = getRandom(max_children);
//
//    			if (last_nodes == 1 && children == 0)
//    			{
//    				while ((children = getRandom(max_children)) == 0);
//    			}
//
//    			if (nodes_count + children <= size)
//    			{
//    				iter.insertDegree(children);
//    				count += children;
//    				nodes_count += children;
//    			}
//    			else {
//    				goto exit;
//    			}
//    		}
//
//    		if (count > 0)
//    		{
//    			last_nodes = count;
//    		}
//    		else {
//    			break;
//    		}
//    	}
//
//    	exit:
//
//    	BigInt remainder = 2 * (nodes_count + 1) + 1 - tree.ctr().size();
//
//    	cout<<"size "<<tree.ctr().size()<<" remainder "<<remainder<<endl;
//
//    	iter.insertZeroes(remainder);
//
    	return node_count;
    }

    void createLouds(Ctr& tree, vector<char> degrees)
    {
    	Iterator iter = tree.begin();

    	for (auto d: degrees)
    	{
    		iter.insertDegree(d);
    	}
    }


    void checkTreeStructure(Ctr& tree, const LoudsNode& node, const LoudsNode& parent, BigInt& count)
    {
    	count++;

    	if (node.node() > 0)
    	{
    		BigInt parentIdx = tree.parentNode(node).node();
    		AssertEQ(MA_SRC, parentIdx, parent.node());
    	}

    	LoudsNodeRange children = tree.children(node);

    	AssertNEQ(MA_SRC, node.node(), children.node());

    	for (LoudsNode child = children.first(); child < children.last(); child++)
    	{
    		checkTreeStructure(tree, child, node, count);
    	}
    }



    void checkTreeStructure(Ctr& tree, const LoudsNode& node, LoudsNode parent)
    {
    	BigInt count = 0;
    	checkTreeStructure(tree, node, parent, count);
    }

    void checkTreeStructure(Ctr& tree)
    {
    	if (tree.size() > 2)
    	{
    		BigInt count = 0;
    		checkTreeStructure(tree, LoudsNode(0, 1, 1), LoudsNode(0, 1, 1), count);

    		AssertEQ(MA_SRC, count, tree.nodes());
    	}
    	else
    	{
    		AssertEQ(MA_SRC, tree.size(), 0);
    	}
    }


};

}

#endif
