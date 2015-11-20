
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PACKED_PACKED_TREE_FIND_HPP_
#define MEMORIA_TESTS_PACKED_PACKED_TREE_FIND_HPP_

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include "packed_tree_test_base.hpp"

namespace memoria {

using namespace std;

template <typename PackedTreeT>
class PackedTreeFindTest: public PackedTreeTestBase<PackedTreeT> {

    using MyType = PackedTreeFindTest<PackedTreeT>;

    using Base = PackedTreeTestBase<PackedTreeT>;

    typedef typename Base::Tree                                                 Tree;
    typedef typename Base::Values                                               Values;
    typedef typename Base::Value                                                Value;
    typedef typename Tree::IndexValue                                           IndexValue;

public:

    Int iterations_ = 1000;

    PackedTreeFindTest(StringRef name): Base(name)
    {
        this->size_ = 4096;

        MEMORIA_ADD_TEST_PARAM(iterations_);

        MEMORIA_ADD_TEST(testFindForward);
        MEMORIA_ADD_TEST(testFindBackward);
    }

    virtual ~PackedTreeFindTest() throw() {}


    template <typename Walker>
    Int find_fw(const Tree* tree, Int block, Int start, IndexValue limit)
    {
    	Int end = tree->size();

    	Walker walker(limit);

    	for (Int c = start; c < end; c++)
    	{
    		Value value = tree->value(block, c);

    		if (walker.compare(value))
    		{
    			return c;
    		}
    		else {
    			walker.next();
    		}
    	}

    	return end;
    }

    template <typename Walker>
    Int find_bw(const Tree* tree, Int block, Int start, IndexValue limit)
    {
    	Walker walker(limit);

    	for (Int c = start; c >= 0; c--)
    	{
    		Value value = tree->value(block, c);

    		if (walker.compare(value))
    		{
    			return c;
    		}
    		else {
    			walker.next();
    		}
    	}

    	return -1;
    }



    void testFindForward()
    {
        for (Int c = 1024; c <= this->size_; c += 1024)
        {
            testFindForward(c);
        }
    }

    void testFindForward(Int tree_size)
    {
        Base::out()<<tree_size<<endl;

        Tree* tree = Base::createEmptyTree();
        PARemover remover(tree);

        auto values = Base::fillRandom(tree, tree_size);

        Int size = tree->size();

        for (Int c = 0; c < iterations_; c++)
        {
            Int start   = this->getRandom(size - 2);
            Int rnd     = this->getRandom(size - start - 2);
            Int end     = start + rnd + 2;

            Int block   = this->getRandom(Tree::Blocks);

            Int sum     = tree->sum(block, start, end);

            if (sum == 0) continue;

            auto result1_lt = tree->findGTForward(block, start, sum).idx();
            auto result1_le = tree->findGEForward(block, start, sum).idx();

            auto result2_lt = find_fw<typename Tree::FindGTWalker>(tree, block, start, sum);
            auto result2_le = find_fw<typename Tree::FindGEWalker>(tree, block, start, sum);

            AssertEQ(MA_SRC, result1_lt, result2_lt, SBuf()<<start<<" "<<sum<<" "<<block);
            AssertEQ(MA_SRC, result1_le, result2_le, SBuf()<<start<<" "<<sum<<" "<<block);
        }
    }


    void testFindBackward()
    {
        for (Int c = 1024; c <= this->size_; c += 1024)
        {
            testFindBackward(c);
        }
    }

    void testFindBackward(Int tree_size)
    {
        Base::out()<<tree_size<<endl;

        Tree* tree = Base::createEmptyTree();
        PARemover remover(tree);

        auto values = Base::fillRandom(tree, tree_size);

        Int size = tree->size();

        for (Int c = 0; c < iterations_; c++)
        {
        	Int start   = this->getRandom(size - 2) + 2;
            Int rnd     = this->getRandom(start - 2) + 1;
            Int end     = start - rnd;
            Int block   = this->getRandom(Tree::Blocks);

            AssertGE(MA_SRC, end, 0);

            Int sum     = tree->sum(block, end + 1, start + 1);

            // we do not handle zero sums correctly in this test yet
            if (sum == 0) continue;

            auto result1_lt = tree->findGTBackward(block, start, sum).idx();
            auto result1_le = tree->findGEBackward(block, start, sum).idx();

            auto result2_lt = find_bw<typename Tree::FindGTWalker>(tree, block, start, sum);
            auto result2_le = find_bw<typename Tree::FindGEWalker>(tree, block, start, sum);

            AssertEQ(MA_SRC, result1_lt, result2_lt, SBuf()<<" - "<<start<<" "<<sum<<" "<<block);
            AssertEQ(MA_SRC, result1_le, result2_le, SBuf()<<" - "<<start<<" "<<sum<<" "<<block);
        }
    }
};


}


#endif
