
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PFSE_CREATE_HPP_
#define MEMORIA_TESTS_PFSE_CREATE_HPP_

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include "pfse_test_base.hpp"

#include <memory>

namespace memoria {

using namespace std;

template <
	typename IK,
	typename V,
	Int Blocks_				= 1,
	Int BF 					= PackedTreeBranchingFactor,
	Int VPB 				= PackedTreeBranchingFactor
>
struct PackedFSETreeCreateTypes {
    typedef IK              IndexValue;
    typedef V               Value;
    typedef PackedAllocator	Allocator;

    template <typename CV>
    using Codec = FSECodec<Value, CV>;

    static const Int Blocks                 = Blocks_;
    static const Int BranchingFactor        = BF;
    static const Int ValuesPerBranch        = VPB;

    static const Int ALIGNMENT_BLOCK        = 8;
};


template <Int BF, Int VPB>
class PackedFSETreeCreateTest: public PackedFSETestBase<PackedFSETreeCreateTypes<Int, Int, 1, BF, VPB>> {

	typedef PackedFSETreeCreateTest																MyType;
	typedef PackedFSETestBase<PackedFSETreeCreateTypes<Int, Int, 1, BF, VPB>> 					Base;

	typedef typename Base::Tree 			Tree;
	typedef typename Base::Value			Value;
	typedef typename Base::Values			Values;

public:

	PackedFSETreeCreateTest(): Base((SBuf()<<"Create."<<BF<<"."<<VPB).str())
    {
    	MEMORIA_ADD_TEST(testReindex);
    }

    virtual ~PackedFSETreeCreateTest() throw() {}


    void testReindex()
    {
    	for (Int block_size = 160; block_size < 128*1024; ) //sizeof(Tree)
    	{
    		testReindex(block_size);

    		if (block_size < 4096)
    		{
    			block_size += 1;
    		}
    		else {
    			block_size += getRandom(1000) + 1;
    		}
    	}
    }

    void testReindex(Int block_size)
    {
    	Base::out()<<"Block size: "<<block_size<<endl;
    	Tree* tree = Base::createTree(block_size);

    	Value c = 0;
    	Base::fillTree(0, [&]()->Values {
    		return Values(c++);
    	});


    	for (int idx = 0; idx < tree->size(); idx++)
    	{
    		AssertEQ(MA_SRC, idx, tree->value(idx));
    	}

    	if (tree->size() > Tree::ValuesPerBranch)
    	{
    		Int max = Base::sumValues(tree, 0, tree->size());
    		AssertEQ(MA_SRC, tree->maxValue(0), max);
    	}
    }
};


}


#endif
