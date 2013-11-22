

// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#include <memoria/memoria.hpp>

#include <memoria/containers/map2/map_factory.hpp>

#include <memoria/tools/tools.hpp>

#include <typeinfo>
#include <iostream>
#include <vector>
#include <type_traits>

using namespace std;
using namespace memoria;
using namespace memoria::vapi;

typedef SmallInMemAllocator                                                     Allocator;
typedef SCtrTF<Map2<BigInt, BigInt>>::Type                                      Map2Ctr;
typedef SCtrTF<Map<BigInt, BigInt>>::Type                                       Map1Ctr;



typedef typename Map2Ctr::ISubtreeProvider                       Map2ISubtreeProvider;
typedef typename Map2Ctr::DefaultSubtreeProviderBase             Map2DefaultSubtreeProviderBase;
typedef typename Map2Ctr::NonLeafNodeKeyValuePair                Map2NonLeafNodeKeyValuePair;
typedef typename Map2Ctr::LeafNodeKeyValuePair                   Map2LeafNodeKeyValuePair;
typedef typename Map2Ctr::Accumulator                            Map2Accumulator;
typedef typename Map2Ctr::Value                                  Map2Value;


template <typename CtrType>
class SubtreeProvider: public CtrType::DefaultSubtreeProviderBase
{
    typedef typename CtrType::ISubtreeProvider                       ISubtreeProvider;
    typedef typename CtrType::DefaultSubtreeProviderBase             DefaultSubtreeProviderBase;
    typedef typename CtrType::NonLeafNodeKeyValuePair                NonLeafNodeKeyValuePair;
    typedef typename CtrType::LeafNodeKeyValuePair                   LeafNodeKeyValuePair;
    typedef typename CtrType::Accumulator                            Accumulator;
    typedef typename CtrType::Value                                  Value;


    typedef DefaultSubtreeProviderBase          Base;

public:
    SubtreeProvider(CtrType* ctr, BigInt total): Base(*ctr, total) {}

    virtual LeafNodeKeyValuePair getLeafKVPair(BigInt begin)
    {
        Accumulator acc;
        acc[0] = 1;
        return LeafNodeKeyValuePair(acc, Value());
    }
};


int main(void) {

    MEMORIA_INIT(SmallProfile<>);

    Map2Ctr::initMetadata();

    Allocator allocator;

    BigInt name1 = 1;
    BigInt name2 = 2;

    if (!File("map3d.dump").isExists())
    {
        Map1Ctr map1(&allocator, CTR_CREATE, name1);
        Map2Ctr map2(&allocator, CTR_CREATE, name2);

        map1.setNewPageSize(4096);
        map2.setNewPageSize(4096);

        SubtreeProvider<Map1Ctr> provider1(&map1, 10000000);
        SubtreeProvider<Map2Ctr> provider2(&map2, 10000000);

        auto i1 = map1.Begin();
        map1.insertSubtree(i1, provider1);

        auto i2 = map2.Begin();
        map2.insertSubtree(i2, provider2);

        allocator.commit();

        FileOutputStreamHandlerImpl file("map3d.dump");
        allocator.store(&file);
    }
    else {
        FileInputStreamHandlerImpl file("map3d.dump");
        allocator.load(&file);

        Map1Ctr map1(&allocator, CTR_FIND, name1);
        Map2Ctr map2(&allocator, CTR_FIND, name2);

        Int max_reads = 1000000;

        vector<Int> indexes(max_reads);

        for (auto& v: indexes)
        {
            v = getRandom(map1.size());
        }

        BigInt t0 = getTimeInMillis();

        for (Int c = 0; c < max_reads; c++)
        {
            map1.findGE(indexes[c], 0)<<endl;
        }

        BigInt t1 = getTimeInMillis();

        BigInt t2 = getTimeInMillis();

        for (Int c = 0; c < max_reads; c++)
        {
            map2.findGE(indexes[c], 0)<<endl;
        }

        BigInt t3 = getTimeInMillis();

        cout<<"Map1 Random Read: "<<FormatTime(t1 - t0)<<endl;
        cout<<"Map2 Random Read: "<<FormatTime(t3 - t2)<<endl;
    }

    return 0;
}

