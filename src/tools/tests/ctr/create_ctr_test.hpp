
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_CREATE_CTR_CREATE_CTR_TEST_HPP_
#define MEMORIA_TESTS_CREATE_CTR_CREATE_CTR_TEST_HPP_

#include "../shared/params.hpp"
#include "../tests_inc.hpp"

#include <vector>

namespace memoria {

using namespace memoria::vapi;
using namespace std;






class CreateCtrTest: public SPTestTask {

    typedef CreateCtrTest                                                       MyType;

    typedef KVPair<BigInt, BigInt>                                              Pair;
    typedef vector<Pair>                                                        PairVector;
    typedef SCtrTF<VectorMap<BigInt, Byte>>::Type                               VectorMapCtr;
    typedef SCtrTF<Map1>::Type                                                  MapCtr;
    typedef VectorMapCtr::Iterator                                              VMIterator;

    PairVector pairs_;

    Int map_size_           = 1024*256;
    Int vector_map_size_    = 200;
    Int block_size_         = 1024;

    Int iteration_          = 0;

    BigInt map_name_;
    BigInt vector_map_name_;

public:

    CreateCtrTest(): SPTestTask("CreateCtr")
    {
        MEMORIA_ADD_TEST_PARAM(map_size_)->setDescription("Size of the Map container");
        MEMORIA_ADD_TEST_PARAM(vector_map_size_)->setDescription("Size of the VectorMap container");
        MEMORIA_ADD_TEST_PARAM(block_size_)->setDescription("Size of data block inserted into VectorMap container");

        MEMORIA_ADD_TEST_PARAM(map_name_)->state();
        MEMORIA_ADD_TEST_PARAM(vector_map_name_)->state();
        MEMORIA_ADD_TEST_PARAM(iteration_)->state();


        MEMORIA_ADD_TEST(runTest);
    }

    virtual ~CreateCtrTest() throw() {}




    void runTest(ostream& out)
    {
        DefaultLogHandlerImpl logHandler(out);


        if (btree_random_branching_)
        {
            btree_branching_ = 8 + getRandom(100);
            out<<"BTree Branching: "<<btree_branching_<<endl;
        }

        Allocator allocator;
        allocator.getLogger()->setHandler(&logHandler);

        MapCtr map(&allocator);

        map.setBranchingFactor(100);

        map_name_ = map.name();

        BigInt t00 = getTimeInMillis();

        for (Int c = 0; c < map_size_; c++)
        {
            map[getRandom()] = getRandom();
        }

        VectorMapCtr vector_map(&allocator);

        vector_map.setBranchingFactor(100);

        vector_map_name_ = vector_map.name();

        for (Int c = 0; c < vector_map_size_; c++)
        {
            vector_map[getRandom()] = createBuffer<Byte>(getRandom(block_size_), getRandom(256));
        }

        allocator.commit();

        check(allocator, MEMORIA_SOURCE);

        BigInt t0 = getTimeInMillis();

        String name = this->getResourcePath("alloc1.dump");

        StoreAllocator(allocator, name);

        BigInt t1 = getTimeInMillis();

        Allocator new_alloc;

        LoadAllocator(new_alloc, name);

        BigInt t2 = getTimeInMillis();

        out<<"Store Time: "<<FormatTime(t1 - t0)<<endl;
        out<<"Load Time:  "<<FormatTime(t2 - t1)<<endl;

        check(new_alloc, MEMORIA_SOURCE);

        MapCtr new_map(&new_alloc, map.name());

        MEMORIA_TEST_THROW_IF(map.getBranchingFactor(), !=, new_map.getBranchingFactor());

        MEMORIA_TEST_THROW_IF(map.getSize(), !=, new_map.getSize());

        auto new_iter = new_map.Begin();

        for (auto iter = map.Begin(); !iter.isEnd(); iter.nextKey(), new_iter.nextKey())
        {
            MEMORIA_TEST_THROW_IF(iter.getKey(0), !=, new_iter.getKey(0));
            MEMORIA_TEST_THROW_IF(iter.getValue(), !=, new_iter.getValue());
        }

        BigInt t22 = getTimeInMillis();

        VectorMapCtr new_vector_map(&new_alloc, vector_map.name());

        MEMORIA_TEST_THROW_IF(vector_map.getBranchingFactor(), !=, new_vector_map.getBranchingFactor());

        auto new_vm_iter = new_vector_map.Begin();

        for (auto iter = vector_map.Begin(); iter.isNotEnd(); iter.next(), new_vm_iter.next())
        {
            MEMORIA_TEST_THROW_IF(iter.size(), !=, new_vm_iter.size());

            MemBuffer<Byte> data = iter.read();

            checkBufferWritten(new_vm_iter, data, "Array data check failed", MEMORIA_SOURCE);
        }

        BigInt t33 = getTimeInMillis();

        out<<"Create Time: "<<FormatTime(t0 - t00)<<endl;
        out<<"check Time:  "<<FormatTime(t22 - t2)<<endl;
        out<<"check Time:  "<<FormatTime(t33 - t22)<<endl;
    }


};


}


#endif

