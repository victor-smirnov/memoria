

// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#include <memoria/memoria.hpp>

#include <memoria/containers/map2/map_factory.hpp>

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

    MapCtr map(&allocator);

    for (int c = 0; c < 10; c++)
    {
        map[c] = c + 1;
    }

    for (const auto& iter: map)
    {
        cout<<"Map: "<<iter.key()<<" "<<iter.value()<<endl;
    }

    cout<<endl;

    return 0;
}

