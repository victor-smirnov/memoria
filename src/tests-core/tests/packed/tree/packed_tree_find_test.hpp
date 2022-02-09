
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

template <typename PackedTreeT>
class PackedTreeFindTest: public PackedTreeTestBase<PackedTreeT> {

    using MyType = PackedTreeFindTest<PackedTreeT>;

    using Base = PackedTreeTestBase<PackedTreeT>;

    typedef typename Base::Tree                                                 Tree;
    typedef typename Base::Values                                               Values;
    typedef typename Base::Value                                                Value;
    typedef typename Tree::IndexValue                                           IndexValue;

    using typename Base::TreePtr;

    using Base::createEmptyTree;
    using Base::fillVector;
    using Base::fillRandom;
    using Base::assertIndexCorrect;
    using Base::assertEqual;
    using Base::getRandom;
    using Base::createRandomValuesVector;
    using Base::assertEmpty;
    using Base::out;
    using Base::size_;
    using Base::iterations_;

public:

    PackedTreeFindTest()
    {
        this->iterations_ = 1000;
        this->size_ = 4096;
    }

    static void init_suite(TestSuite& suite)
    {
        MMA_CLASS_TESTS(suite, testFindForward, testFindForwardFromStart, testFindBackward, testFindBackwardFromEnd);
    }



    template <typename Walker>
    auto find_fw(const TreePtr& tree, size_t block, size_t start, IndexValue limit)
    {
        size_t end = tree->size();

        Walker walker(limit);

        for (size_t c = start; c < end; c++)
        {
            Value value = tree->value(block, c);

            if (walker.compare(value))
            {
                return walker.idx(c);
            }
            else {
                walker.next();
            }
        }

        return walker.idx(end);
    }

    template <typename Walker>
    auto find_bw(const TreePtr& tree, size_t block, size_t start, IndexValue limit)
    {
        Walker walker(limit);

        for (size_t cc = start + 1; cc > 0; cc--)
        {
            size_t c = cc - 1;
            Value value = tree->value(block, c);

            if (walker.compare(value))
            {
                return walker.idx(c);
            }
            else {
                walker.next();
            }
        }

        return walker.idx(-1);
    }



    void testFindForward()
    {
        for (size_t c = 4; c < 1024; c *= 2)
        {
            testFindForward(c);
        }

        for (size_t c = 1024*3; c <= this->size_; c += 1024)
        {
            testFindForward(c);
        }
    }

    void testFindForward(size_t tree_size)
    {
        out() << tree_size << std::endl;

        auto tree = createEmptyTree();

        auto values = fillRandom(tree, tree_size);

        size_t size = tree->size();

        size_t block_t    = this->getRandom(Tree::Blocks);
        auto total_sum = tree->sum(block_t, 0, size);

        auto result_lt_t = tree->findGTForward(block_t, 0, total_sum);
        auto result_le_t = tree->findGEForward(block_t, 0, total_sum);

        size_t last = size;

        assert_equals(result_lt_t.local_pos(), last);
        assert_equals(result_lt_t.prefix(), total_sum);

        assert_equals(result_le_t.local_pos(), last - 1);
        assert_equals(result_le_t.prefix(), total_sum - tree->value(block_t, last - 1));

        result_lt_t = tree->findGTForward(block_t, 0, total_sum + 100);
        result_le_t = tree->findGEForward(block_t, 0, total_sum + 100);

        assert_equals(result_lt_t.local_pos(), size);
        assert_equals(result_lt_t.prefix(), total_sum);

        assert_equals(result_le_t.local_pos(), size);
        assert_equals(result_le_t.prefix(), total_sum);


        for (size_t c = 0; c < iterations_; c++)
        {
            size_t start   = this->getRandom(size - 2);
            size_t rnd     = this->getRandom(size - start - 2);
            size_t end     = start + rnd + 2;

            size_t block   = this->getRandom(Tree::Blocks);

            auto sum     = tree->sum(block, start, end);
            auto sum_v   = this->sum(values, block, start, end);

            assert_equals(sum, sum_v, "SUMS {} {}", start, end);

            if (sum == 0) continue;

            auto result1_lt = tree->findGTForward(block, start, sum);
            auto result1_le = tree->findGEForward(block, start, sum);

            auto result2_lt = find_fw<typename Tree::FindGTWalker>(tree, block, start, sum);
            auto result2_le = find_fw<typename Tree::FindGEWalker>(tree, block, start, sum);


            assert_equals(result1_lt.local_pos(), result2_lt.local_pos(), "IDX {} {} {}", start, sum, block);
            assert_equals(result1_lt.prefix(), result2_lt.prefix(), "SUM {} {} {}", start, sum, block);

            assert_equals(result1_le.local_pos(), result2_le.local_pos(), "IDX {} {} {}", start, sum, block);
            assert_equals(result1_le.prefix(), result2_le.prefix(), "SUM {} {} {}", start, sum, block);


            auto sum0 = tree->sum(block, start, size);

            result_lt_t = tree->findGTForward(block, start, sum0 + 100);
            result_le_t = tree->findGEForward(block, start, sum0 + 100);

            assert_equals(result_lt_t.local_pos(), size, "IDX {} {} {}", start, sum0, block);
            assert_equals(result_lt_t.prefix(), sum0, "IDX {} {} {}", start, sum0, block);

            assert_equals(result_le_t.local_pos(), size, "IDX {} {} {}", start, sum0, block);
            assert_equals(result_le_t.prefix(), sum0, "IDX {} {} {}", start, sum0, block);
        }
    }

    void testFindForwardFromStart()
    {
        for (size_t c = 1024; c <= this->size_; c += 1024)
        {
            testFindForwardFromStart(c);
        }
    }

    void testFindForwardFromStart(size_t tree_size)
    {
        out() << tree_size << std::endl;

        auto tree = createEmptyTree();
        auto values = fillRandom(tree, tree_size);

        size_t size = tree->size();

        for (size_t c = 0; c < iterations_; c++)
        {
            size_t rnd     = this->getRandom(size - 2);
            size_t end     = rnd + 2;

            size_t block   = this->getRandom(Tree::Blocks);

            auto sum        = tree->sum(block, 0, end);

            if (sum == 0) continue;

            auto result1_lt = tree->findGTForward(block, sum);
            auto result1_le = tree->findGEForward(block, sum);

            auto result2_lt = find_fw<typename Tree::FindGTWalker>(tree, block, 0, sum);
            auto result2_le = find_fw<typename Tree::FindGEWalker>(tree, block, 0, sum);

            assert_equals(result1_lt.local_pos(), result2_lt.local_pos(), "IDX {} {}", sum, block);
            assert_equals(result1_lt.prefix(), result2_lt.prefix(), "SUM {} {}", sum, block);

            assert_equals(result1_le.local_pos(), result2_le.local_pos(), "IDX {} {}", sum, block);
            assert_equals(result1_le.prefix(), result2_le.prefix(), "SUM {} {}", sum, block);
        }
    }



    void testFindBackward()
    {
        for (size_t c = 4; c < 1024; c *= 2)
        {
            testFindBackward(c);
        }

        for (size_t c = 1024; c <= this->size_; c += 1024)
        {
            testFindBackward(c);
        }
    }

    void testFindBackward(size_t tree_size)
    {
        out() << tree_size << std::endl;

        auto tree = createEmptyTree();

        auto values = fillRandom(tree, tree_size);

        size_t size        = tree->size();
        size_t block_t     = getRandom(Tree::Blocks);
        auto total_sum  = tree->sum(block_t, 0, size);

        auto result_lt_t = tree->findGTBackward(block_t, size - 1, total_sum);
        auto result_le_t = tree->findGEBackward(block_t, size - 1, total_sum);

        assert_equals(result_lt_t.local_pos(), size);
        assert_equals(result_lt_t.prefix(), total_sum);

        assert_equals(result_le_t.local_pos(), 0);
        assert_equals(result_le_t.prefix(), total_sum - tree->value(block_t, 0));

        result_lt_t = tree->findGTBackward(block_t, size - 1, total_sum + 100);
        result_le_t = tree->findGEBackward(block_t, size - 1, total_sum + 100);

        assert_equals(result_lt_t.local_pos(), size);
        assert_equals(result_lt_t.prefix(), total_sum);

        assert_equals(result_le_t.local_pos(), size);
        assert_equals(result_le_t.prefix(), total_sum);

        for (size_t c = 0; c < iterations_; c++)
        {
            size_t start   = getRandom(size - 2) + 2;
            size_t rnd     = getRandom(start - 2) + 1;
            size_t end     = start - rnd;
            size_t block   = getRandom(Tree::Blocks);

            assert_ge(end, 0);

            auto sum = tree->sum(block, end + 1, start + 1);

            // we do not handle zero sums correctly in this test yet
            if (sum == 0) continue;

            auto result1_lt = tree->findGTBackward(block, start, sum);
            auto result1_le = tree->findGEBackward(block, start, sum);

            auto result2_lt = find_bw<typename Tree::FindGTWalker>(tree, block, start, sum);
            auto result2_le = find_bw<typename Tree::FindGEWalker>(tree, block, start, sum);

            assert_equals(result1_lt.local_pos(), result2_lt.local_pos(), "IDX {} {} {}", start, sum, block);
            assert_equals(result1_lt.prefix(), result2_lt.prefix(), "SUM {} {} {}", start, sum, block);

            assert_equals(result1_le.local_pos(), result2_le.local_pos(), "IDX {} {} {}", start, sum, block);
            assert_equals(result1_le.prefix(), result2_le.prefix(), "SUM {} {} {}", start, sum, block);
        }
    }

    void testFindBackwardFromEnd()
    {
        for (size_t c = 1024; c <= this->size_; c += 1024)
        {
            testFindBackwardFromEnd(c);
        }
    }

    void testFindBackwardFromEnd(size_t tree_size)
    {
        out() << tree_size << std::endl;

        auto tree = createEmptyTree();
        auto values = fillRandom(tree, tree_size);

        size_t size = tree->size();

        for (size_t c = 0; c < iterations_; c++)
        {
            size_t start   = size - 1;
            size_t rnd     = getRandom(start - 2) + 1;
            size_t end     = start - rnd;
            size_t block   = getRandom(Tree::Blocks);

            assert_ge(end, 0);

            auto sum = tree->sum(block, end + 1, start + 1);

            // we do not handle zero sums correctly in this test yet
            if (sum == 0) continue;

            auto result1_lt = tree->findGTBackward(block, sum);
            auto result1_le = tree->findGEBackward(block, sum);

            auto result2_lt = find_bw<typename Tree::FindGTWalker>(tree, block, start, sum);
            auto result2_le = find_bw<typename Tree::FindGEWalker>(tree, block, start, sum);

            assert_equals(result1_lt.local_pos(), result2_lt.local_pos(), "IDX {} {} {}", start, sum, block);
            assert_equals(result1_lt.prefix(), result2_lt.prefix(), "SUM {} {} {}", start, sum, block);

            assert_equals(result1_le.local_pos(), result2_le.local_pos(), "IDX {} {} {}", start, sum, block);
            assert_equals(result1_le.prefix(), result2_le.prefix(), "SUM {} {} {}", start, sum, block);
        }
    }
};


}}
