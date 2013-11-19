
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

#include "../dump.hpp"

namespace memoria {

class MVCCCreateTest: public TestTask {
	typedef TestTask															Base;
	typedef MVCCCreateTest														MyType;

protected:

	typedef GenericFileAllocator												Allocator;

	typedef MVCCAllocator<FileProfile<>, Allocator::Page>						TxnMgr;
	typedef typename TxnMgr::TxnPtr												TxnPtr;

	typedef CtrTF<FileProfile<>, Vector<Int>>::Type								VectorCtr;

	typedef typename Allocator::ID												ID;

	typedef typename CtrTF<FileProfile<>, DblMrkMap2<BigInt, ID, 2>>::Type		Map2Ctr;

public:

	MVCCCreateTest(StringRef name): Base(name)
	{
		MetadataRepository<FileProfile<>>::init();
		Allocator::initMetadata();
		TxnMgr::initMetadata();
		VectorCtr::initMetadata();

		MEMORIA_ADD_TEST(testCreate);
//		MEMORIA_ADD_TEST(testSingleTxn);
//		MEMORIA_ADD_TEST(testDoubleTxn);
	}


	void testCreate()
	{
		String name = getResourcePath("create.db");
		Allocator allocator(name, OpenMode::RWCT);

		TxnMgr mgr(&allocator);

		VectorCtr ctr(&mgr, CTR_CREATE);

		BigInt ctr_name = ctr.name();

		vector<Int> data(10000);

		ctr.seek(0).insert(data);
		ctr.seek(0).insert(data);
		ctr.seek(0).insert(data);

		allocator.flush();

		mgr.dumpAllocator(this->getResourcePath("mvcc"));

		allocator.close();

		Allocator allocator2(name, OpenMode::RW);

		TxnMgr mgr2(&allocator2);

		assertCtrContent(mgr2, ctr_name, 30000);
	}

	void testCreate1()
	{
		String name = getResourcePath("create.db");
		Allocator allocator(name, OpenMode::RWCT);

		Map2Ctr ctr(&allocator, CTR_CREATE);

		for (int c = 1; c <= 3; c++)
		{
			auto iter = ctr.create(c);

			int max = c == 2 ? 2000 : 20;

			for (int d = 1; d < max; d++)
			{
				iter.insert(d, ID(d));
			}
		}


		allocator.flush();

	}

	void testSingleTxn()
	{
		String name = getResourcePath("single_txn.db");
		Allocator allocator(name, OpenMode::RWCT);

		TxnMgr mgr(&allocator);

		auto txn = mgr.begin();

		BigInt ctr_name = createCtr(txn, 2000).name();

		txn->commit();

		mgr.flush();

		allocator.close();

		Allocator allocator2(name, OpenMode::RW);

		TxnMgr mgr2(&allocator2);

		auto txn2 = mgr2.begin();

		assertCtrContent(txn2, ctr_name, 2000);
	}


	void testDoubleTxn()
	{
		String name = getResourcePath("double_txn.db");
		Allocator allocator(name, OpenMode::RWCT);

		TxnMgr mgr(&allocator);

		auto txn1 = mgr.begin();
		auto txn2 = mgr.begin();

		BigInt ctr1_name = createCtr(txn1, 2000).name();

		txn1->commit();

		BigInt ctr2_name = createCtr(txn2, 4000).name();

		txn2->commit();

		assertCtrContent(mgr, ctr1_name, 2000);
		assertCtrContent(mgr, ctr2_name, 4000);


		allocator.close();

		Allocator allocator2(name, OpenMode::RW);

		TxnMgr mgr2(&allocator2);

		assertCtrContent(mgr2, ctr1_name, 2000);
		assertCtrContent(mgr2, ctr2_name, 4000);
	}

	VectorCtr createCtr(TxnPtr& txn, BigInt size)
	{
		vector<Int> data(size);

		VectorCtr ctr(txn.get(), CTR_CREATE);
		ctr.seek(0).insert(data);

		return ctr;
	}

	VectorCtr createCtr(TxnPtr& txn)
	{
		VectorCtr ctr(txn.get(), CTR_CREATE);
		return ctr;
	}

	VectorCtr createCtr(TxnMgr& mgr)
	{
		VectorCtr ctr(&mgr, CTR_CREATE);
		return ctr;
	}



	void assertCtrContent(TxnMgr& mgr, BigInt name, BigInt size)
	{
		VectorCtr ctr(&mgr, CTR_FIND, name);
		AssertEQ(MA_SRC, ctr.size(), size);

		vector<Int> v = ctr.seek(0).subVector(size);
		AssertEQ(MA_SRC, v.size(), (size_t)size);
	}

	void assertCtrContent(Allocator& mgr, BigInt name, BigInt size)
	{
		VectorCtr ctr(&mgr, CTR_FIND, name);
		AssertEQ(MA_SRC, ctr.size(), size);

		vector<Int> v = ctr.seek(0).subVector(size);
		AssertEQ(MA_SRC, v.size(), (size_t)size);
	}


	void assertCtrContent(TxnPtr& txn, BigInt name, BigInt size)
	{
		VectorCtr ctr(txn.get(), CTR_FIND, name);
		AssertEQ(MA_SRC, ctr.size(), size);

		vector<Int> v = ctr.seek(0).subVector(size);
		AssertEQ(MA_SRC, v.size(), (size_t)size);
	}

	void dumpStat(Allocator& file_allocator)
	{
		out()<<"FileAllocator Stat: "
				<<" "<<file_allocator.shared_pool_size()
				<<" "<<file_allocator.shared_created()
				<<" "<<file_allocator.shared_deleted()
				<<endl;
	}

};

}


#endif
