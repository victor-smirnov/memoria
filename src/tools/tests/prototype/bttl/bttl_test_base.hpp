
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_BTTL_TEST_BASE_HPP_
#define MEMORIA_TESTS_BTTL_TEST_BASE_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/profile_tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/prototypes/bt_tl/bttl_factory.hpp>
#include <memoria/prototypes/bt_tl/tools/bttl_tools_random_gen.hpp>
#include "../bt/bt_test_base.hpp"
#include "bttl_test_factory.hpp"

#include <functional>

namespace memoria {

using namespace std;


template <
    typename ContainerTypeName,
    typename AllocatorType,
    typename Profile
>
class BTTLTestBase;

template <
    Int Levels,
    PackedSizeType SizeType,
    typename AllocatorType,
    typename Profile
>
class BTTLTestBase<BTTLTestCtr<Levels, SizeType>, AllocatorType, Profile>: public BTTestBase<BTTLTestCtr<Levels, SizeType>, AllocatorType, Profile> {

    using ContainerTypeName = BTTLTestCtr<Levels, SizeType>;

    using MyType = BTTLTestBase<
                ContainerTypeName,
                Profile,
                AllocatorType
    >;

    using Base = BTTestBase<ContainerTypeName, AllocatorType, Profile>;

protected:
    using Ctr           = typename CtrTF<Profile, ContainerTypeName>::Type;
    using Iterator      = typename Ctr::Iterator;
    using IteratorPtr      	= typename Ctr::IteratorPtr;
    using ID            	= typename Ctr::ID;
    using BranchNodeEntry   = typename Ctr::BranchNodeEntry;

    using Allocator     = AllocatorType;

    using CtrSizesT     = typename Ctr::Types::CtrSizesT;
    using CtrSizeT      = typename Ctr::Types::CtrSizeT;

    static const Int Streams = Ctr::Types::Streams;


    using Base::commit;
    using Base::drop;
    using Base::branch;
    using Base::allocator;
    using Base::snapshot;
    using Base::check;
    using Base::out;
    using Base::storeAllocator;
    using Base::isReplayMode;
    using Base::getResourcePath;



    bool dump = false;

    BigInt size             = 1000000;
    Int iterations          = 5;
    Int level_limit         = 1000;
    Int last_level_limit    = 100;



public:

    BTTLTestBase(StringRef name):
        Base(name)
    {
        Ctr::initMetadata();

        MEMORIA_ADD_TEST_PARAM(size);
        MEMORIA_ADD_TEST_PARAM(iterations);
        MEMORIA_ADD_TEST_PARAM(dump);
        MEMORIA_ADD_TEST_PARAM(level_limit);
        MEMORIA_ADD_TEST_PARAM(last_level_limit);
    }

    virtual ~BTTLTestBase() throw() {}

    virtual void smokeCoverage(Int scale)
    {
        size        = 10000 * scale;
        iterations  = 1;
    }

    virtual void smallCoverage(Int scale)
    {
        size        = 100000 * scale;
        iterations  = 1;
    }

    virtual void normalCoverage(Int scale)
    {
        size        = 10000000 * scale;
        iterations  = 1;
    }

    virtual void largeCoverage(Int scale)
    {
        size        = 100000000 * scale;
        iterations  = 10;
    }

    CtrSizesT sampleTreeShape() {
        return sampleTreeShape(level_limit, last_level_limit, size);
    }

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
                Int level_size = this->getRandom(limits[c]) + ((c == Streams - 1)? 10 : 1);

                shape[c] = level_size;

                resource = resource / level_size;
            }

            shape[0] = resource;
        }

        return shape;
    }

    template <typename Provider>
    CtrSizesT fillCtr(Ctr& ctr, Provider& provider)
    {
        auto iter = ctr.seek(0);

        long t0 = getTimeInMillis();

        auto totals = ctr._insert(*iter.get(), provider);

        check("Bulk Insertion", MA_SRC);

        long t1 = getTimeInMillis();

        out() << "Creation time: " << FormatTime(t1 - t0) << " consumed: " << provider.consumed() << endl;

        auto sizes = ctr.sizes();

        AssertEQ(MA_SRC, provider.consumed(), sizes);
        AssertEQ(MA_SRC, totals, sizes);

        auto ctr_totals = ctr.total_counts();

        out()<<"Totals: "<<ctr_totals<<" "<<sizes<<endl;

        AssertEQ(MA_SRC, ctr_totals, sizes);

        return totals;
    }

    void checkIterator(const char* source, const IteratorPtr& iter, const CtrSizesT& positions, Int level = 0)
    {
    	auto& cache = iter->cache();

    	AssertEQ(source, cache.data_pos(), positions, SBuf()<<"Positions do not match");
    	AssertEQ(source, cache.abs_pos()[0], positions[0], SBuf()<<"Level 0 absolute positions do not match");

    	for (Int l = level + 1; l < Streams; l++)
    	{
    		AssertEQ(source, cache.data_pos()[l], -1, SBuf()<<"Position for level "<<l<<" is not default (-1)");
    		AssertEQ(source, cache.data_size()[l], -1, SBuf()<<"Substream size for level "<<l<<" is not default (-1)");
    		AssertEQ(source, cache.abs_pos()[l], -1, SBuf()<<"Absolute position for level "<<l<<" is not default (-1)");
    	}
    }


    void scanAndCheckIterator(Ctr& ctr)
    {
    	CtrSizesT pos{-1};
    	CtrSizesT abs_pos;

    	scanAndCheckIterator_(ctr.begin(), pos, abs_pos, ctr.size());

    	AssertEQ(MA_RAW_SRC, abs_pos, ctr.sizes());
    }

private:
    void scanAndCheckIterator_(const IteratorPtr& iter, CtrSizesT& pos, CtrSizesT& abs_pos, CtrSizeT size, Int level = 0)
    {
    	AssertEQ(MA_SRC, iter->stream(), level);

    	AssertEQ(MA_SRC, iter->size(), size);

    	pos[level] = 0;


    	if (level < Streams - 1)
    	{
    		for (CtrSizeT c = 0; c < size; c++)
    		{
    			auto substream_size = iter->substream_size();

    			iter->toData();

    			scanAndCheckIterator_(iter, pos, abs_pos, substream_size, level + 1);

    			iter->toIndex();

    			checkIterator(MA_RAW_SRC, iter, pos, level);
    			AssertEQ(MA_RAW_SRC, abs_pos[level], iter->cache().abs_pos()[level]);

    			auto len = iter->skipFw(1);
    			AssertEQ(MA_RAW_SRC, len, 1);

    			pos[level] ++;
    			abs_pos[level] ++;
    		}
    	}
    	else {
    		checkIterator(MA_RAW_SRC, iter, pos, level);
    		AssertEQ(MA_RAW_SRC, abs_pos[level], iter->cache().abs_pos()[level]);

    		auto len = iter->skipFw(size);
    		AssertEQ(MA_RAW_SRC, len, size);

    		pos[level] += size;
    		abs_pos[level] += size;
    	}

    	checkIterator(MA_RAW_SRC, iter, pos, level);

    	pos[level] = -1;
    }


public:


    void checkTree(Ctr& ctr)
    {
        auto iter = ctr.seek(0);
        checkSubtree(*iter.get(), ctr.sizes()[0]);
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

        checkIterator(MA_RAW_SRC, iter, path, 0);

        for (Int s = 1; s < Streams; s++)
        {
            if (path[s] >= 0)
            {
                iter->toData(path[s]);
                checkIterator(MA_RAW_SRC, iter, path, s);
            }
        }

        this->checkSubtree(*iter.get(), 1);
    }




    void checkSubtree(Iterator& iter, CtrSizeT scan_size = 1)
    {
        if (iter.stream() < Streams - 1)
        {
            for (Int s = 0; s < scan_size; s++)
            {
                CtrSizeT size = iter.substream_size();

                iter.toData();
                iter.checkPrefix();

                AssertEQ(MA_SRC, size, iter.size());

                checkSubtree(iter, size);

                iter.toIndex();
                iter.skipFw(1);
            }
        }
        else {
            Int data = 0;
            CtrSizeT cnt = 0;
            if (scan_size == -1) scan_size = 0;

            auto scanned = iter.template scan<IntList<Levels - 1>>([&](const auto* obj, Int start, Int end) {
                if (cnt == 0)
                {
                    data = obj->value(0, start);
                }

                for (Int c = start; c < end; c++)
                {
                    AssertEQ(MA_SRC, data, obj->value(0, c));
                }

                cnt += end - start;
            });

            AssertEQ(MA_SRC, scanned, scan_size);
            AssertEQ(MA_SRC, scanned, cnt);
        }
    }

    void checkRanks(Ctr& ctr)
    {
        auto i = ctr.seek(0);

        CtrSizesT extents;

        long t2 = getTimeInMillis();

        do
        {
            auto sizes = i->leaf_sizes();

            auto total_leafrank_ = sizes.sum();

            typename Ctr::Types::LeafPrefixRanks prefix_ranks;

            ctr.compute_leaf_prefixes(i->leaf(), extents, prefix_ranks);

            auto total_ranks = ctr.leafrank_(i->leaf(), sizes, prefix_ranks, sizes.sum());

            AssertEQ(MA_SRC, total_ranks, sizes);

            for (Int c = 0; c < total_leafrank_; )
            {
                auto ranks = ctr.leafrank_(i->leaf(), sizes, prefix_ranks, c);

                AssertEQ(MA_SRC, ranks.sum(), c);

                c += this->getRandom(100) + 1;
            }

            extents += ctr.node_extents(i->leaf());
        }
        while(i->nextLeaf());

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
            auto current_extent = i->leaf_extent();

            AssertEQ(MA_SRC, current_extent, extent);

            for (Int c = 0; c < Streams; c++)
            {
                if (extent[c] < 0)
                {
                    i->dumpPath(this->out());
                }

                AssertGE(MA_SRC, extent[c], 0, SBuf()<<"Extent: "<<extent);
            }

            auto ex = ctr.node_extents(i->leaf());

            extent += ex;
        }
        while(i->nextLeaf());

        long t3 = getTimeInMillis();

        this->out()<<"Extent verification time: "<<FormatTime(t3 - t2)<<endl;
    }
};

}


#endif
