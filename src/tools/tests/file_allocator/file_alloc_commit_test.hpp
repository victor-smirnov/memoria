
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
		MEMORIA_ADD_TEST(testUpdateLogOverflow);
		MEMORIA_ADD_TEST(testFileSizeAfterRollback);

		MEMORIA_ADD_TEST(testRecoverNewDB);
		MEMORIA_ADD_TEST(testRecoverCleanDB);

		MEMORIA_ADD_TEST(testRecoverFromFailureBeforeBackup);
		MEMORIA_ADD_TEST(testRecoverFromFailureBeforeCommit);
	}

	void testCleanStatus()
	{
		String name = getResourcePath("clean_status.db");
		Allocator allocator(name, OpenMode::RWCT);

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
		Allocator allocator(name, OpenMode::RWCT);

		Ctr ctr(&allocator, CTR_CREATE, 10000);

		vector<Int> data(2000000);

		AssertThrows<Exception>(MA_SRC, [&ctr, &data]() {
			ctr.seek(0).insert(data);
		});
	}


	void testFileSizeAfterRollback()
	{
		String name = getResourcePath("rollback_file_size.db");
		Allocator allocator(name, OpenMode::RWCT);

		allocator.commit();

		UBigInt file_size = allocator.file_size();

		AssertEQ(MA_SRC, file_size, allocator.superblock()->file_size());

		Ctr ctr(&allocator, CTR_CREATE, 10000);

		vector<Int> data(20000);

		ctr.seek(0).insert(data);

		allocator.rollback();

		UBigInt new_file_size = allocator.file_size();

		AssertEQ(MA_SRC, new_file_size, allocator.superblock()->file_size());
		AssertEQ(MA_SRC, new_file_size, file_size);
	}

	void testRecoverNewDB()
	{
		String name = getResourcePath("new.db");
		Allocator allocator(name, OpenMode::RWCT);

		allocator.commit();
		allocator.close();

		AssertEQ(MA_SRC, (Int)Allocator::recover(name), (Int)Allocator::RecoveryStatus::CLEAN);

		AssertDoesntThrow(MA_SRC, [name]() {
			Allocator allocator2(name, OpenMode::RWCT);
		});
	}

	void testRecoverCleanDB()
	{
		String name = getResourcePath("clean.db");
		Allocator allocator(name, OpenMode::RWCT);

		Ctr ctr(&allocator, CTR_CREATE, 10000);

		vector<Int> data(20000);

		ctr.seek(0).insert(data);

		allocator.commit();
		allocator.close();

		AssertEQ(MA_SRC, (Int)Allocator::recover(name), (Int)Allocator::RecoveryStatus::CLEAN);

		AssertDoesntThrow(MA_SRC, [name]() {
			Allocator allocator2(name, OpenMode::RWCT);
		});
	}

	void testRecoverFromFailureBeforeBackup()
	{
		String name = getResourcePath("failure_before_log.db");
		Allocator allocator(name, OpenMode::RWCT);

		Ctr ctr(&allocator, CTR_CREATE, 10000);

		vector<Int> data(20000);

		ctr.seek(0).insert(data);

		allocator.failure() = Allocator::FAILURE_BEFORE_BACKUP;

		AssertThrows<Exception>(MA_SRC, [&allocator]() {
			allocator.commit();
		});

		allocator.close(false);

		AssertEQ(MA_SRC, (Int)Allocator::recover(name), (Int)Allocator::RecoveryStatus::FILE_SIZE);

		AssertDoesntThrow(MA_SRC, [name]() {
			Allocator allocator2(name, OpenMode::RWCT);

			//AssertEQ(MA_SRC, allocator2.file_size(), )
		});
	}


	void testRecoverFromFailureBeforeCommit()
	{
		String name = getResourcePath("failure_before_commit.db");
		Allocator allocator(name, OpenMode::RWCT);

		Ctr ctr(&allocator, CTR_CREATE, 10000);

		vector<Int> data(200000);

		ctr.seek(0).insert(data);

		allocator.commit();

		ctr.seek(0).insert(data);

		allocator.failure() = Allocator::FAILURE_BEFORE_COMMIT;

		AssertThrows<Exception>(MA_SRC, [&allocator]() {
			allocator.commit();
		});

		allocator.close(false);

		AssertEQ(MA_SRC, (Int)Allocator::recover(name), (Int)Allocator::RecoveryStatus::LOG);

		AssertDoesntThrow(MA_SRC, [name]() {
			Allocator allocator2(name, OpenMode::RWC);

			Ctr ctr(&allocator2, CTR_FIND, 10000);

			AssertEQ(MA_SRC, ctr.size(), 200000);
		});
	}
};

}


#endif
