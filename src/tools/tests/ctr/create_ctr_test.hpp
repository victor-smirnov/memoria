
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

    typedef KVPair<BigInt, BigInt>                                      Pair;
    typedef vector<Pair>                                                PairVector;
    typedef SmallCtrTypeFactory::Factory<VectorMapCtr>::Type            VectorMapCtrType;
    typedef SmallCtrTypeFactory::Factory<Map1Ctr>::Type                    MapCtrType;
    typedef VectorMapCtrType::Iterator                                  VMIterator;

    struct TaskReplay: public ReplayParams {

        BigInt map_name_;
        BigInt vector_map_name_;

        TaskReplay(): ReplayParams()
        {
            Add("map_name", map_name_);
            Add("vector_map_name", vector_map_name_);
        }
    };



    PairVector pairs_;

    Int map_size_;
    Int vector_map_size_;
    Int block_size_;

public:

    CreateCtrTest(): SPTestTask("CreateCtr"), map_size_(1024*256), vector_map_size_(200), block_size_(1024)
    {
        //SmallCtrTypeFactory::Factory<Root>::Type::initMetadata();
        VectorMapCtrType::initMetadata();
        MapCtrType::initMetadata();

        Add("MapSize", map_size_);
        Add("VectorMapSize", vector_map_size_);
        Add("BlockSize", block_size_);
    }

    virtual ~CreateCtrTest() throw() {}

    virtual TestReplayParams* createTestStep(StringRef name) const
    {
        return new TaskReplay();
    }


    virtual void Replay(ostream& out, TestReplayParams* step_params)
    {
    }

    virtual void Run(ostream& out)
    {
        DefaultLogHandlerImpl logHandler(out);

        CreateCtrTest* task_params = this;

        TaskReplay params;

        params.size_ = task_params->size_;
        if (task_params->btree_random_branching_)
        {
            task_params->btree_branching_ = 8 + getRandom(100);
            out<<"BTree Branching: "<<task_params->btree_branching_<<endl;
        }

        Allocator allocator;
        allocator.getLogger()->setHandler(&logHandler);

        MapCtrType map(&allocator);

        map.setBranchingFactor(100);

        params.map_name_ = map.name();

        BigInt t00 = getTimeInMillis();

        for (Int c = 0; c < task_params->map_size_; c++)
        {
            map[getRandom()] = getRandom();
        }

        VectorMapCtrType vector_map(&allocator);

        vector_map.setBranchingFactor(100);

        params.vector_map_name_ = vector_map.name();

        for (Int c = 0; c < task_params->vector_map_size_; c++)
        {
            vector_map[getRandom()] = createBuffer(getRandom(task_params->block_size_), getRandom(256));
        }

        allocator.commit();

        BigInt t0 = getTimeInMillis();

        StoreAllocator(allocator, "alloc1.dump");

        BigInt t1 = getTimeInMillis();

        Allocator new_alloc;

        LoadAllocator(new_alloc, "alloc1.dump");

        BigInt t2 = getTimeInMillis();

        out<<"Store Time: "<<FormatTime(t1 - t0)<<endl;
        out<<"Load Time:  "<<FormatTime(t2 - t1)<<endl;

        MapCtrType new_map(&new_alloc, map.name());

        MEMORIA_TEST_THROW_IF(map.getBranchingFactor(), !=, new_map.getBranchingFactor());

        MEMORIA_TEST_THROW_IF(map.getSize(), !=, new_map.getSize());

        auto new_iter = new_map.Begin();

        for (auto iter = map.Begin(); !iter.isEnd(); iter.nextKey(), new_iter.nextKey())
        {
            MEMORIA_TEST_THROW_IF(iter.getKey(0), !=, new_iter.getKey(0));
            MEMORIA_TEST_THROW_IF(iter.getValue(), !=, new_iter.getValue());
        }

        BigInt t22 = getTimeInMillis();

        VectorMapCtrType new_vector_map(&new_alloc, vector_map.name());

        MEMORIA_TEST_THROW_IF(vector_map.getBranchingFactor(), !=, new_vector_map.getBranchingFactor());

        auto new_vm_iter = new_vector_map.Begin();

        for (auto iter = vector_map.Begin(); iter.isNotEnd(); iter.next(), new_vm_iter.next())
        {
            MEMORIA_TEST_THROW_IF(iter.size(), !=, new_vm_iter.size());

            ArrayData data = iter.read();

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

