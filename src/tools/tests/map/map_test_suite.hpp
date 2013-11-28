
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_CTR_MAP_TEST_SUITE_HPP_
#define MEMORIA_TESTS_CTR_MAP_TEST_SUITE_HPP_


#include "../tests_inc.hpp"

//#include "map_api_test.hpp"
#include "map_create_test.hpp"
#include "map_remove_test.hpp"
#include "map_select_test.hpp"
//#include "map_batch_test.hpp"

#include <vector>

namespace memoria {

using namespace memoria::vapi;
using namespace std;

class MapTestSuite: public TestSuite {

public:

    MapTestSuite(): TestSuite("MapSuite")
    {
//    	registerTask(new MapCreateTest<SMrkMap<BigInt, BigInt, 1>>("SMrkMap.Create"));
//
//    	registerTask(new MapCreateTest<MrkMap<BigInt, BigInt, 1>>("MrkMap.Create"));
//
//        registerTask(new MapCreateTest<Map<BigInt, BigInt>>("Map.Create"));
        registerTask(new MapCreateTest<CMap<Granularity::Bit>>("CMap1.Create"));
        registerTask(new MapCreateTest<CMap<Granularity::Byte>>("CMap2.Create"));
//
//        registerTask(new MapRemoveTest<Map<BigInt, BigInt>>("Map.Remove"));
//
//    	registerTask(new MapRemoveTest<SMrkMap<BigInt, BigInt, 1>>("SMrkMap.Remove"));
//    	registerTask(new MapRemoveTest<MrkMap<BigInt, BigInt, 1>>("MrkMap.Remove"));
//
        registerTask(new MapRemoveTest<CMap<Granularity::Bit>>("CMap1.Remove"));
        registerTask(new MapRemoveTest<CMap<Granularity::Byte>>("CMap2.Remove"));
//
//    	registerTask(new MapSelectTest<SMrkMap<BigInt, BigInt, 1>>("SMrkMap.Select"));
    }

};

}


#endif

