
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_CTR_PMAP_TEST_SUITE_HPP_
#define MEMORIA_TESTS_CTR_PMAP_TEST_SUITE_HPP_

#include "../shared/params.hpp"
#include "../tests_inc.hpp"

#include "pmap_data.hpp"
#include "pmap_find.hpp"
#include "pmap_reindex.hpp"
#include "pmap_sum.hpp"
#include "pmap_walk_bw.hpp"
#include "pmap_walk_fw.hpp"

#include <vector>

namespace memoria {

using namespace memoria::vapi;
using namespace std;

class PackedMapTestSuite: public TestSuite {

public:

	PackedMapTestSuite(): TestSuite("PackedMapTestSuite")
	{
		RegisterTask(new PMapDataTest());
		RegisterTask(new PMapfindTest());
		RegisterTask(new PMapReindexTest());

		RegisterTask(new PMapSumTest<2>());
		RegisterTask(new PMapSumTest<4>());
		RegisterTask(new PMapSumTest<8>());
		RegisterTask(new PMapSumTest<16>());
		RegisterTask(new PMapSumTest<32>());
		RegisterTask(new PMapSumTest<64>());

		RegisterTask(new PMapWalkFwTest<2>());
		RegisterTask(new PMapWalkFwTest<4>());
		RegisterTask(new PMapWalkFwTest<8>());
		RegisterTask(new PMapWalkFwTest<16>());
		RegisterTask(new PMapWalkFwTest<32>());
		RegisterTask(new PMapWalkFwTest<64>());
		RegisterTask(new PMapWalkFwTest<5>());
		RegisterTask(new PMapWalkFwTest<13>());
		RegisterTask(new PMapWalkFwTest<22>());

		RegisterTask(new PMapWalkBwTest<2>());
		RegisterTask(new PMapWalkBwTest<4>());
		RegisterTask(new PMapWalkBwTest<8>());
		RegisterTask(new PMapWalkBwTest<16>());
		RegisterTask(new PMapWalkBwTest<32>());
		RegisterTask(new PMapWalkBwTest<64>());
		RegisterTask(new PMapWalkBwTest<5>());
		RegisterTask(new PMapWalkBwTest<13>());
		RegisterTask(new PMapWalkBwTest<22>());
	}

};

}


#endif

