
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PALLOC_WAVELETTREE_HPP_
#define MEMORIA_TESTS_PALLOC_WAVELETTREE_HPP_

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/prototypes/btree/tools.hpp>

#include <memoria/core/packed2/packed_bitvector.hpp>

#include <memory>
#include <map>

namespace memoria {

using namespace std;

class PackedWaveletTreeTest: public TestTask {

	typedef TestTask														Base;
	typedef PackedWaveletTreeTest 											MyType;

	typedef PackedWaveletTreeTypes<> 										Types;

	typedef PackedWaveletTree<Types>										Tree;

	typedef shared_ptr<Tree>												TreePtr;

	typedef typename Tree::CardinalTree::LoudsTree							LoudsTree;

	typedef pair<UInt, Int> 												Pair;

public:

    void traverseTreePaths(const LoudsTree* tree, function<void (const PackedLoudsNode&, Int)> fn, Int level = 0)
    {
    	traverseTreePaths(tree, tree->root(), PackedLoudsNode(), fn, level);
    }

    void traverseTreePaths(
    		const LoudsTree* tree,
    		const PackedLoudsNode& node,
    		const PackedLoudsNode& parent,
    		function<void (const PackedLoudsNode&, Int)> fn, Int level = 0)
    {
    	if (tree->isLeaf(node))
    	{
    		fn(parent, level - 1);
    	}
    	else
    	{
    		PackedLoudsNode child = tree->first_child(node);
    		while (child != PackedLoudsNode() && !tree->isLeaf(child))
    		{
    			traverseTreePaths(tree, child, node, fn, level + 1);
    			child = tree->right_sibling(child);
    		}
    	}
    }


	PackedWaveletTreeTest(): TestTask("WaveletTree")
    {
		MEMORIA_ADD_TEST(testTree);
    }

    virtual ~PackedWaveletTreeTest() throw() {}

    TreePtr createSequence(Int block_size, Int free_space)
    {
    	free_space = PackedAllocator::roundDownBytesToAlignmentBlocks(free_space);

    	Int tree_block_size = Tree::block_size(block_size);

    	void* memory_block = malloc(tree_block_size + free_space);
    	memset(memory_block, 0, tree_block_size);

    	Tree* tree = T2T<Tree*>(memory_block);

    	tree->init(tree_block_size);

    	tree->forceResize(free_space);

    	return TreePtr(tree);
    }

    vector <UInt> createRandomAlphabet(Int size)
    {
    	vector <UInt> text(size);

    	for (auto& v: text)
    	{
    		v = getRandom();
    	}

    	return text;
    }

    vector <UInt> createRandomText(Int size, const vector<UInt>& alphabet)
    {
    	vector <UInt> text(size);

    	for (auto& v: text)
    	{
    		v = alphabet[getRandom(alphabet.size())];
    	}

    	return text;
    }

    vector<Pair> getRankedSymbols(const vector<UInt>& text)
    {
    	vector<Pair> result;

    	std::map<UInt, Int> ranks;

    	for (UInt c = 0; c < text.size(); c++)
    	{
    		UInt letter = text[c];

    		if (ranks.find(letter) == ranks.end())
    		{
    			ranks[letter] = 0;
    		}

    		ranks[letter]++;
    	}

    	for (auto pair: ranks)
    	{
    		result.push_back(pair);
    	}

    	std::sort(result.begin(), result.end(), [](const Pair& a, const Pair& b ) {
    		return a.second > b.second;
    	});

    	return result;
    }

    Int rank(const vector<UInt>& text, Int idx, UInt symbol)
    {
    	Int sum = 0;

    	for (Int c = 0; c <= idx; c++)
    	{
    		sum += text[c] == symbol;
    	}

    	return sum;
    }

    Int select(const vector<UInt>& text, Int rank, UInt symbol)
    {
    	for (UInt c = 0; c < text.size(); c++)
    	{
    		rank -= text[c] == symbol;

    		if (rank == 0)
    		{
    			return c;
    		}
    	}

    	return text.size();
    }




    void testTree()
    {
    	TreePtr tree_ptr = createSequence(64*1024, 40*1024);
    	Tree* tree = tree_ptr.get();
    	tree->prepare();

    	auto fn = [](const PackedLoudsNode& node, Int level) {
    		AssertEQ(MA_SRC, level, 3);
    	};

    	auto alphabet = createRandomAlphabet(10);

    	auto text = createRandomText(1000, alphabet);

    	for (UInt c = 0; c < text.size(); c++)
    	{
    		UBigInt path = text[c];
    		tree->insert(c, path);

    		traverseTreePaths(tree->ctree()->tree(), fn);
    	}

    	AssertEQ(MA_SRC, tree->size(), (Int)text.size());

    	for (UInt c = 0; c < text.size(); c++)
    	{
    		UInt value = tree->value(c);
    		AssertEQ(MA_SRC, value, text[c]);
    	}

    	auto ranks = getRankedSymbols(text);

    	for (auto& rnk: ranks)
    	{
    		UInt sym = rnk.first;

    		for (UInt c = 0; c < text.size(); c++)
    		{
    			Int rank1 = tree->rank(c, sym);
    			Int rank2 = rank(text, c, sym);

    			AssertEQ(MA_SRC, rank1, rank2);
    		}
    	}

    	for (auto& rnk: ranks)
    	{
    		UInt sym = rnk.first;
    		Int rank = rnk.second;

    		for (Int r = 1; r <= rank; r++)
    		{
    			Int idx1 = tree->select(r, sym);
    			Int idx2 = select(text, r, sym);

    			AssertEQ(MA_SRC, idx1, idx2);
    		}
    	}



//    	tree->dump(cout, false);

    }

};


}


#endif
