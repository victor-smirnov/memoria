
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_TESTS_MVCC_TEST_SUITE_HPP_
#define MEMORIA_TESTS_MVCC_TEST_SUITE_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/allocators/file/factory.hpp>
#include <memoria/allocators/mvcc/mvcc_allocator.hpp>

#include "mvcc_create_test.hpp"

namespace memoria {

struct MVCCTestSuite: public TestSuite {
	MVCCTestSuite(): TestSuite("MVCCSuite")
    {
        registerTask(new MVCCCreateTest("Create"));
    }
};

}


#endif
