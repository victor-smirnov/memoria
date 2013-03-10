
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

	typedef shared_ptr<Tree> 				TreePtr;

	typedef typename Tree::Value			Value;

public:

	PackedFSETestBase(StringRef name): TestTask(name){}

    virtual ~PackedFSETestBase() throw() {}

    TreePtr createTree(Int size)
    {
    	void* buffer = malloc(size);

    	if (buffer)
    	{
    		Tree* tree = T2T<Tree*>(buffer);

    		tree->initBlockSize(size - sizeof(Tree));

    		return TreePtr(tree);
    	}
    	else {
    		throw Exception(MA_SRC, SBuf()<<"Can't allocate "<<size<<" bytes");
    	}
    }

    void fillTree(TreePtr& tree, function<Value()> value_provider)
    {
    	fillTree(tree, tree->max_size(), value_provider);
    }

    void fillTree(TreePtr& tree, Int size, function<Value()> value_provider)
    {
    	for (int c = 0; c < size; c++)
    	{
    		tree->value(c) = value_provider();
    	}

    	tree->size() = size;

    	tree->reindex();
    }

    vector<Value> sumValues(TreePtr tree, Int start)
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

    Value sumValues(TreePtr tree, Int start, Int stop) const
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
