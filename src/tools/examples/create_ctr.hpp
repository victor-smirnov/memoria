// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_KV_MAP_TASK_HPP_
#define MEMORIA_TESTS_KV_MAP_TASK_HPP_

#include "examples.hpp"

#include <vector>
#include <algorithm>
#include <sstream>
#include <memory>

namespace memoria {


class CreateCtrExample: public SPExampleTask {
public:
    typedef KVPair<BigInt, BigInt> Pair;

private:
    typedef vector<Pair> PairVector;
    typedef SmallCtrTypeFactory::Factory<Map1Ctr>::Type                 MapCtrType;
    typedef typename MapCtrType::Iterator                               Iterator;
    typedef typename MapCtrType::ID                                     ID;
    typedef typename MapCtrType::Accumulator                            Accumulator;

    PairVector pairs;
    PairVector pairs_sorted;

public:

    CreateCtrExample() :
        SPExampleTask("CreateCtr")
    {
        SmallCtrTypeFactory::Factory<RootCtr>::Type::initMetadata();
        SmallCtrTypeFactory::Factory<Map1Ctr>::Type::initMetadata();
    }

    virtual ~CreateCtrExample() throw () {
    }


    virtual void Run(ostream& out)
    {
        DefaultLogHandlerImpl logHandler(out);


        if (this->btree_random_airity_)
        {
            this->btree_branching_ = 8 + getRandom(100);
            out<<"BTree Branching: "<<this->btree_branching_<<endl;
        }

        Int SIZE = this->size_;

        Allocator allocator;
        allocator.getLogger()->setHandler(&logHandler);

        MapCtrType map(&allocator);

        map.setBranchingFactor(this->btree_branching_);

        for (Int c = 0; c < SIZE; c++)
        {
            map[c] = c;
        }

        allocator.commit();

        BigInt t0 = getTimeInMillis();
        StoreAllocator(allocator, "/dev/null");
        BigInt t1 = getTimeInMillis();

        out<<"Store Time: "<<FormatTime(t1 - t0)<<endl;
    }
};

}

#endif
