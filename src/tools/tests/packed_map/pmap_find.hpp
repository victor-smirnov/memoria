
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PMAP_PMAP_FIND_HPP_
#define MEMORIA_TESTS_PMAP_PMAP_FIND_HPP_

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/prototypes/btree/tools.hpp>

#include <memoria/core/packed/packed_sum_tree.hpp>

#include <memory>

namespace memoria {

using namespace std;


class PMapFindTest: public TestTask {

    typedef PMapFindTest MyType;

    template <typename Key_, typename Value_, Int Blocks_ = 3>
    struct PMapfindTypes {
        typedef Key_                        Key;
        typedef Key_                        IndexKey;
        typedef Value_                      Value;

        static const Int Blocks             = Blocks_;
        static const Int BranchingFactor    = 8;

        typedef Accumulators<Key, Blocks>   Accumulator;
    };


    typedef PMapfindTypes<Int, Int, 1>      Types;

    typedef typename Types::Accumulator     Accumulator;
    typedef typename Types::Key             Key;
    typedef typename Types::Value           Value;

    static const Int Blocks                 = Types::Blocks;

    typedef PackedSumTree<Types>            Map;

    Int buffer_size     = 1024*16;

public:

    PMapFindTest(): TestTask("Find")
    {
        MEMORIA_ADD_TEST_PARAM(buffer_size);

        MEMORIA_ADD_TEST(runTest);
    }

    virtual ~PMapFindTest() throw() {}

    void FillPMap(Map* map, Int size)
    {
        for (Int c = 0; c < size; c++)
        {
            map->key(0, c) = 2;
        }

        map->size() = size;

        map->reindex(0);

        AssertEQ(MA_SRC, map->maxKey(0), size*2);
    }


    void runTest(ostream& out)
    {
        unique_ptr<Byte[]>  buffer_ptr(new Byte[buffer_size]);
        Byte* buffer        = buffer_ptr.get();

        memset(buffer, buffer_size, 0);

        Map* map = T2T<Map*>(buffer);

        for (Int div = 1; div <= 16; div *= 2)
        {
            map->initByBlock(buffer_size / div);

            for (Int size = 0; size < map->maxSize(); size++)
            {
                map->key(0, size) = 2;
                map->size()++;
                map->reindex(0, size, size + 1);

                for (Int c = 0; c < map->size(); c++)
                {
                    Key src_key = (c + 1) * 2;

                    AssertEQ(MA_SRC, c, map->findLE(0, src_key));
                    AssertEQ(MA_SRC, c, map->findLE(0, src_key - 1));

                    AssertEQ(MA_SRC, c, map->findLT(0, src_key - 1));

                    Int sum1 = 0, sum2 = 0;
                    AssertEQ(MA_SRC, c, map->findLTS(0, src_key - 1, sum1));

                    map->sum(0, 0, c, sum2);
                    AssertEQ(MA_SRC, sum1, sum2);


                    if (c < map->size() - 1)
                    {
                    	AssertEQ(MA_SRC, c + 1, map->findLT(0, src_key));
                    }
                    else {
                    	AssertEQ(MA_SRC, map->size(), map->findLT(0, src_key));
                    }
                }
            }
        }
    }
};


}


#endif
