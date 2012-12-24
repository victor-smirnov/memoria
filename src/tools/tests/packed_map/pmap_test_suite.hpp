
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_CTR_PMAP_TEST_SUITE_HPP_
#define MEMORIA_TESTS_CTR_PMAP_TEST_SUITE_HPP_

#include "../tests_inc.hpp"

#include "pmap_data.hpp"
#include "pmap_find.hpp"
#include "pmap_reindex.hpp"
#include "pmap_transfer.hpp"
#include "pmap_sum.hpp"
#include "pmap_walk_bw.hpp"
#include "pmap_walk_fw.hpp"

#include <vector>

namespace memoria {

using namespace memoria::vapi;
using namespace std;

class PackedMapTestSuite: public TestSuite {

public:

    PackedMapTestSuite(): TestSuite("PackedMapSuite")
    {
        registerTask(new PMapDataTest());
        registerTask(new PMapFindTest());
        registerTask(new PMapReindexTest());

        registerTask(new PMapTransferTest<true>("TransferSameMaps"));
        registerTask(new PMapTransferTest<false>("TransferDifferentMaps"));

        registerTask(new PMapSumTest<2>());
        registerTask(new PMapSumTest<4>());
        registerTask(new PMapSumTest<8>());
        registerTask(new PMapSumTest<16>());
        registerTask(new PMapSumTest<32>());
        registerTask(new PMapSumTest<64>());

        registerTask(new PMapWalkFwTest<2>());
        registerTask(new PMapWalkFwTest<4>());
        registerTask(new PMapWalkFwTest<8>());
        registerTask(new PMapWalkFwTest<16>());
        registerTask(new PMapWalkFwTest<32>());
        registerTask(new PMapWalkFwTest<64>());
        registerTask(new PMapWalkFwTest<5>());
        registerTask(new PMapWalkFwTest<13>());
        registerTask(new PMapWalkFwTest<22>());

        registerTask(new PMapWalkBwTest<2>());
        registerTask(new PMapWalkBwTest<4>());
        registerTask(new PMapWalkBwTest<8>());
        registerTask(new PMapWalkBwTest<16>());
        registerTask(new PMapWalkBwTest<32>());
        registerTask(new PMapWalkBwTest<64>());
        registerTask(new PMapWalkBwTest<5>());
        registerTask(new PMapWalkBwTest<13>());
        registerTask(new PMapWalkBwTest<22>());
    }

};

}


#endif

