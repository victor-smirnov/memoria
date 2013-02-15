
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PMAP_PMAP_SUM_HPP_
#define MEMORIA_TESTS_PMAP_PMAP_SUM_HPP_

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/prototypes/btree/tools.hpp>

#include <memoria/core/packed/packed_sum_tree.hpp>

#include <memory>

namespace memoria {

using namespace std;



template <Int BranchingFactor_>
class PMapSumTest: public TestTask {

    typedef PMapSumTest<BranchingFactor_> MyType;

    template <typename Key_, typename Value_, Int BF>
    struct PMapSumTypes {
        typedef Key_                        Key;
        typedef Key_                        IndexKey;
        typedef Value_                      Value;

        static const Int Blocks             = 1;
        static const Int BranchingFactor    = BF;

        typedef Accumulators<Key, Blocks>   Accumulator;
    };

    typedef PMapSumTypes<BigInt, EmptyValue, BranchingFactor_>  Types;

    typedef typename Types::Accumulator     Accumulator;
    typedef typename Types::Key             Key;
    typedef typename Types::Value           Value;

    static const Int Blocks                 = Types::Blocks;

    typedef PackedSumTree<Types>                Map;

    Int block_size  = 16384;
    Int max_size    = 0;

    Int start;
    Int end;

    Int size;


public:

    PMapSumTest():
        TestTask("Sum."+toString(BranchingFactor_)),
        block_size(16384),
        max_size(0)
    {
        MEMORIA_ADD_TEST_PARAM(block_size);
        MEMORIA_ADD_TEST_PARAM(max_size);

        MEMORIA_ADD_TEST_PARAM(start)->state();
        MEMORIA_ADD_TEST_PARAM(end)->state();
        MEMORIA_ADD_TEST_PARAM(size)->state();

        MEMORIA_ADD_TEST_WITH_REPLAY(runTest, runReplay);
    }

    virtual ~PMapSumTest() throw() {}

    void runReplay()
    {
        unique_ptr<Byte[]>  buffer_ptr(new Byte[block_size]);
        Byte* buffer        = buffer_ptr.get();


        Map* map            = T2T<Map*>(buffer);

        map->initByBlock(block_size - sizeof(Map));

        FillMap(map, max_size != 0 ? max_size : map->maxSize());

        Accumulator acc;
        map->sum(start, end, acc);

        BigInt sum = Sum(map, start, end);

        AssertEQ(MA_SRC, acc[0], sum, SBuf()<<start<<","<<end);
    }

    void FillMap(Map* map, Int size)
    {
        for (Int c = 0; c < size; c++)
        {
            for (Int d = 0; d < Blocks; d++)
            {
                map->key(d, c) = getRandom(50);
            }
        }

        map->size() = size;

        map->reindexAll(0, size);
    }


    BigInt Sum(Map* map, Int start, Int end) const
    {
        BigInt sum = 0;
        for (Int c = start; c < end; c++)
        {
            sum += map->key(0, c);
        }
        return sum;
    }

    void runTest()
    {
        unique_ptr<Byte[]>  buffer_ptr(new Byte[block_size]);
        Byte* buffer        = buffer_ptr.get();

        Map* map            = T2T<Map*>(buffer);

        map->initByBlock(block_size - sizeof(Map));

        FillMap(map, max_size != 0 ? max_size : map->maxSize());

        for (Int end = 0; end < map->size(); end++)
        {
            out()<<end<<endl;

            for (Int start = 0; start < end; start++)
            {
                Accumulator acc;
                map->sum(start, end, acc);

                BigInt sum = Sum(map, start, end);

                AssertEQ(MA_SRC, acc[0], sum, SBuf()<<start<<","<<end);
            }
        }
    }
};


}


#endif /* PMAP_DATA_HPP_ */
