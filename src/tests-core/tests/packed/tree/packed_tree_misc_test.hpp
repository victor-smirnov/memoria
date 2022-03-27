
// Copyright 2013 Victor Smirnov
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

#include "packed_tree_test_base.hpp"

namespace memoria {
namespace tests {

template <
    typename PackedTreeT
>
class PackedTreeMiscTest: public PackedTreeTestBase<PackedTreeT> {

    using MyType = PackedTreeMiscTest<PackedTreeT>;
    using Base   = PackedTreeTestBase<PackedTreeT>;

    typedef typename Base::Tree                                                 Tree;
    typedef typename Base::Values                                               Values;

    static constexpr size_t Blocks = Base::Blocks;

public:

    using Base::createEmptyTree;
    using Base::fillVector;
    using Base::assertIndexCorrect;
    using Base::assertEqual;
    using Base::getRandom;
    using Base::createRandomValuesVector;
    using Base::get_so;
    using Base::assertEmpty;
    using Base::out;
    using Base::iterations_;
    using Base::size_;


    static void init_suite(TestSuite& suite)
    {
        MMA_CLASS_TESTS(suite, testInsertVector, testFillTree, testAddValue, testMerge);
        MMA_CLASS_TESTS(suite, testSplitToEmpty, testSplitToPreFilled, testRemoveMulti, testRemoveAll);
        MMA_CLASS_TESTS(suite, testClear);
    }



    void testInsertVector()
    {
        for (int c = 1; c < 100; c+=10)
        {
            testInsertVector(c * 100);
        }
    }

    void testInsertVector(size_t size)
    {
        out() << size << std::endl;

        auto tree_ss = createEmptyTree();
        auto tree = get_so(tree_ss);

        std::vector<Values> v = createRandomValuesVector(size);

        fillVector(tree, v);

        assertIndexCorrect(MA_SRC, tree);

        assertEqual(tree, v);
    }

    void testFillTree() {
        for (int c = 1; c < 128; c*=2) {
            testFillTree(c * 1024);
        }
    }

    void testFillTree(size_t tree_size)
    {
        Base::out() << tree_size << std::endl;

        auto tree_ss = Base::createEmptyTree();
        auto tree = get_so(tree_ss);

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

    void addValues(std::vector<Values>& values, size_t idx, const Values v)
    {
        for (size_t c = 0; c< Blocks; c++)
        {
            values[idx][c] += v[c];
        }
    }


    void testAddValue(size_t size)
    {
        // addValues() is currently not implemented
        // Uncomment the following code once it is.

        /*Base::out() << size << std::endl;

        auto tree = createEmptyTree();
        auto tree_values = createRandomValuesVector(size);

        fillVector(tree, tree_values);

        for (size_t c = 0; c < iterations_; c++)
        {
            Values value = Base::createRandom();
            size_t idx = getRandom(tree->size());

            tree->addValues(idx, value);
            Base::assertIndexCorrect(MA_SRC, tree);

            addValues(tree_values, idx, value);

            Base::assertEqual(tree, tree_values);
        }*/
    }


    void testSplitToEmpty()
    {
        for (int c = 64; c <= this->size_; c*=2)
        {
            testSplitToEmpty(c);
        }
    }

    void testSplitToEmpty(size_t size)
    {
        Base::out() << size << std::endl;

        auto tree1_ss = createEmptyTree();
        auto tree1 = get_so(tree1_ss);

        auto tree2_ss = createEmptyTree();
        auto tree2 = get_so(tree2_ss);

        auto tree_values1 = Base::createRandomValuesVector(size);

        fillVector(tree1, tree_values1);

        size_t idx = this->getRandom(size);

        tree1.split_to(tree2, idx);

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

    void testSplitPreFilled(size_t size)
    {
        Base::out() << size << std::endl;

        auto tree1_ss = createEmptyTree();
        auto tree1 = get_so(tree1_ss);

        auto tree2_ss = createEmptyTree();
        auto tree2 = get_so(tree2_ss);

        auto tree_values1 = createRandomValuesVector(size);
        auto tree_values2 = createRandomValuesVector(size < 100 ? size : 100);

        fillVector(tree1, tree_values1);
        fillVector(tree2, tree_values2);

        size_t idx = getRandom(size);

        tree1.split_to(tree2, idx);

        tree_values2.insert(tree_values2.begin(), tree_values1.begin() + idx, tree_values1.end());

        tree_values1.erase(tree_values1.begin() + idx, tree_values1.end());

        assertEqual(tree1, tree_values1);
        assertEqual(tree2, tree_values2);
    }


    void testRemoveMulti()
    {
        for (size_t size = 8; size <= this->size_; size*=2)
        {
            out() << size << std::endl;

            auto tree_ss = Base::createEmptyTree();
            auto tree = get_so(tree_ss);

            auto tree_values = createRandomValuesVector(size, 300);

            fillVector(tree, tree_values);

            assertEqual(tree, tree_values);

            for (size_t c = 0; c < this->iterations_; c++)
            {
                size_t start   = getRandom(tree.size());
                size_t end     = start + getRandom(tree.size() - start);

                size_t block_size = tree.data()->block_size();

                auto state = tree.make_update_state();
                tree.commit_remove(start, end, state.first);

                tree_values.erase(tree_values.begin() + start, tree_values.begin() + end);

                assertIndexCorrect(MA_SRC, tree);
                assertEqual(tree, tree_values);

                auto new_block_size = tree.data()->block_size();

                assert_le(new_block_size, block_size);
            }
        }
    }

    void testRemoveAll()
    {
        for (size_t size = 1; size <= this->size_; size*=2)
        {
            out() << size << std::endl;

            auto tree_ss = createEmptyTree();
            auto tree = get_so(tree_ss);

            auto tree_values = createRandomValuesVector(size);
            fillVector(tree, tree_values);

            assertEqual(tree, tree_values);

            auto state = tree.make_update_state();
            tree.commit_remove(0, tree.size(), state.first);

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

    void testMerge(size_t size)
    {
        Base::out() << size << std::endl;

        auto tree1_ss = Base::createEmptyTree();
        auto tree1 = get_so(tree1_ss);

        auto tree2_ss = Base::createEmptyTree();
        auto tree2 = get_so(tree2_ss);

        auto tree_values1 = Base::createRandomValuesVector(size);
        auto tree_values2 = Base::createRandomValuesVector(size);

        Base::fillVector(tree1, tree_values1);
        Base::fillVector(tree2, tree_values2);

        auto state = tree1.make_update_state();
        tree1.commit_merge_with(tree2, state.first);

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

    void testClear(size_t size)
    {
        out() << size << std::endl;

        auto tree_ss = createEmptyTree();
        auto tree = get_so(tree_ss);

        auto block_size = tree.data()->block_size();

        auto tree_values = createRandomValuesVector(size);
        fillVector(tree, tree_values);

        assertEqual(tree, tree_values);

        tree.clear();
        tree.data()->set_block_size(block_size);

        assertEmpty(tree);

        fillVector(tree, tree_values);

        assertEqual(tree, tree_values);

        tree.clear();
        assertEmpty(tree);
    }

};


}}
