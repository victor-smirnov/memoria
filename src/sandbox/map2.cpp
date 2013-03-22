

// Copyright Victor Smirnov 2012.
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
typedef SCtrTF<Map2<BigInt, BigInt>>::Type                                      MapCtr;


int main(void) {

    MEMORIA_INIT(SmallProfile<>);

    Allocator allocator;

    MapCtr::initMetadata();

    MapCtr map(&allocator);

    BigInt t0 = getTimeInMillis();

    for (int c = 0; c < 1000000; c++)
    {
        map[c] = c + 1;
    }

    BigInt t1 = getTimeInMillis();

    BigInt cnt = 0;

    for (const auto& iter: map)
    {
        //cout<<"Map: "<<iter.key()<<" "<<iter.value()<<endl;

        cnt += iter.key() + iter.value();
    }

    BigInt t2 = getTimeInMillis();

    allocator.commit();

    FileOutputStreamHandlerImpl file("map2d.dump");


    BigInt t3 = getTimeInMillis();

    allocator.store(&file);

    BigInt t4 = getTimeInMillis();

    cout<<"Create: "<<FormatTime(t1 - t0)<<" Read: "<<(t2 - t1)<<" Store: "<<FormatTime(t4 - t3)<<" Result: "<<cnt<<endl;

    cout<<endl;

    return 0;
}

