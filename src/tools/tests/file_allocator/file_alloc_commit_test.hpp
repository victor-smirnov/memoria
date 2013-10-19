
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_TESTS_FILEALLOC_COMMIT_TEST_HPP_
#define MEMORIA_TESTS_FILEALLOC_COMMIT_TEST_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/allocators/file/factory.hpp>

#include "file_alloc_test_base.hpp"

namespace memoria {

class FileAllocatorCommitTest: public FileAllocatorTestBase {
	typedef FileAllocatorTestBase												Base;
	typedef FileAllocatorCommitTest												MyType;

protected:
	typedef typename Base::Allocator											Allocator;

	typedef typename FCtrTF<Vector<Int>>::Type									Ctr;

public:

	FileAllocatorCommitTest(StringRef name): Base(name)
	{
		Ctr::initMetadata();

		MEMORIA_ADD_TEST(testCleanStatus);
	}

	void testCleanStatus()
	{
		String name = getResourcePath("clean_status.db");
		Allocator allocator(name, OpenMode::READ | OpenMode::WRITE | OpenMode::CREATE | OpenMode::TRUNC);

		AssertTrue(MA_SRC, allocator.is_clean());

		Ctr ctr1(&allocator, CTR_CREATE, 10000);

		AssertFalse(MA_SRC, allocator.is_clean());

		allocator.commit();

		AssertTrue(MA_SRC, allocator.is_clean());

		AssertThrows<Exception>(MA_SRC, [&allocator]() {
			Ctr ctr(&allocator, CTR_CREATE | CTR_THROW_IF_EXISTS, 10000);
		});

		Ctr ctr2(&allocator, CTR_CREATE, 10001);

		allocator.rollback();

		AssertTrue(MA_SRC, allocator.is_clean());

		AssertFalse(MA_SRC, allocator.getRootID(10001).isSet());
	}

	void testUpdateLogOverflow()
	{
		String name = getResourcePath("update_log_overflow.db");
		Allocator allocator(name, OpenMode::READ | OpenMode::WRITE | OpenMode::CREATE | OpenMode::TRUNC);

		Ctr ctr(&allocator, CTR_CREATE, 10000);

		vector<Int> data(2000000);

		AssertThrows<Exception>(MA_SRC, [&ctr, &data]() {
			ctr.seek(0).insert(data);
		});
	}

};

}


#endif
