
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_STATICLOUDS_MISC_HPP_
#define MEMORIA_TESTS_STATICLOUDS_MISC_HPP_

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/core/tools/louds_tree.hpp>

#include "staticlouds_test_base.hpp"

#include <memory>

namespace memoria {

using namespace std;

class StaticLoudsMiscTest: public StaticLoudsTestBase {

    typedef StaticLoudsMiscTest 												MyType;



public:

    StaticLoudsMiscTest(): StaticLoudsTestBase("Misc")
    {

    }

    virtual ~StaticLoudsMiscTest() throw() {}



};


}


#endif
