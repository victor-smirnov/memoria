
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_WT_TEST_HPP_
#define MEMORIA_TESTS_WT_TEST_HPP_

#include <memoria/tools/profile_tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memory>
#include <map>

namespace memoria {

using namespace std;

class WTTest: public SPTestTask {

	typedef SPTestTask														Base;
	typedef WTTest 															MyType;

	typedef typename SCtrTF<WT>::Type                                    	Ctr;
	typedef typename Ctr::Iterator                                          Iterator;


	typedef pair<UInt, Int> 												Pair;

	Int alphabet_size_ 	= 10;
	Int remove_check_ 	= 100;

public:

	WTTest(StringRef name): SPTestTask(name)
    {
		size_ = 1000;

		MEMORIA_ADD_TEST_PARAM(alphabet_size_);
		MEMORIA_ADD_TEST_PARAM(remove_check_);

		MEMORIA_ADD_TEST(testCreate);
		MEMORIA_ADD_TEST(testRemove);
    }

    virtual ~WTTest() throw() {}



    void testCreate()
    {
    	Allocator allocator;
    	Ctr ctr(&allocator);

    	allocator.commit();

    	try {
    		ctr.prepare();

    		auto alphabet = createRandomAlphabet(alphabet_size_);
    		auto text = createRandomText(this->size_, alphabet);

    		for (UInt c = 0; c < text.size(); c++)
    		{
    			out()<<c<<" "<<hex<<text[c]<<dec<<std::endl;

    			UBigInt value1 = text[c];

    			ctr.insert(c, value1);
    		}

    		assertText(ctr, text);

    		allocator.commit();

    		auto ranks = getRankedSymbols(text);

    		out()<<"Check ranks"<<std::endl;

    		for (auto& rnk: ranks)
    		{
    			UInt sym = rnk.first;

    			for (UInt c = 0; c < text.size(); c += 10)
    			{
    				Int rank1 = ctr.rank(c, sym);
    				Int rank2 = rank(text, c, sym);

    				AssertEQ(MA_SRC, rank1, rank2);
    			}
    		}

    		out()<<"Check selects"<<std::endl;

    		for (auto& rnk: ranks)
    		{
    			UInt sym = rnk.first;
    			Int rank = rnk.second;

    			for (Int r = 1; r <= rank; r++)
    			{
    				Int idx1 = ctr.select(r, sym);
    				Int idx2 = select(text, r, sym);

    				AssertEQ(MA_SRC, idx1, idx2);
    			}
    		}

    		StoreResource(allocator, "wtc");
    	}
    	catch (...) {
    		 Store(allocator);
    		 throw;
    	}
    }

    void testRemove()
    {
    	Allocator allocator;
    	Ctr ctr(&allocator);

    	allocator.commit();

    	try {
    		ctr.prepare();

    		auto alphabet = createRandomAlphabet(alphabet_size_);
    		auto text = createRandomText(this->size_, alphabet);

    		for (UInt c = 0; c < text.size(); c++)
    		{
    			out()<<c<<" "<<hex<<text[c]<<dec<<std::endl;

    			UBigInt value1 = text[c];

    			ctr.insert(c, value1);
    		}

    		assertText(ctr, text);

    		allocator.commit();

    		Int cnt = 0;
    		while (ctr.size() > 0)
    		{
    			Int idx = getRandom(ctr.size());

    			out()<<"Remove at "<<idx<<endl;

    			ctr.remove(idx);
    			text.erase(text.begin() + idx);

    			if ((cnt++) % remove_check_ == 0)
    			{
    				assertText(ctr, text);
    			}
    		}

    		allocator.commit();

    		StoreResource(allocator, "wtr");
    	}
    	catch (...) {
    		Store(allocator);
    		throw;
    	}
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

    void assertText(Ctr& tree, const vector<UInt>& text)
    {
    	AssertEQ(MA_SRC, tree.size(), (BigInt)text.size());

    	for (UInt c = 0; c < text.size(); c++)
    	{
    		UInt value = tree.value(c);

    		AssertEQ(MA_SRC, value, text[c], SBuf()<<c);
    	}
    }
};


}


#endif