
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/tools/tests.hpp>
#include <memoria/v1/tools/tools.hpp>

#include <memoria/v1/core/tools/louds_tree.hpp>

#include "staticlouds_test_base.hpp"

#include <memory>

namespace memoria {

using namespace std;

class StaticLoudsMiscTest: public StaticLoudsTestBase {

    typedef StaticLoudsMiscTest                                                 MyType;

public:

    StaticLoudsMiscTest(): StaticLoudsTestBase("Misc")
    {}

    virtual ~StaticLoudsMiscTest() throw() {}
};


}
