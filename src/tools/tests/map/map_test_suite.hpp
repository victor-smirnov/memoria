
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_CTR_MAP_TEST_SUITE_HPP_
#define MEMORIA_TESTS_CTR_MAP_TEST_SUITE_HPP_

#include "../shared/params.hpp"
#include "../tests_inc.hpp"

#include "map_test.hpp"

#include <vector>

namespace memoria {

using namespace memoria::vapi;
using namespace std;

class MapTestSuite: public TestSuite {

public:

	MapTestSuite(): TestSuite("MapTestSuite")
	{
		registerTask(new MapTest());
	}

};

}


#endif

