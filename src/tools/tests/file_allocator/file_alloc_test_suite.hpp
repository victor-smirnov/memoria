
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_TESTS_FILEALLOC_TEST_SUITE_HPP_
#define MEMORIA_TESTS_FILEALLOC_TEST_SUITE_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/allocators/file/factory.hpp>

#include "file_alloc_create_test.hpp"
#include "file_alloc_commit_test.hpp"

namespace memoria {

struct FileAllocatorTestSuite: public TestSuite {

	FileAllocatorTestSuite(): TestSuite("FileAllocatorSuite")
    {
        registerTask(new FileAllocatorCreateTest("Create"));
        registerTask(new FileAllocatorCommitTest("Commit"));
    }

};

}


#endif
