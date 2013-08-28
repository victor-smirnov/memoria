
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PACKED_PACKED_TREE_VLEMISC_HPP_
#define MEMORIA_TESTS_PACKED_PACKED_TREE_VLEMISC_HPP_

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include "packed_tree_test_base.hpp"

namespace memoria {

using namespace std;

template <
    template <typename> class CodecType = ValueFSECodec,
    Int Blocks      = 1,
    Int VPB         = PackedTreeBranchingFactor,
    Int BF          = PackedTreeBranchingFactor
>
class PackedTreeVLEMiscTest: public PackedTreeTestBase <
    PkdVTree,
    CodecType,
    Blocks,
    VPB,
    BF
> {

    typedef PackedTreeVLEMiscTest<
            CodecType,
            Blocks,
            VPB,
            BF
    >                                                                           MyType;

    typedef PackedTreeTestBase <
        PkdVTree,
        CodecType,
        Blocks,
        VPB,
        BF
    >                                                                           Base;

    typedef typename Base::Tree                                                 Tree;
    typedef typename Base::Values                                               Values;

    Int iterations_ = 1000;

public:


    PackedTreeVLEMiscTest(StringRef name): Base(name)
    {
        this->size_ = 8192;

        MEMORIA_ADD_TEST_PARAM(iterations_);

        MEMORIA_ADD_TEST(testIndexLayoutSize);
        MEMORIA_ADD_TEST(testIndexLayout);

        MEMORIA_ADD_TEST(testGetValueOffset);
    }

    virtual ~PackedTreeVLEMiscTest() throw() {}

    void testIndexLayoutSize()
    {
        vector<Int> sizes        = {258, 511, 1025, 4894, 8192, 512578, 398123478};
        vector<Int> layout_sizes = {3, 3, 4, 4, 4, 5, 7};

        for (Int c = 0; c < sizes.size(); c++)
        {
            Int layout_size = Tree::TreeTools::compute_layout_size(sizes[c]);
            AssertEQ(MA_SRC, layout_sizes[c], layout_size);
        }
    }

    void testIndexLayout()
    {
        vector<Int> sizes        = {258, 511, 1025, 4894, 8192, 512578, 398123478};

        vector<vector<Int>> layouts = {
                {2, 1, 9},
                {2, 1, 16},
                {3, 1, 2, 33},
                {3, 1, 5, 153},
                {3, 1, 8, 256},
                {4, 1, 16, 501, 16019},
                {6, 1, 12, 380, 12150, 388793, 12441359}
        };

        for (Int c = 0; c < sizes.size(); c++)
        {
            Int layout_size = Tree::TreeTools::compute_layout_size(sizes[c]);
            vector<Int> layout(layout_size);

            Tree::TreeTools::buildIndexTreeLayout(layout.data(), sizes[c], layout_size);

            AssertEQ(MA_SRC, layout, layouts[c]);
        }
    }


    Int value_offset(const Tree* tree, Int idx)
    {
        typename Tree::Codec codec;

        Int pos     = 0;
        Int limit   = tree->data_size();

        auto values = tree->values();

        for (Int c = 0; c < idx; c++)
        {
            pos += codec.length(values, pos, limit);
        }

        return pos;
    }

    void testGetValueOffset()
    {
        for (Int c = 256; c < this->size_; c *= 2)
        {
            testGetValueOffset(c);
        }
    }

    void testGetValueOffset(Int block_size)
    {
        Base::out()<<block_size<<endl;

        Tree* tree = Base::createEmptyTree(block_size);
        PARemover remover(tree);

        Int offset0 = tree->value_offset(0);

        AssertEQ(MA_SRC, offset0, 0);

        Int offset1 = tree->value_offset(tree->raw_size());
        AssertEQ(MA_SRC, offset1, tree->data_size());

        for (Int c = 0; c < iterations_; c++)
        {
            Int idx = getRandom(tree->raw_size());

            Int offset1 = tree->value_offset(idx);
            Int offset2 = value_offset(tree, idx);

            AssertEQ(MA_SRC, offset1, offset2);
        }
    }
};


}


#endif
