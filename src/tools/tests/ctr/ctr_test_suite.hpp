
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "../tests_inc.hpp"

#include "create_ctr_test.hpp"

#include <vector>

namespace memoria {
namespace v1 {

using namespace std;

class CtrTestSuite: public TestSuite {

public:

    CtrTestSuite(): TestSuite("CtrSuite")
    {
        registerTask(new CreateCtrTest());
    }

};

}}