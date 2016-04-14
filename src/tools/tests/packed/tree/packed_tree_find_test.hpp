
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

#include <memoria/v1/tools/tests.hpp>
#include <memoria/v1/tools/tools.hpp>

#include "packed_tree_test_base.hpp"

namespace memoria {
namespace v1 {

using namespace std;

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

public:

    Int iterations_ = 1000;

    PackedTreeFindTest(StringRef name): Base(name)
    {
        this->size_ = 4096;

        MEMORIA_ADD_TEST_PARAM(iterations_);

        MEMORIA_ADD_TEST(testFindForward);
        MEMORIA_ADD_TEST(testFindForwardFromStart);

        MEMORIA_ADD_TEST(testFindBackward);
        MEMORIA_ADD_TEST(testFindBackwardFromEnd);
    }

    virtual ~PackedTreeFindTest() noexcept {}



    template <typename Walker>
    auto find_fw(const TreePtr& tree, Int block, Int start, IndexValue limit)
    {
        Int end = tree->size();

        Walker walker(limit);

        for (Int c = start; c < end; c++)
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
    auto find_bw(const TreePtr& tree, Int block, Int start, IndexValue limit)
    {
        Walker walker(limit);

        for (Int c = start; c >= 0; c--)
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

        return walker.idx(-1);
    }



    void testFindForward()
    {
        for (Int c = 4; c < 1024; c *= 2)
        {
            testFindForward(c);
        }

        for (Int c = 1024*3; c <= this->size_; c += 1024)
        {
            testFindForward(c);
        }
    }

    void testFindForward(Int tree_size)
    {
        out()<<tree_size<<endl;

        auto tree = createEmptyTree();

        auto values = fillRandom(tree, tree_size);

        Int size = tree->size();

        Int block_t    = this->getRandom(Tree::Blocks);
        auto total_sum = tree->sum(block_t, 0, size);

        auto result_lt_t = tree->findGTForward(block_t, 0, total_sum);
        auto result_le_t = tree->findGEForward(block_t, 0, total_sum);

        Int last = size;

        AssertEQ(MA_SRC, result_lt_t.idx(), last);
        AssertEQ(MA_SRC, result_lt_t.prefix(), total_sum);

        AssertEQ(MA_SRC, result_le_t.idx(), last - 1);
        AssertEQ(MA_SRC, result_le_t.prefix(), total_sum - tree->value(block_t, last - 1));

        result_lt_t = tree->findGTForward(block_t, 0, total_sum + 100);
        result_le_t = tree->findGEForward(block_t, 0, total_sum + 100);

        AssertEQ(MA_SRC, result_lt_t.idx(), size);
        AssertEQ(MA_SRC, result_lt_t.prefix(), total_sum);

        AssertEQ(MA_SRC, result_le_t.idx(), size);
        AssertEQ(MA_SRC, result_le_t.prefix(), total_sum);


        for (Int c = 0; c < iterations_; c++)
        {
            Int start   = this->getRandom(size - 2);
            Int rnd     = this->getRandom(size - start - 2);
            Int end     = start + rnd + 2;

            Int block   = this->getRandom(Tree::Blocks);

            auto sum     = tree->sum(block, start, end);
            auto sum_v   = this->sum(values, block, start, end);

            AssertEQ(MA_SRC, sum, sum_v, SBuf()<<"SUMS "<<start<<" "<<end);

            if (sum == 0) continue;

            auto result1_lt = tree->findGTForward(block, start, sum);
            auto result1_le = tree->findGEForward(block, start, sum);

            auto result2_lt = find_fw<typename Tree::FindGTWalker>(tree, block, start, sum);
            auto result2_le = find_fw<typename Tree::FindGEWalker>(tree, block, start, sum);


            AssertEQ(MA_SRC, result1_lt.idx(), result2_lt.idx(), SBuf()<<"IDX "<<start<<" "<<sum<<" "<<block);
            AssertEQ(MA_SRC, result1_lt.prefix(), result2_lt.prefix(), SBuf()<<"SUM "<<start<<" "<<sum<<" "<<block);

            AssertEQ(MA_SRC, result1_le.idx(), result2_le.idx(), SBuf()<<"IDX "<<start<<" "<<sum<<" "<<block);
            AssertEQ(MA_SRC, result1_le.prefix(), result2_le.prefix(), SBuf()<<"SUM "<<start<<" "<<sum<<" "<<block);


            auto sum0 = tree->sum(block, start, size);

            result_lt_t = tree->findGTForward(block, start, sum0 + 100);
            result_le_t = tree->findGEForward(block, start, sum0 + 100);

            AssertEQ(MA_SRC, result_lt_t.idx(), size, SBuf()<<"IDX "<<start<<" "<<sum0<<" "<<block);
            AssertEQ(MA_SRC, result_lt_t.prefix(), sum0, SBuf()<<"IDX "<<start<<" "<<sum0<<" "<<block);

            AssertEQ(MA_SRC, result_le_t.idx(), size, SBuf()<<"IDX "<<start<<" "<<sum0<<" "<<block);
            AssertEQ(MA_SRC, result_le_t.prefix(), sum0, SBuf()<<"IDX "<<start<<" "<<sum0<<" "<<block);
        }
    }

    void testFindForwardFromStart()
    {
        for (Int c = 1024; c <= this->size_; c += 1024)
        {
            testFindForwardFromStart(c);
        }
    }

    void testFindForwardFromStart(Int tree_size)
    {
        out()<<tree_size<<endl;

        auto tree = createEmptyTree();
        auto values = fillRandom(tree, tree_size);

        Int size = tree->size();

        for (Int c = 0; c < iterations_; c++)
        {
            Int rnd     = this->getRandom(size - 2);
            Int end     = rnd + 2;

            Int block   = this->getRandom(Tree::Blocks);

            Int sum     = tree->sum(block, 0, end);

            if (sum == 0) continue;

            auto result1_lt = tree->findGTForward(block, sum);
            auto result1_le = tree->findGEForward(block, sum);

            auto result2_lt = find_fw<typename Tree::FindGTWalker>(tree, block, 0, sum);
            auto result2_le = find_fw<typename Tree::FindGEWalker>(tree, block, 0, sum);

            AssertEQ(MA_SRC, result1_lt.idx(), result2_lt.idx(), SBuf()<<"IDX "<<sum<<" "<<block);
            AssertEQ(MA_SRC, result1_lt.prefix(), result2_lt.prefix(), SBuf()<<"SUM "<<" "<<sum<<" "<<block);

            AssertEQ(MA_SRC, result1_le.idx(), result2_le.idx(), SBuf()<<"IDX "<<" "<<sum<<" "<<block);
            AssertEQ(MA_SRC, result1_le.prefix(), result2_le.prefix(), SBuf()<<"SUM "<<" "<<sum<<" "<<block);
        }
    }



    void testFindBackward()
    {
        for (Int c = 4; c < 1024; c *= 2)
        {
            testFindBackward(c);
        }

        for (Int c = 1024; c <= this->size_; c += 1024)
        {
            testFindBackward(c);
        }
    }

    void testFindBackward(Int tree_size)
    {
        out()<<tree_size<<endl;

        auto tree = createEmptyTree();

        auto values = fillRandom(tree, tree_size);

        Int size        = tree->size();
        Int block_t     = getRandom(Tree::Blocks);
        auto total_sum  = tree->sum(block_t, 0, size);

        auto result_lt_t = tree->findGTBackward(block_t, size - 1, total_sum);
        auto result_le_t = tree->findGEBackward(block_t, size - 1, total_sum);

        AssertEQ(MA_SRC, result_lt_t.idx(), -1);
        AssertEQ(MA_SRC, result_lt_t.prefix(), total_sum);

        AssertEQ(MA_SRC, result_le_t.idx(), 0);
        AssertEQ(MA_SRC, result_le_t.prefix(), total_sum - tree->value(block_t, 0));

        result_lt_t = tree->findGTBackward(block_t, size - 1, total_sum + 100);
        result_le_t = tree->findGEBackward(block_t, size - 1, total_sum + 100);

        AssertEQ(MA_SRC, result_lt_t.idx(), -1);
        AssertEQ(MA_SRC, result_lt_t.prefix(), total_sum);

        AssertEQ(MA_SRC, result_le_t.idx(), -1);
        AssertEQ(MA_SRC, result_le_t.prefix(), total_sum);

        for (Int c = 0; c < iterations_; c++)
        {
            Int start   = getRandom(size - 2) + 2;
            Int rnd     = getRandom(start - 2) + 1;
            Int end     = start - rnd;
            Int block   = getRandom(Tree::Blocks);

            AssertGE(MA_SRC, end, 0);

            Int sum     = tree->sum(block, end + 1, start + 1);

            // we do not handle zero sums correctly in this test yet
            if (sum == 0) continue;

            auto result1_lt = tree->findGTBackward(block, start, sum);
            auto result1_le = tree->findGEBackward(block, start, sum);

            auto result2_lt = find_bw<typename Tree::FindGTWalker>(tree, block, start, sum);
            auto result2_le = find_bw<typename Tree::FindGEWalker>(tree, block, start, sum);

            AssertEQ(MA_SRC, result1_lt.idx(), result2_lt.idx(), SBuf()<<"IDX "<<start<<" "<<sum<<" "<<block);
            AssertEQ(MA_SRC, result1_lt.prefix(), result2_lt.prefix(), SBuf()<<"SUM "<<start<<" "<<sum<<" "<<block);

            AssertEQ(MA_SRC, result1_le.idx(), result2_le.idx(), SBuf()<<"IDX "<<start<<" "<<sum<<" "<<block);
            AssertEQ(MA_SRC, result1_le.prefix(), result2_le.prefix(), SBuf()<<"SUM "<<start<<" "<<sum<<" "<<block);
        }
    }

    void testFindBackwardFromEnd()
    {
        for (Int c = 1024; c <= this->size_; c += 1024)
        {
            testFindBackwardFromEnd(c);
        }
    }

    void testFindBackwardFromEnd(Int tree_size)
    {
        out()<<tree_size<<endl;

        auto tree = createEmptyTree();
        auto values = fillRandom(tree, tree_size);

        Int size = tree->size();

        for (Int c = 0; c < iterations_; c++)
        {
            Int start   = size - 1;
            Int rnd     = getRandom(start - 2) + 1;
            Int end     = start - rnd;
            Int block   = getRandom(Tree::Blocks);

            AssertGE(MA_SRC, end, 0);

            Int sum     = tree->sum(block, end + 1, start + 1);

            // we do not handle zero sums correctly in this test yet
            if (sum == 0) continue;

            auto result1_lt = tree->findGTBackward(block, sum);
            auto result1_le = tree->findGEBackward(block, sum);

            auto result2_lt = find_bw<typename Tree::FindGTWalker>(tree, block, start, sum);
            auto result2_le = find_bw<typename Tree::FindGEWalker>(tree, block, start, sum);

            AssertEQ(MA_SRC, result1_lt.idx(), result2_lt.idx(), SBuf()<<"IDX "<<start<<" "<<sum<<" "<<block);
            AssertEQ(MA_SRC, result1_lt.prefix(), result2_lt.prefix(), SBuf()<<"SUM "<<start<<" "<<sum<<" "<<block);

            AssertEQ(MA_SRC, result1_le.idx(), result2_le.idx(), SBuf()<<"IDX "<<start<<" "<<sum<<" "<<block);
            AssertEQ(MA_SRC, result1_le.prefix(), result2_le.prefix(), SBuf()<<"SUM "<<start<<" "<<sum<<" "<<block);
        }
    }
};


}}