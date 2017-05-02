
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
class PackedTreeSumTest: public PackedTreeTestBase <PackedTreeT> {

    using MyType = PackedTreeSumTest<PackedTreeT>;
    using Base   = PackedTreeTestBase <PackedTreeT>;

    typedef typename Base::Tree                                                 Tree;
    typedef typename Base::Values                                               Values;
    typedef typename Tree::IndexValue                                           IndexValue;

    using typename Base::TreePtr;

    int32_t iterations_ = 1000;

public:


    PackedTreeSumTest(StringRef name): Base(name)
    {
        this->size_ = 8192;

        MEMORIA_ADD_TEST_PARAM(iterations_);

        MEMORIA_ADD_TEST(testBlockSum);
    }

    virtual ~PackedTreeSumTest() noexcept {}

    IndexValue sum(const TreePtr& tree, int32_t block, int32_t start, int32_t end)
    {
        IndexValue sum = 0;

        for (int32_t c = start; c < end; c++)
        {
            sum += tree->value(block, c);
        }

        return sum;
    }





    void testBlockSum()
    {
        for (int32_t c = 1024; c <= this->size_; c += 1024)
        {
            testBlockSum(c);
        }
    }

    void testBlockSum(int32_t tree_size)
    {
        Base::out()<<tree_size<<endl;

        auto tree = Base::createEmptyTree();

        auto values = Base::fillRandom(tree, tree_size);

        int32_t size = tree->size();

        for (int32_t c = 0; c < iterations_; c++)
        {
            int32_t end     = this->getRandom(size / 2) + size / 2;
            int32_t start   = this->getRandom(size / 2);

            int32_t block   = this->getRandom(Tree::Blocks);

            int32_t sum1 = tree->sum(block, start, end);
            int32_t sum2 = Base::sum(values, block, start, end);
            int32_t sum3 = this->sum(tree, block, start, end);

            AssertEQ(MA_SRC, sum2, sum3);
            AssertEQ(MA_SRC, sum1, sum2);
        }
    }
};


}}