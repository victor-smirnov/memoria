// Copyright Victor Smirnov 2014.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_MAP_MAP_MAPX_TEST_HPP_
#define MEMORIA_TESTS_MAP_MAP_MAPX_TEST_HPP_

#include <memoria/containers/map/map_factory.hpp>
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
class MapXTest: public SPTestTask {

    using Base = SPTestTask;
    using MyType = MapXTest<MapName>;


    typedef typename DCtrTF<MapName>::Type Ctr;
    typedef typename Ctr::Iterator Iterator;
    typedef typename Ctr::ID ID;
    typedef typename Ctr::BranchNodeEntry BranchNodeEntry;


//    typedef typename Base::Allocator                                            Allocator;
//    typedef typename Base::Ctr                                                  Ctr;

    Int key_        = 0;
    Int skip_bw_    = 0;
    Int select_bw_  = 0;

public:

    MapXTest(String name): Base(name)
    {
        MEMORIA_ADD_TEST_PARAM(key_);
        MEMORIA_ADD_TEST_PARAM(skip_bw_);
        MEMORIA_ADD_TEST_PARAM(select_bw_);

        MEMORIA_ADD_TEST(runTest);
    }

    virtual ~MapXTest() throw () {}


    void runTest()
    {
        DefaultLogHandlerImpl logHandler(Base::out());

        Allocator allocator;
        allocator.getLogger()->setHandler(&logHandler);

        Ctr map(&allocator);

        auto iter = map.Begin();

//        auto entry = std::make_tuple((BigInt)1, (BigInt)2);

//        map.insertEntry(iter, entry);

//        Base::ctr_name_ = map.name();
//
//        BigInt last_key = 0;
//
//        for (Int c = 1; c <= 1000; c++)
//        {
//            last_key += getRandom(5) + 1;
//
//            auto iter = map[last_key];
//
//            iter = 1234;
//        }
//
//        auto begin  = map.Begin();
//        auto end    = map.RBegin();
//
//        begin.removeTo(end);

        allocator.commit();

        this->StoreAllocator(allocator, this->getResourcePath("api.dump"));
    }


};

}

#endif
