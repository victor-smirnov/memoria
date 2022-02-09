
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
class PackedTreeSumTest: public PackedTreeTestBase <PackedTreeT> {

    using MyType = PackedTreeSumTest<PackedTreeT>;
    using Base   = PackedTreeTestBase <PackedTreeT>;

    typedef typename Base::Tree                                                 Tree;
    typedef typename Base::Values                                               Values;
    typedef typename Tree::IndexValue                                           IndexValue;

    using typename Base::TreePtr;

public:

    using Base::iterations_;
    using Base::size_;
    using Base::out;


    static void init_suite(TestSuite& suite)
    {
        MMA_CLASS_TESTS(suite, testBlockSum);
    }

    PackedTreeSumTest()
    {
        this->iterations_ = 1000;
        this->size_ = 8192;
    }



    IndexValue sum(const TreePtr& tree, size_t block, size_t start, size_t end)
    {
        IndexValue sum = 0;

        for (size_t c = start; c < end; c++)
        {
            sum += tree->value(block, c);
        }

        return sum;
    }

    void testBlockSum()
    {
        for (size_t c = 1024; c <= size_; c += 1024)
        {
            testBlockSum(c);
        }
    }

    void testBlockSum(size_t tree_size)
    {
        out() << tree_size << std::endl;

        auto tree = Base::createEmptyTree();

        auto values = Base::fillRandom(tree, tree_size);

        size_t size = tree->size();

        for (size_t c = 0; c < iterations_; c++)
        {
            size_t end     = this->getRandom(size / 2) + size / 2;
            size_t start   = this->getRandom(size / 2);

            size_t block   = this->getRandom(Tree::Blocks);

            auto sum1 = tree->sum(block, start, end);
            auto sum2 = Base::sum(values, block, start, end);
            auto sum3 = this->sum(tree, block, start, end);

            assert_equals(sum2, sum3);
            assert_equals(sum1, sum2);
        }
    }
};


}}
