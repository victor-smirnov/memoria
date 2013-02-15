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

class MapApiTest: public MapTestBase {

    typedef MapApiTest                                                       MyType;

public:

    MapApiTest(): MapTestBase("API")
    {
        MEMORIA_ADD_TEST(runTest);
    }

    virtual ~MapApiTest() throw () {}


    void runTest()
    {
        DefaultLogHandlerImpl logHandler(out());

        Allocator allocator;
        allocator.getLogger()->setHandler(&logHandler);

        Ctr map(&allocator);

        ctr_name_ = map.name();
    }


};

}

#endif
