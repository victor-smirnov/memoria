// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_MAP_MAP_API_TEST_HPP_
#define MEMORIA_TESTS_MAP_MAP_API_TEST_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include "map_test_base.hpp"

#include <vector>
#include <algorithm>
#include <sstream>
#include <memory>

namespace memoria {

template <
    typename MapName
>
class MapApiTest: public MapTestBase<MapName> {

    typedef MapTestBase<MapName>                                                Base;
    typedef MapApiTest<MapName>                                                 MyType;

    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::Ctr                                                  Ctr;

public:

    MapApiTest(String name): Base(name)
    {
        MEMORIA_ADD_TEST(runTest);
    }

    virtual ~MapApiTest() throw () {}


    void runTest()
    {
    	DefaultLogHandlerImpl logHandler(Base::out());

        Allocator allocator;
        allocator.getLogger()->setHandler(&logHandler);

        Ctr map(&allocator);

        Base::ctr_name_ = map.name();


        auto iter1 = map[1];
        iter1 = std::make_tuple(1, 2);

        cout<<"First: "<<std::get<0>(iter1.value())<<endl;
        cout<<"Second: "<<std::get<1>(iter1.value())<<endl;

        cout<<"E.Key: "<<iter1.entry().key()<<endl;
        cout<<"E.First: "<<std::get<0>(iter1.entry().value())<<endl;
        cout<<"E.Second: "<<std::get<1>(iter1.entry().value())<<endl;

        allocator.commit();

        this->StoreAllocator(allocator, this->getResourcePath("api.dump"));
    }


};

}

#endif
