
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_TESTS_MVCC_CREATE_TEST_HPP_
#define MEMORIA_TESTS_MVCC_TEST_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/allocators/file/factory.hpp>
#include <memoria/allocators/mvcc/mvcc_allocator.hpp>



namespace memoria {

class MVCCCreateTest: public TestTask {
	typedef TestTask															Base;
	typedef MVCCCreateTest														MyType;

protected:

	typedef GenericFileAllocator												Allocator;

	typedef MVCCAllocator<FileProfile<>, Allocator::Page>						TxnMgr;

	typedef CtrTF<FileProfile<>, Vector<Int>>::Type								VectorCtr;

public:

	MVCCCreateTest(StringRef name): Base(name)
	{
		MetadataRepository<FileProfile<>>::init();
		Allocator::initMetadata();
		TxnMgr::initMetadata();
		VectorCtr::initMetadata();

		MEMORIA_ADD_TEST(testCreate);
	}


	void testCreate()
	{
		String name = getResourcePath("create.db");
		Allocator allocator(name, OpenMode::RWCT);

		TxnMgr mgr(&allocator);

		allocator.commit();

		VectorCtr ctr(&mgr, CTR_CREATE);

		BigInt ctr_name = ctr.name();

		vector<Int> data(10000);

		ctr.seek(0).insert(data);
		ctr.seek(0).insert(data);
		ctr.seek(0).insert(data);

		allocator.commit();

		allocator.close();


		Allocator allocator2(name, OpenMode::RW);

		TxnMgr mgr2(&allocator2);

		VectorCtr ctr2(&mgr2, CTR_FIND, ctr_name);

		AssertEQ(MA_SRC, ctr2.size(), 30000);

		vector<Int> v = ctr2.seek(0).subVector(30000);

		AssertEQ(MA_SRC, v.size(), 30000ul);
	}
};

}


#endif
