
// Copyright Victor Smirnov 2013-2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_BTTL_TEST_BASE_HPP_
#define MEMORIA_TESTS_BTTL_TEST_BASE_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/profile_tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/prototypes/bt_tl/bttl_factory.hpp>
#include <memoria/prototypes/bt_tl/tools/bttl_random_gen.hpp>

#include "bt_test_base.hpp"

#include <functional>

namespace memoria {

using namespace std;

template <
	typename ContainerTypeName,
    typename AllocatorType,
	typename Profile
>
class BTTLTestBase: public BTTestBase<ContainerTypeName, AllocatorType, Profile> {

    using MyType = BTTLTestBase<
                ContainerTypeName,
				Profile,
				AllocatorType
    >;

    using Base = BTTestBase<ContainerTypeName, AllocatorType, Profile>;

protected:
    using Ctr 			= typename CtrTF<Profile, ContainerTypeName>::Type;
    using Iterator 		= typename Ctr::Iterator;
    using ID 			= typename Ctr::ID;
    using Accumulator 	= typename Ctr::Accumulator;

    using Allocator 	= AllocatorType;

    using CtrSizesT		= typename Ctr::Types::Position;
    using CtrSizeT		= typename Ctr::Types::CtrSizeT;

    static const Int Streams = Ctr::Types::Streams;

    bool dump = false;

public:

    BTTLTestBase(StringRef name):
        Base(name)
    {
        Ctr::initMetadata();

        MEMORIA_ADD_TEST_PARAM(dump);
    }

    virtual ~BTTLTestBase() throw() {}

    CtrSizesT sampleTreeShape(Int level_limit, Int last_level_limit, CtrSizeT size)
    {
    	CtrSizesT shape;

    	CtrSizesT limits(level_limit);
    	limits[Streams - 1] = last_level_limit;

    	while(shape[0] == 0)
    	{
    		BigInt resource = size;

    		for (Int c = Streams - 1; c > 0; c--)
    		{
    			Int level_size = getRandom(limits[c]) + ((c == Streams - 1)? 10 : 1);

    			shape[c] = level_size;

    			resource = resource / level_size;
    		}

    		shape[0] = resource;
    	}

    	return shape;
    }

    virtual void checkAllocator(const char* msg, const char* source)
    {
    	::memoria::check<Allocator>(*this->allocator_.get(), msg, source);
    }

    template <typename Provider>
    CtrSizesT fillCtr(Ctr& ctr, Provider& provider)
    {
    	auto iter = ctr.seek(0);

    	long t0 = getTimeInMillis();

    	auto totals = ctr._insert(iter, provider);

    	checkAllocator("Bulk Insertion", MA_SRC);

    	long t1 = getTimeInMillis();

    	this->out()<<"Creation time: "<<FormatTime(t1 - t0)<<" consumed: "<<provider.consumed()<<endl;

    	auto sizes = ctr.sizes();

    	AssertEQ(MA_SRC, provider.consumed(), sizes);
    	AssertEQ(MA_SRC, totals, sizes);

    	auto ctr_totals = ctr.total_counts();

    	this->out()<<"Totals: "<<ctr_totals<<" "<<sizes<<endl;

    	AssertEQ(MA_SRC, ctr_totals, sizes);

    	this->allocator()->commit();

    	if (dump)
    		this->storeAllocator("core.dump");

    	return totals;
    }

    void checkSubtree(Ctr& ctr, CtrSizeT r)
    {
    	CtrSizesT path(-1);
    	path[0] = r;

    	this->checkSubtree(ctr, path);
    }

    void checkSubtree(Ctr& ctr, const CtrSizesT& path)
    {
    	auto iter = ctr.seek(path[0]);

    	for (Int s = 1; s < Streams; s++)
    	{
    		if (path[s] >= 0)
    		{
    			iter.toData(path[s]);
    		}
    	}

    	this->checkSubtree(iter, iter.stream(), 0);
    }


    void checkSubtree(Iterator& iter, Int level = 0, CtrSizeT scan_size = 1)
    {
    	AssertEQ(MA_SRC, iter.stream(), level);

    	for (Int s = 0; s < scan_size; s++)
    	{
    		if (level > 0)
    		{
    			AssertEQ(MA_SRC, iter.pos(), s);
    		}

    		iter.toData();
    		iter.checkPrefix();

    		if (level < Streams - 1)
    		{
    			CtrSizeT size = iter.size();
    			checkSubtree(iter, level + 1, size);
    		}
    		else {

    			Int data = 0;
    			CtrSizeT cnt = 0;
    			auto scanned = iter.template scan<IntList<2>>([&](const auto* obj, Int start, Int end) {
    				if (cnt == 0)
    				{
    					data = obj->value(start);
    				}

    				for (Int c = start; c < end; c++)
    				{
    					AssertEQ(MA_SRC, data, obj->value(c));
    				}

    				cnt += end - start;
    			});

    			AssertEQ(MA_SRC, scanned, scan_size);
    		}

    		iter.toIndex();
    		iter.skipFw(1);
    	}
    }

    void checkRanks(Ctr& ctr)
    {
    	auto i = ctr.seek(0);

    	CtrSizesT extents;

    	long t2 = getTimeInMillis();

    	do
    	{
    		auto sizes = i.leaf_sizes();

    		auto total_leaf_rank = sizes.sum();

    		typename Ctr::Types::LeafPrefixRanks prefix_ranks;

    		ctr.compute_leaf_prefixes(i.leaf(), extents, prefix_ranks);

    		auto total_ranks = ctr.leaf_rank(i.leaf(), sizes, prefix_ranks, sizes.sum());

    		AssertEQ(MA_SRC, total_ranks, sizes);

    		for (Int c = 0; c < total_leaf_rank; )
    		{
    			auto ranks = ctr.leaf_rank(i.leaf(), sizes, prefix_ranks, c);

    			AssertEQ(MA_SRC, ranks.sum(), c);

    			c += getRandom(100) + 1;
    		}

    		extents += ctr.node_extents(i.leaf());
    	}
    	while(i.nextLeaf());

    	long t3 = getTimeInMillis();

    	this->out()<<"Rank verification time: "<<FormatTime(t3 - t2)<<endl;

    }


    void checkExtents(Ctr& ctr)
    {
    	auto i = ctr.seek(0);

    	CtrSizesT extent;

    	long t2 = getTimeInMillis();

    	do
    	{
    		auto current_extent = i.leaf_extent();

    		AssertEQ(MA_SRC, current_extent, extent);

    		for (Int c = 0; c < Streams; c++) {

    			if (extent[c] < 0)
    			{
    				i.dump(this->out());
    			}

    			AssertGE(MA_SRC, extent[c], 0);
    		}

    		extent += ctr.node_extents(i.leaf());
    	}
    	while(i.nextLeaf());

    	long t3 = getTimeInMillis();

    	this->out()<<"Extent verification time: "<<FormatTime(t3 - t2)<<endl;
    }
};

}


#endif