
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PACKED_PACKED_TREE_MISC_HPP_
#define MEMORIA_TESTS_PACKED_PACKED_TREE_MISC_HPP_

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include "packed_tree_test_base.hpp"

namespace memoria {

using namespace std;

template <
	typename PackedTreeT
>
class PackedTreeMiscTest: public PackedTreeTestBase<PackedTreeT> {

    using MyType = PackedTreeMiscTest<PackedTreeT>;
    using Base 	 = PackedTreeTestBase<PackedTreeT>;

    typedef typename Base::Tree                                                 Tree;
    typedef typename Base::Values                                               Values;

    static constexpr Int Blocks = Base::Blocks;

    Int iterations_ = 10;

public:

    using Base::createEmptyTree;
    using Base::fillVector;
    using Base::assertIndexCorrect;
    using Base::assertEqual;
    using Base::getRandom;
    using Base::createRandomValuesVector;
    using Base::assertEmpty;
    using Base::out;


    PackedTreeMiscTest(StringRef name): Base(name)
    {
        this->size_ = 8192;

        MEMORIA_ADD_TEST_PARAM(iterations_);

        MEMORIA_ADD_TEST(testInsertVector);

        MEMORIA_ADD_TEST(testFillTree);

        MEMORIA_ADD_TEST(testAddValue);

        MEMORIA_ADD_TEST(testMerge);

        MEMORIA_ADD_TEST(testSplitToEmpty);
        MEMORIA_ADD_TEST(testSplitToPreFilled);

        MEMORIA_ADD_TEST(testRemoveMulti);
        MEMORIA_ADD_TEST(testRemoveAll);

        MEMORIA_ADD_TEST(testClear);
    }

    virtual ~PackedTreeMiscTest() throw() {}

    void testInsertVector()
    {
        for (int c = 1; c < 100; c+=10)
        {
            testInsertVector(c * 100);
        }
    }

    void testInsertVector(Int size)
    {
        out()<<size<<std::endl;

        auto tree = createEmptyTree();

        vector<Values> v = createRandomValuesVector(size);

        fillVector(tree, v);

        assertIndexCorrect(MA_SRC, tree);

        assertEqual(tree, v);
    }

    void testFillTree()
    {
        for (int c = 1; c < 128; c*=2)
        {
            testFillTree(c * 1024);
        }
    }

    void testFillTree(Int tree_size)
    {
        Base::out()<<tree_size<<std::endl;

        auto tree = Base::createEmptyTree();

        vector<Values> v = Base::fillRandom(tree, tree_size);

        Base::assertIndexCorrect(MA_SRC, tree);

        Base::assertEqual(tree, v);
    }

    void testAddValue()
    {
        for (int c = 1; c <= 64; c*=2)
        {
            testAddValue(c);
        }
    }

    void addValues(vector<Values>& values, Int idx, const Values v)
    {
        for (Int c = 0; c< Blocks; c++)
        {
            values[idx][c] += v[c];
        }
    }


    void testAddValue(Int size)
    {
        Base::out()<<size<<std::endl;

        auto tree = createEmptyTree();
        auto tree_values = createRandomValuesVector(size);

        fillVector(tree, tree_values);

        for (Int c = 0; c < iterations_; c++)
        {
            Values value = Base::createRandom();
            Int idx = getRandom(tree->size());

            tree->addValues(idx, value);
            Base::assertIndexCorrect(MA_SRC, tree);

            addValues(tree_values, idx, value);

            Base::assertEqual(tree, tree_values);
        }
    }


    void testSplitToEmpty()
    {
        for (int c = 64; c <= this->size_; c*=2)
        {
            testSplitToEmpty(c);
        }
    }

    void testSplitToEmpty(Int size)
    {
        Base::out()<<size<<std::endl;

        auto tree1 = createEmptyTree();
        auto tree2 = createEmptyTree();

        auto tree_values1 = Base::createRandomValuesVector(size);

        fillVector(tree1, tree_values1);

        Int idx = this->getRandom(size);

        tree1->splitTo(tree2.get(), idx);

        vector<Values> tree_values2(tree_values1.begin() + idx, tree_values1.end());

        tree_values1.erase(tree_values1.begin() + idx, tree_values1.end());

        assertEqual(tree1, tree_values1);
        assertEqual(tree2, tree_values2);
    }

    void testSplitToPreFilled()
    {
        for (int c = 2; c <= this->size_; c*=2)
        {
            testSplitPreFilled(c);
        }
    }

    void testSplitPreFilled(Int size)
    {
        Base::out()<<size<<std::endl;

        auto tree1 = createEmptyTree();
        auto tree2 = createEmptyTree();

        auto tree_values1 = createRandomValuesVector(size);
        auto tree_values2 = createRandomValuesVector(size < 100 ? size : 100);

        fillVector(tree1, tree_values1);
        fillVector(tree2, tree_values2);

        Int idx = getRandom(size);

        tree1->splitTo(tree2.get(), idx);

        tree_values2.insert(tree_values2.begin(), tree_values1.begin() + idx, tree_values1.end());

        tree_values1.erase(tree_values1.begin() + idx, tree_values1.end());

        assertEqual(tree1, tree_values1);
        assertEqual(tree2, tree_values2);
    }


    void testRemoveMulti()
    {
        for (Int size = 8; size <= this->size_; size*=2)
        {
            out()<<size<<std::endl;

            auto tree = Base::createEmptyTree();
            auto tree_values = createRandomValuesVector(size, 300);

            fillVector(tree, tree_values);

            assertEqual(tree, tree_values);

            for (Int c = 0; c < this->iterations_; c++)
            {
                Int start   = getRandom(tree->size());
                Int end		= start + getRandom(tree->size() - start);

                Int block_size = tree->block_size();

                tree->remove(start, end);

                tree_values.erase(tree_values.begin() + start, tree_values.begin() + end);

                assertIndexCorrect(MA_SRC, tree);
                assertEqual(tree, tree_values);

                auto new_block_size = tree->block_size();

                AssertLE(MA_SRC, new_block_size, block_size);
            }
        }
    }

    void testRemoveAll()
    {
        for (Int size = 1; size <= this->size_; size*=2)
        {
            out()<<size<<std::endl;

            auto tree = createEmptyTree();
            auto tree_values = createRandomValuesVector(size);
            fillVector(tree, tree_values);

            assertEqual(tree, tree_values);

            tree->remove(0, tree->size());

            assertEmpty(tree);
        }
    }



    void testMerge()
    {
        for (int c = 1; c <= this->size_; c*=2)
        {
            testMerge(c);
        }
    }

    void testMerge(Int size)
    {
        Base::out()<<size<<std::endl;

        auto tree1 = Base::createEmptyTree();

        auto tree2 = Base::createEmptyTree();

        auto tree_values1 = Base::createRandomValuesVector(size);
        auto tree_values2 = Base::createRandomValuesVector(size);

        Base::fillVector(tree1, tree_values1);
        Base::fillVector(tree2, tree_values2);

        tree1->mergeWith(tree2.get());

        tree_values2.insert(tree_values2.end(), tree_values1.begin(), tree_values1.end());

        assertEqual(tree2, tree_values2);
    }


    void testClear()
    {
    	testClear(0);

        for (int c = 1; c <= this->size_; c*=2)
        {
            testClear(c);
        }
    }

    void testClear(Int size)
    {
        out()<<size<<std::endl;

        auto tree = createEmptyTree();
        auto block_size = tree->block_size();

        auto tree_values = createRandomValuesVector(size);
        fillVector(tree, tree_values);

        assertEqual(tree, tree_values);

        tree->clear();
        tree->set_block_size(block_size);

        assertEmpty(tree);

        fillVector(tree, tree_values);

        assertEqual(tree, tree_values);

        tree->clear();
        assertEmpty(tree);
    }

};


}


#endif
