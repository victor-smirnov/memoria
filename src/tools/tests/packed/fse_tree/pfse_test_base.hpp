
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PFSE_BASE_HPP_
#define MEMORIA_TESTS_PFSE_BASE_HPP_

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/prototypes/btree/tools.hpp>

#include <memoria/core/packed2/packed_vle_tree.hpp>

#include <memoria/core/tools/exint_codec.hpp>

#include <memory>

namespace memoria {

using namespace std;

template <typename Types_>
class PackedFSETestBase: public TestTask {
protected:
	typedef Types_							Types;
	typedef PackedFSETestBase 				MyType;


	typedef PackedFSETree<Types> 			Tree;

	typedef typename Tree::Value			Value;
	typedef typename Tree::Values			Values;

public:

	PackedFSETestBase(StringRef name): TestTask(name){}

    virtual ~PackedFSETestBase() throw() {}

    Tree* createTree(Int size)
    {
    	void* buffer = malloc(size);
    	memset(buffer, 0, size);

    	if (buffer)
    	{
    		PackedAllocator* alloc = T2T<PackedAllocator*>(buffer);
    		alloc->init(size, 1);

    		Tree* tree = alloc->template allocate<Tree>(0, alloc->client_area());

    		return tree;
    	}
    	else {
    		throw Exception(MA_SRC, SBuf()<<"Can't allocate "<<size<<" bytes");
    	}
    }

    void remove(Tree* tree)
    {
    	free(tree->allocator());
    }

    void fillTree(Tree* tree, function<Values ()> value_provider)
    {
    	fillTree(tree, tree->max_size(), value_provider);
    }

    void fillTree(Tree* tree, Int size, function<Values ()> value_provider)
    {
    	tree->insert(0, size, value_provider);
    	tree->reindex();
    }

    vector<Value> sumValues(const Tree* tree, Int start)
    {
    	vector<Value> values;

    	Value sum = 0;

    	for (Int c = start; c < tree->size(); c++)
    	{
    		sum += tree->value(c);
    		values.push_back(sum);
    	}

    	return values;
    }

    Value sumValues(const Tree* tree, Int start, Int stop) const
    {
    	Value sum = 0;

    	for (Int c = start; c < stop; c++)
    	{
    		sum += tree->value(c);
    	}

    	return sum;
    }

    void runFunctionFor(const vector<Int>& values, function<void (Int)> fn)
    {
    	for (auto value: values)
    	{
    		fn(value);
    	}
    }
};


}


#endif
