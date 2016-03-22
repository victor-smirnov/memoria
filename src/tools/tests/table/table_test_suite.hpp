
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once


#include "../tests_inc.hpp"

#include "table_core_test.hpp"


namespace memoria {

using namespace std;

class TableTestSuite: public TestSuite {

public:

    TableTestSuite(): TestSuite("TableSuite")
    {
//      registerTask(new TableCoreTest<Table<BigInt, Byte>>("Core"));
    }
};

}
