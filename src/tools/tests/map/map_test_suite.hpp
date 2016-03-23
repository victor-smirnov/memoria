
// Copyright Victor Smirnov 2012-2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once


#include "../tests_inc.hpp"

#include "map_create_test.hpp"
#include "map_remove_test.hpp"

#include <vector>

namespace memoria {
namespace v1 {

using namespace std;

class MapTestSuite: public TestSuite {

public:

    MapTestSuite(): TestSuite("MapSuite")
    {
        registerTask(new MapRemoveTest<Map<UUID, BigInt>>("MapM.Remove"));
        registerTask(new MapCreateTest<Map<UUID, BigInt>>("MapM.Create"));
    }

};

}}