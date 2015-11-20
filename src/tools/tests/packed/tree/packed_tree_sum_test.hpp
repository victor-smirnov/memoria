
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PACKED_PACKED_TREE_SUM_HPP_
#define MEMORIA_TESTS_PACKED_PACKED_TREE_SUM_HPP_

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include "packed_tree_test_base.hpp"

namespace memoria {

using namespace std;

template <typename PackedTreeT>
class PackedTreeSumTest: public PackedTreeTestBase <PackedTreeT> {

    using MyType = PackedTreeSumTest<PackedTreeT>;
    using Base 	 = PackedTreeTestBase <PackedTreeT>;

    typedef typename Base::Tree                                                 Tree;
    typedef typename Base::Values                                               Values;
    typedef typename Tree::IndexValue                                           IndexValue;

    Int iterations_ = 1000;

public:


    PackedTreeSumTest(StringRef name): Base(name)
    {
        this->size_ = 8192;

        MEMORIA_ADD_TEST_PARAM(iterations_);

        MEMORIA_ADD_TEST(testBlockSum);
    }

    virtual ~PackedTreeSumTest() throw() {}

    IndexValue sum(const Tree* tree, Int block, Int start, Int end)
    {
        IndexValue sum = 0;

        for (Int c = start; c < end; c++)
        {
            sum += tree->value(block, c);
        }

        return sum;
    }





    void testBlockSum()
    {
        for (Int c = 1024; c <= this->size_; c += 1024)
        {
            testBlockSum(c);
        }
    }

    void testBlockSum(Int tree_size)
    {
        Base::out()<<tree_size<<endl;

        Tree* tree = Base::createEmptyTree();
        PARemover remover(tree);

        auto values = Base::fillRandom(tree, tree_size);

        Int size = tree->size();

        for (Int c = 0; c < iterations_; c++)
        {
            Int end     = this->getRandom(size / 2) + size / 2;
            Int start   = this->getRandom(size / 2);

            Int block   = this->getRandom(Tree::Blocks);

            Int sum1 = tree->sum(block, start, end);
            Int sum2 = Base::sum(values, block, start, end);
            Int sum3 = this->sum(tree, block, start, end);

            AssertEQ(MA_SRC, sum2, sum3);
            AssertEQ(MA_SRC, sum1, sum2);
        }
    }
};


}


#endif
