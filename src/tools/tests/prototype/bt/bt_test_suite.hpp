
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once


#include "../../tests_inc.hpp"

#include "bt_core_test.hpp"


namespace memoria {
namespace v1 {

using namespace std;

class BTTestSuite: public TestSuite {

public:

    BTTestSuite(): TestSuite("BT.Suite")
    {
        //registerTask(new BTCoreTest<Table<BigInt, Byte>>("Core"));
    }
};

}}