
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_STATICLOUDS_BASE_HPP_
#define MEMORIA_TESTS_STATICLOUDS_BASE_HPP_

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/core/tools/louds_tree.hpp>

#include <memory>

namespace memoria {

using namespace std;

class StaticLoudsTestBase: public TestTask {





public:

    StaticLoudsTestBase(StringRef name): TestTask(name)
    {

    }

    virtual ~StaticLoudsTestBase() throw() {}




    void checkTreeStructure(LoudsTree& tree, size_t nodeIdx, size_t parentIdx, size_t& count)
    {
    	count++;

    	if (nodeIdx > 0)
    	{
    		size_t parent = tree.parent(nodeIdx);

    		AssertEQ(MA_SRC, parent, parentIdx);
    	}

    	size_t child = tree.firstChild(nodeIdx);

    	while (child != LoudsTree::END)
    	{
    		checkTreeStructure(tree, child, nodeIdx, count);
    		child = tree.nextSibling(child);
    	}
    }

    void checkTreeStructure(LoudsTree& tree, size_t nodeIdx, size_t parentIdx)
    {
    	size_t count = 0;
    	checkTreeStructure(tree, nodeIdx, parentIdx, count);
    }

    void checkTreeStructure(LoudsTree& tree)
    {
    	if (tree.size() > 2)
    	{
    		size_t count = 0;
    		checkTreeStructure(tree, 0, 0, count);

    		AssertEQ(MA_SRC, count, tree.rank1(tree.size() - 1));
    	}
    	else
    	{
    		AssertEQ(MA_SRC, tree.size(), 0u);
    	}
    }

    template <typename T>
    LoudsTree createLouds(vector<T>& degrees)
    {
    	LoudsTree tree;

    	for (auto d: degrees)
    	{
    		tree.appendUDS(d);
    	}

    	tree.reindex();
    	return tree;
    }


    LoudsTree createRandomTree(size_t size, size_t max_children = 10)
    {
    	LoudsTree tree;

    	vector<size_t> level;

    	tree.appendUDS(1);

    	level.push_back(1);

    	size_t node_count = 0;

    	while (level.size() > 0)
    	{
    		vector<size_t> next_level;

    		for (size_t parent_degree: level)
    		{
    			for (size_t c = 0; c < parent_degree; c++)
    			{
    				size_t child_degree = getRandom(max_children);

    				if (child_degree + node_count > size) {
    					child_degree = 0;
    				}

    				next_level.push_back(child_degree);
    				node_count += child_degree;

    				tree.appendUDS(child_degree);
    			}

    		}

    		level = next_level;
    	}

    	tree.reindex();
    	return tree;
    }
};


}


#endif
