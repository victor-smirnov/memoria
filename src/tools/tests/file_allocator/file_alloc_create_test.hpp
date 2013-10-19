
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_TESTS_FILEALLOC_CREATE_TEST_HPP_
#define MEMORIA_TESTS_FILEALLOC_CREATE_TEST_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/allocators/file/factory.hpp>

#include "file_alloc_test_base.hpp"

namespace memoria {

class FileAllocatorCreateTest: public FileAllocatorTestBase {
	typedef FileAllocatorTestBase												Base;
	typedef FileAllocatorCreateTest												MyType;

protected:
	typedef typename Base::Allocator											Allocator;

	typedef typename FCtrTF<Map<BigInt, BigInt>>::Type							MapCtr;

public:

	FileAllocatorCreateTest(StringRef name): Base(name)
	{
		MapCtr::initMetadata();

		MEMORIA_ADD_TEST(testOpenMode);
//		MEMORIA_ADD_TEST(testAttributes);
//		MEMORIA_ADD_TEST(testInitialFileSize);
	}

	void testOpenMode()
	{
		File db_file(getResourcePath("open_mode.db"));

		if (db_file.isExists())
		{
			db_file.deleteFile();
		}

		AssertDoesntThrow(MA_SRC, [&db_file]()
		{
			Allocator allocator(db_file.getPath(), OpenMode::READ | OpenMode::WRITE | OpenMode::CREATE);
		});

		AssertDoesntThrow(MA_SRC, [&db_file]()
		{
			Allocator allocator(db_file.getPath(), OpenMode::READ | OpenMode::WRITE);
		});

		AssertThrows<Exception>(MA_SRC, [&db_file]()
		{
			Allocator allocator(db_file.getPath(), OpenMode::READ);

			auto page = allocator.createPage(allocator.properties().defaultPageSize());

			allocator.commit();
		});

		AssertDoesntThrow(MA_SRC, [&db_file]()
		{
			Allocator allocator(db_file.getPath(), OpenMode::READ);

			auto page = allocator.createPage(allocator.properties().defaultPageSize());

			allocator.rollback();
		});


		AssertThrows<Exception>(MA_SRC, [&db_file]()
		{
			Allocator allocator(db_file.getPath(), OpenMode::READ | OpenMode::CREATE);
		});

		AssertDoesntThrow(MA_SRC, [&db_file]()
		{
			Allocator allocator(db_file.getPath(), OpenMode::READ | OpenMode::WRITE | OpenMode::TRUNC);
		});

		db_file.deleteFile();

		AssertDoesntThrow(MA_SRC, [&db_file]()
		{
			Allocator allocator(db_file.getPath(), OpenMode::READ | OpenMode::WRITE | OpenMode::CREATE);
			MapCtr map(&allocator, CTR_CREATE, 10000);

			allocator.commit();
		});

		AssertDoesntThrow(MA_SRC, [&db_file]()
		{
			Allocator allocator(db_file.getPath(), OpenMode::READ | OpenMode::WRITE | OpenMode::TRUNC);
			MapCtr map(&allocator, CTR_CREATE | CTR_THROW_IF_EXISTS, 10000);

			allocator.commit();
		});
	}

	void testAttributes()
	{
		String name = getResourcePath("attributes.db");

		Allocator allocator(name, OpenMode::READ | OpenMode::WRITE | OpenMode::CREATE | OpenMode::TRUNC);

		AssertTrue(MA_SRC, allocator.is_new());
		AssertTrue(MA_SRC, allocator.is_clean());

		allocator.close();

		Allocator new_allocator(name, OpenMode::READ | OpenMode::WRITE);

		AssertFalse(MA_SRC, new_allocator.is_new());
		AssertTrue(MA_SRC, new_allocator.is_clean());
	}

	void testInitialFileSize()
	{
		String name = getResourcePath("initial_file_size.db");

		Allocator allocator(name, OpenMode::READ | OpenMode::WRITE | OpenMode::CREATE | OpenMode::TRUNC);

		File file(name);

		Int page_size 	= allocator.properties().defaultPageSize();
		Int batch_size 	= allocator.allocation_batch_size();

		AssertEQ(MA_SRC, file.size(), batch_size * page_size);
	}
};

}


#endif
