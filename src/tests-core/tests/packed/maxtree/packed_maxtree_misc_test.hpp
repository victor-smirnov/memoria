
// Copyright 2016 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#pragma once

#include "packed_maxtree_test_base.hpp"

namespace memoria {
namespace tests {

template <
    typename PackedTreeT
>
class PackedMaxTreeMiscTest: public PackedMaxTreeTestBase<PackedTreeT> {

    using MyType = PackedMaxTreeMiscTest<PackedTreeT>;
    using Base   = PackedMaxTreeTestBase<PackedTreeT>;

    typedef typename Base::Tree                                                 Tree;
    typedef typename Base::Values                                               Values;

    static constexpr int32_t Blocks = Base::Blocks;

    int32_t iterations_ = 10;

public:

    using Base::createEmptyTree;
    using Base::fillVector;
    using Base::assertIndexCorrect;
    using Base::assertEqual;
    using Base::getRandom;
    using Base::createRandomValuesVector;
    using Base::assertEmpty;
    using Base::out;

    PackedMaxTreeMiscTest() {
        this->size_ = 8192;
    }

    MMA_STATE_FILEDS(iterations_);

    static void init_suite(TestSuite& suite)
    {
        MMA_CLASS_TESTS(suite, testInsertVector, testFillTree, testMerge);
        MMA_CLASS_TESTS(suite, testSplitToEmpty, testSplitToPreFilled, testRemoveMulti);
        MMA_CLASS_TESTS(suite, testRemoveAll, testClear);
    }



    void testInsertVector()
    {
        for (int c = 1; c < 100; c+=10)
        {
            testInsertVector(c * 100);
        }
    }

    void testInsertVector(int32_t size)
    {
        out() << size << std::endl;

        auto tree = createEmptyTree();

        std::vector<Values> v = createRandomValuesVector(size);

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

    void testFillTree(int32_t tree_size)
    {
        Base::out() << tree_size << std::endl;

        auto tree = Base::createEmptyTree();

        std::vector<Values> v = Base::fillRandom(tree, tree_size);

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

    void addValues(std::vector<Values>& values, int32_t idx, const Values v)
    {
        for (int32_t c = 0; c< Blocks; c++)
        {
            values[idx][c] += v[c];
        }
    }


    void testAddValue(int32_t size)
    {
        Base::out() << size << std::endl;

        auto tree = createEmptyTree();
        auto tree_values = createRandomValuesVector(size);

        fillVector(tree, tree_values);

        for (int32_t c = 0; c < iterations_; c++)
        {
            Values value = Base::createRandom();
            int32_t idx = getRandom(tree->size());

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

    void testSplitToEmpty(int32_t size)
    {
        Base::out() << size << std::endl;

        auto tree1 = createEmptyTree();
        auto tree2 = createEmptyTree();

        auto tree_values1 = Base::createRandomValuesVector(size);

        fillVector(tree1, tree_values1);

        int32_t idx = this->getRandom(size);

        OOM_THROW_IF_FAILED(tree1->splitTo(tree2.get(), idx), MMA_SRC);

        std::vector<Values> tree_values2(tree_values1.begin() + idx, tree_values1.end());

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

    void testSplitPreFilled(int32_t size)
    {
        Base::out() << size << std::endl;

        auto tree1 = createEmptyTree();
        auto tree2 = createEmptyTree();

        auto tree_values1 = createRandomValuesVector(size);
        auto tree_values2 = createRandomValuesVector(size < 100 ? size : 100);

        fillVector(tree1, tree_values1);
        fillVector(tree2, tree_values2);

        int32_t idx = getRandom(size);

        OOM_THROW_IF_FAILED(tree1->splitTo(tree2.get(), idx), MMA_SRC);

        tree_values2.insert(tree_values2.begin(), tree_values1.begin() + idx, tree_values1.end());

        tree_values1.erase(tree_values1.begin() + idx, tree_values1.end());

        assertEqual(tree1, tree_values1);
        assertEqual(tree2, tree_values2);
    }


    void testRemoveMulti()
    {
        for (int32_t size = 8; size <= this->size_; size*=2)
        {
            out() << size << std::endl;

            auto tree = Base::createEmptyTree();
            auto tree_values = createRandomValuesVector(size, 300);

            fillVector(tree, tree_values);

            assertEqual(tree, tree_values);

            for (int32_t c = 0; c < this->iterations_; c++)
            {
                int32_t start   = getRandom(tree->size());
                int32_t end     = start + getRandom(tree->size() - start);

                int32_t block_size = tree->block_size();

                OOM_THROW_IF_FAILED(tree->remove(start, end), MMA_SRC);

                tree_values.erase(tree_values.begin() + start, tree_values.begin() + end);

                assertIndexCorrect(MA_SRC, tree);
                assertEqual(tree, tree_values);

                auto new_block_size = tree->block_size();

                assert_le(new_block_size, block_size);
            }
        }
    }

    void testRemoveAll()
    {
        for (int32_t size = 1; size <= this->size_; size*=2)
        {
            out() << size << std::endl;

            auto tree = createEmptyTree();
            auto tree_values = createRandomValuesVector(size);
            fillVector(tree, tree_values);

            assertEqual(tree, tree_values);

            OOM_THROW_IF_FAILED(tree->remove(0, tree->size()), MMA_SRC);

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

    void testMerge(int32_t size)
    {
        Base::out() << size << std::endl;

        auto tree1 = Base::createEmptyTree();

        auto tree2 = Base::createEmptyTree();

        auto tree_values1 = Base::createRandomValuesVector(size);
        auto tree_values2 = Base::createRandomValuesVector(size);

        Base::fillVector(tree1, tree_values1);
        Base::fillVector(tree2, tree_values2);

        OOM_THROW_IF_FAILED(tree1->mergeWith(tree2.get()), MMA_SRC);

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

    void testClear(int32_t size)
    {
        out() << size << std::endl;

        auto tree = createEmptyTree();
        auto block_size = tree->block_size();

        auto tree_values = createRandomValuesVector(size);
        fillVector(tree, tree_values);

        assertEqual(tree, tree_values);

        OOM_THROW_IF_FAILED(tree->clear(), MMA_SRC);
        tree->set_block_size(block_size);

        assertEmpty(tree);

        fillVector(tree, tree_values);

        assertEqual(tree, tree_values);

        OOM_THROW_IF_FAILED(tree->clear(), MMA_SRC);
        assertEmpty(tree);
    }

};


}}
