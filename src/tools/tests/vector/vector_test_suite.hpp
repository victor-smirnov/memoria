
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/core/types/types.hpp>

#include "vector_test.hpp"

namespace memoria {
namespace v1 {

using namespace std;

class VectorTestSuite: public TestSuite {

public:

    VectorTestSuite(): TestSuite("Vector")
    {
        registerTask(new VectorTest<Vector<Int>>("Int.FX"));
        registerTask(new VectorTest<Vector<VLen<Granularity::Byte>>>("Int.VL.Byte"));
        registerTask(new VectorTest<Vector<VLen<Granularity::Bit>>>("Int.VL.Bit"));
    }
};

}}