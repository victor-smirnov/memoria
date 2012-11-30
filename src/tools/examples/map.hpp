// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_EXAMPLES_MAP_HPP_
#define MEMORIA_EXAMPLES_MAP_HPP_

#include "examples.hpp"

#include <vector>
#include <algorithm>
#include <sstream>
#include <memory>

namespace memoria {




class MapExample: public SPExampleTask {
public:
    typedef KVPair<BigInt, BigInt> Pair;

private:
    typedef vector<Pair> PairVector;
    typedef SmallCtrTypeFactory::Factory<Map1>::Type                MapCtrType;
    typedef typename MapCtrType::Iterator                               Iterator;
    typedef typename MapCtrType::ID                                     ID;


    PairVector pairs;
    PairVector pairs_sorted;

public:

    MapExample() :
        SPExampleTask("Map")
    {
        MapCtrType::initMetadata();
    }

    virtual ~MapExample() throw () {
    }


    virtual void Run(ostream& out)
    {
        DefaultLogHandlerImpl logHandler(out);


        {
            if (this->btree_random_airity_)
            {
                this->btree_branching_ = 8 + getRandom(100);
                out<<"BTree Branching: "<<this->btree_branching_<<endl;
            }

            Allocator allocator;
            allocator.getLogger()->setHandler(&logHandler);

            MapCtrType map(allocator, 1, true);


            for (Int c = 0; c < 10; c++)
            {
                map[c] = c + 1;
            }

            allocator.commit();

            for (auto iter = map.begin(); iter != map.endm(); iter++)
            {
                out<<iter.key()<<" => "<<iter.value()<<endl;
            }


            auto iter1 = map.begin();
            auto iter2 = iter1;

            if (iter1 == iter2)
            {
                out<<"Iterators are equal"<<endl;
            }

            for (auto& val: map)
            {
                out<<val.key()<<" => "<<(BigInt)val<<endl;
            }

            BigInt sum = 0;
            for (auto& val: map)
            {
                sum += val;
            }

            out<<"Sum="<<sum<<endl;
        }
    }
};

}

#endif
