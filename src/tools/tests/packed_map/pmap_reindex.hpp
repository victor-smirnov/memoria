
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PMAP_PMAP_INDEX_HPP_
#define MEMORIA_TESTS_PMAP_PMAP_INDEX_HPP_

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/prototypes/btree/tools.hpp>

#include <memoria/core/packed/packed_sum_tree.hpp>

#include <memory>

namespace memoria {

using namespace std;


class PMapReindexTest: public TestTask {

    typedef PMapReindexTest MyType;

	static const Int Blocks                 = 1;

	typedef Int             				Key;
	typedef Int           					Value;
	typedef Accumulators<Int, Blocks>     	Accumulator;

    typedef PackedTreeTypes<
    		Key,
    		Key,
    		Value,
    		Accumulator,
    		Blocks,
    		4
    >										Types;

    typedef PackedSumTree<Types>                Map;

public:

    PMapReindexTest(): TestTask("Reindex")
    {
        MEMORIA_ADD_TEST(runTest);
    }

    virtual ~PMapReindexTest() throw() {}

    BigInt FillPMap(Map* map, Int size)
    {
        BigInt sum = 0;

        for (Int c = 0; c < size; c++)
        {
            Int key = getRandom(1000);
            map->key(0, c) = key;

            sum += key;
        }

        map->size() = size;

        map->reindex(0);

        return sum;
    }


    void runTest()
    {
        Int buffer_size     = 1024*64;

        unique_ptr<Byte[]>  buffer_ptr(new Byte[buffer_size]);
        Byte* buffer        = buffer_ptr.get();

        memset(buffer, buffer_size, 0);

        Map* map = T2T<Map*>(buffer);

        for (int c = 1; c < 64; c++)
        {
            out()<<"Reindex Pass: "<<c<<endl;

            map->initByBlock(buffer_size / c);

            for (Int c = 1; c < map->maxSize(); c++)
            {
                Int sum = FillPMap(map, c);
                AssertEQ(MA_SRC, map->maxKey(0), sum, SBuf()<<"c="<<c);
            }

            for (Int c = map->maxSize() - 1; c > 0; c--)
            {
                Int sum = FillPMap(map, c);
                AssertEQ(MA_SRC, map->maxKey(0), sum, SBuf()<<"c="<<c);
            }
        }

        out()<<endl<<endl;

        for (int c = 1; c < 64; c++)
        {
            out()<<"Add Pass: "<<c<<endl;

            map->initByBlock(buffer_size / c);

            Int sum = FillPMap(map, map->maxSize());

            for (Int c = 0; c < map->maxSize(); c++)
            {
                map->add(0, c, 1);

                sum++;

                AssertEQ(MA_SRC, map->maxKey(0), sum, SBuf()<<"c="<<c);
            }
        }
    }
};


}


#endif
