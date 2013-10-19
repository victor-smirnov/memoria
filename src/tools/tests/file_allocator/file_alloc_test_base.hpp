
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_TESTS_FILEALLOC_TEST_BASE_HPP_
#define MEMORIA_TESTS_FILEALLOC_TEST_BASE_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/allocators/file/factory.hpp>

namespace memoria {

class FileAllocatorTestBase: public TestTask {
protected:
	typedef GenericFileAllocator												Allocator;

public:

	FileAllocatorTestBase(StringRef name): TestTask(name)
	{
		MetadataRepository<FileProfile<> >::init();
	}



};

}


#endif
