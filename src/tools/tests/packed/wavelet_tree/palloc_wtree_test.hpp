
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PALLOC_WAVELETTREE_HPP_
#define MEMORIA_TESTS_PALLOC_WAVELETTREE_HPP_

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/core/packed/wavelet_tree/packed_wavelet_tree.hpp>
#include <memoria/core/packed/sseq/packed_multisequence.hpp>

#include <memory>
#include <map>

namespace memoria {

using namespace std;

class PackedWaveletTreeTest: public TestTask {

	typedef TestTask														Base;
	typedef PackedWaveletTreeTest 											MyType;

	typedef PackedWaveletTreeTypes<> 										Types;

	typedef PackedWaveletTree<Types>										Tree;

	typedef typename Tree::CardinalTree::LoudsTree							LoudsTree;

	typedef pair<UInt, Int> 												Pair;

	Int alphabet_size_ = 10;

public:

	PackedWaveletTreeTest(): TestTask("WaveletTree")
    {
		size_ = 1000;

		MEMORIA_ADD_TEST_PARAM(alphabet_size_);

		MEMORIA_ADD_TEST(testCreateTree);
		MEMORIA_ADD_TEST(testRemoveTree);

		MEMORIA_ADD_TEST(testDump);
    }

    virtual ~PackedWaveletTreeTest() throw() {}

    void testDump()
    {
    	Tree* tree = createTree();
    	PARemover remover(tree);

    	tree->prepare();

    	tree->insert(0, 0x76053612);
    	tree->insert(1, 0x5e3a3092);
//    	tree->insert(2, 0x7b4f865a);
//    	tree->insert(3, 0x372507b5);

    	cout<<hex<<tree->value(0)<<dec<<endl;

    	tree->dump();
    }


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



    Tree* createTree(Int block_size = 128*1024)
    {
    	PackedAllocator* alloc = T2T<PackedAllocator*>(malloc(block_size));
    	alloc->init(block_size, 1);
    	alloc->setTopLevelAllocator();

    	Tree* tree = alloc->allocateEmpty<Tree>(0);

    	return tree;
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

    void assertText(const Tree* tree, const vector<UInt>& text)
    {
    	AssertEQ(MA_SRC, tree->size(), (Int)text.size());

    	for (UInt c = 0; c < text.size(); c++)
    	{
    		UInt value = tree->value(c);

    		AssertEQ(MA_SRC, value, text[c], SBuf()<<c);
    	}
    }



    void testCreateTree()
    {
    	Tree* tree = createTree();
    	PARemover remover(tree);

    	tree->prepare();

    	auto fn = [](const PackedLoudsNode& node, Int level) {
    		AssertEQ(MA_SRC, level, 3);
    	};

    	auto alphabet = createRandomAlphabet(alphabet_size_);

    	auto text = createRandomText(size_, alphabet);

    	for (UInt c = 0; c < text.size(); c++)
    	{
    		UBigInt path = text[c];

    		out()<<"Insert at "<<c<<": "<<hex<<path<<dec<<std::endl;
    		tree->insert(c, path);

    		traverseTreePaths(tree->ctree()->tree(), fn);
    	}

    	assertText(tree, text);

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

    	tree->dump(this->out());
    }

    void testRemoveTree()
    {
    	Tree* tree = createTree();
    	PARemover remover(tree);

    	tree->prepare();

    	out()<<"Process new tree"<<std::endl;
    	testRemoveTree(tree);

    	out()<<"Process used empty tree"<<std::endl;
    	testRemoveTree(tree);
    }

    void testRemoveTree(Tree* tree)
    {
    	auto fn = [](const PackedLoudsNode& node, Int level) {
    		AssertEQ(MA_SRC, level, 3);
    	};

    	auto alphabet = createRandomAlphabet(alphabet_size_);

    	auto text = createRandomText(size_, alphabet);

    	for (auto t: text) {
    		out()<<hex<<t<<endl;
    	}

    	for (UInt c = 0; c < text.size(); c++)
    	{
    		UBigInt path = text[c];
    		tree->insert(c, path);
    	}

    	traverseTreePaths(tree->ctree()->tree(), fn);

    	tree->dump(out());

    	while (tree->size() > 0)
    	{
    		Int idx = getRandom(tree->size());

    		out()<<"Remove: "<<idx<<" "<<hex<<tree->value(idx)<<dec<<std::endl;

    		tree->remove(idx);

    		text.erase(text.begin() + idx);

    		assertText(tree, text);
    	}

    	tree->dump(out());
    }
};


}


#endif
