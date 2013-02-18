
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PMAP_PMAP_TRANSFER_HPP_
#define MEMORIA_TESTS_PMAP_PMAP_TRANSFER_HPP_

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/prototypes/btree/tools.hpp>

#include <memoria/core/packed/packed_sum_tree.hpp>

#include <memory>

namespace memoria {

using namespace std;

template <bool TheSameMapTypes>
class PMapTransferTest: public TestTask {

    typedef PMapTransferTest<TheSameMapTypes> MyType;

	static const Int Blocks                 = 8;

	typedef Int             				Key;
	typedef Int           					Value;
	typedef Accumulators<BigInt, Blocks>    Accumulator;

    typedef PackedTreeTypes<
    		Key,
    		Key,
    		Value,
    		Accumulator,
    		Blocks,
    		16
    >										Types1;


    typedef typename IfThenElse<
    		TheSameMapTypes,
    		Types1,
    		PackedTreeTypes<
				BigInt,
				BigInt,
				BigInt,
				Accumulator,
				Blocks,
				16
    		>
    >::Result                               Types2;

    typedef PackedSumTree<Types1>           Map1;
    typedef PackedSumTree<Types2>           Map2;

    struct FillMapResult {
        std::vector<Int> keys[Blocks];
        std::vector<Int> values;
    };

    Int offset1;
    Int max1;
    Int offset2;
    Int max2;
    Int size;

public:

    PMapTransferTest(StringRef name = "Transfer"): TestTask(name)
    {
        MEMORIA_ADD_TEST_PARAM(offset1)->state();
        MEMORIA_ADD_TEST_PARAM(max1)->state();
        MEMORIA_ADD_TEST_PARAM(offset2)->state();
        MEMORIA_ADD_TEST_PARAM(max2)->state();
        MEMORIA_ADD_TEST_PARAM(size)->state();

        MEMORIA_ADD_TEST_WITH_REPLAY(runTest, runReplay);
    }

    virtual ~PMapTransferTest() throw() {}

    void runReplay()
    {
        out()<<offset1<<" "<<max1<<" "<<offset2<<" "<<max2<<" "<<size<<endl;
        test(offset1, max1, offset2, max2, size);
    }

    FillMapResult FillPMap(Map1* map, Int size)
    {
        map->size() = 0;
        map->clearUnused();

        FillMapResult result;

        for (Int block = 0; block < Blocks; block++) {
            for (Int c = 0; c < size; c++)
            {
                Int key = getRandom(1000);
                map->key(block, c) = key;
                result.keys[block].push_back(key);
            }
        }

        for (Int c = 0; c < size; c++)
        {
            Int value = getRandom(1000);
            map->value(c) = value;
            result.values.push_back(value);
        }

        map->size() = size;

        map->reindex();

        return result;
    }

    void test(Int offset1, Int max1, Int offset2, Int max2, Int size)
    {

        Int max = max1 > max2 ? max1 : max2;
        Int offset_max = offset1 > offset2 ? offset1 : offset2;

        Map2 tmp;
        tmp.initSizes(max);

        Int buffer_size     = offset_max + sizeof(Map2) + tmp.getTotalDataSize();

        unique_ptr<Byte[]>  buffer_ptr(new Byte[buffer_size]);
        Byte* buffer        = buffer_ptr.get();

        MemBase = T2T<size_t>(buffer);

        memset(buffer, buffer_size, 0);

        Map1* map1  = T2T<Map1*>(buffer + offset1);
        Map2* map2  = T2T<Map2*>(buffer + offset2);

        Map2 map2_buf;

        map1->initSizes(max1);
        map2_buf.initSizes(max2);

        auto result = FillPMap(map1, size);

        map2_buf.size() = map1->size();

        if (isReplayMode()) {
            map1->dumpRanges(map1->memoryBlock(), out());
            map2_buf.dumpRanges(map2->memoryBlock(), out());
        }

        map1->transferTo(&map2_buf, map2->memoryBlock());

        *map2 = map2_buf;

        map2->reindex();

        for (Int block = 0; block < Blocks; block++)
        {
            for (Int c = 0; c < map2->size(); c++)
            {
            	AssertEQ(MA_SRC, map2->key(block, c), result.keys[block][c], SBuf()<<c<<" "<<block);
            }
        }

        for (Int c = 0; c < map2->size(); c++)
        {
        	AssertEQ(MA_SRC, map2->value(c), result.values[c], SBuf()<<"c="<<c);
        }
    }

    void runTest()
    {
        for (int c = 0; c < 10000; c++)
        {
            out()<<c<<endl;

            Int max1 = getRandom(4500) + 500;
            Int max2 = getRandom(4500) + 500;

            Int min = max1 > max2 ? max2 : max1;

            Int offset1 = getRandom(20)*8 + 100;
            Int offset2 = getRandom(20)*8 + 100;

            this->offset1   = offset1;
            this->max1      = max1;
            this->offset2   = offset2;
            this->max2      = max2;
            this->size      = min;

            test(offset1, max1, offset2, max2, min);
        }
    }
};


}


#endif
