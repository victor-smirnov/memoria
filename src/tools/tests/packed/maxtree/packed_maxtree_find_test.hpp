
// Copyright Victor Smirnov 2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/tools/tests.hpp>
#include <memoria/v1/tools/tools.hpp>

#include "../maxtree/packed_maxtree_test_base.hpp"

namespace memoria {
namespace v1 {

using namespace std;

template <typename PackedTreeT>
class PackedMaxTreeFindTest: public PackedMaxTreeTestBase<PackedTreeT> {

    using MyType = PackedMaxTreeFindTest<PackedTreeT>;

    using Base = PackedMaxTreeTestBase<PackedTreeT>;

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

    PackedMaxTreeFindTest(StringRef name): Base(name)
    {
        this->size_ = 16384;

        MEMORIA_ADD_TEST_PARAM(iterations_);

        MEMORIA_ADD_TEST(testFindForward);
    }

    virtual ~PackedMaxTreeFindTest() throw() {}

    template <typename Walker>
    auto find_fw(const TreePtr& tree, Int block, IndexValue limit)
    {
        Int end = tree->size();

        Walker walker(limit);

        for (Int c = 0; c < end; c++)
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

        for (Int c = 0; c < iterations_; c++)
        {
            Int end     = this->getRandom(size);

            Int block   = this->getRandom(Tree::Blocks);

            auto max     = tree->value(block, end);

            auto result1_lt = tree->findGTForward(block, max);
            auto result1_le = tree->findGEForward(block, max);

            auto result2_lt = find_fw<typename Tree::FindGTWalker>(tree, block, max);
            auto result2_le = find_fw<typename Tree::FindGEWalker>(tree, block, max);

            AssertEQ(MA_SRC, result1_lt.idx(), result2_lt.idx(), SBuf()<<"IDX "<<max<<" "<<block);
            AssertEQ(MA_SRC, result1_le.idx(), result2_le.idx(), SBuf()<<"IDX "<<" "<<max<<" "<<block);
        }
    }
};


}}