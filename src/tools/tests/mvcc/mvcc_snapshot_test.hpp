
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_TESTS_MVCC_SNAPSHOT_TEST_HPP_
#define MEMORIA_TESTS_MVCC_SNAPSHOT_TEST_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/allocators/file/factory.hpp>
#include <memoria/allocators/mvcc/mvcc_allocator.hpp>

#include "../dump.hpp"

#include <vector>
#include <map>

namespace memoria {

class MVCCSnapshotTest: public TestTask {
	typedef TestTask															Base;
	typedef MVCCSnapshotTest													MyType;

protected:

	typedef GenericFileAllocator												Allocator;

	typedef MVCCAllocator<FileProfile<>, Allocator::Page>						TxnMgr;
	typedef typename TxnMgr::CtrDirectory										CtrDirectory;
	typedef typename TxnMgr::TxnPtr												TxnPtr;

	typedef CtrTF<FileProfile<>, Vector<Int>>::Type								VectorCtr;

	typedef typename Allocator::ID												ID;



	BigInt data_size_ = 1024*4;

public:

	MVCCSnapshotTest(StringRef name): Base(name)
	{
		MetadataRepository<FileProfile<>>::init();
		Allocator::initMetadata();
		TxnMgr::initMetadata();
		VectorCtr::initMetadata();

		this->size_ = 30;

		MEMORIA_ADD_TEST_PARAM(data_size_);

		MEMORIA_ADD_TEST(testSnapshot);
	}


	void testSnapshot()
	{
    	typename Allocator::Cfg cfg;

    	cfg.pages_buffer_size(10240);
    	cfg.sync_on_commit(false);

		String name = getResourcePath("snapshot.db");
		Allocator allocator(name, OpenMode::RWCT, cfg);

		TxnMgr mgr(&allocator);

		std::vector<std::map<BigInt, vector<Int>>> snapshots_data;

		for (Int c = 0; c < this->size_; c++)
		{
			this->out()<<"Create Snapshot "<<c<<endl;

			auto txn = mgr.begin();

			txn->setSnapshot(true);

			CtrDirectory roots(txn.get(), CTR_FIND, TxnMgr::CtrDirectoryName);

			std::map<BigInt, vector<Int>> snapshot;

			for (auto iter = roots.Begin(); !iter.isEnd(); iter++)
			{
				BigInt ctr_name = iter.key();

				VectorCtr ctr(txn.get(), CTR_FIND, ctr_name);

				vector<Int> ctr_data = snapshots_data[c - 1][ctr_name];

				assertVectorData(ctr, ctr_data);

				bool affect_ctr = getRandom(2) == 1;

				if (affect_ctr)
				{
					snapshot[ctr_name] = affect(ctr);
				}
				else {
					snapshot[ctr_name] = ctr_data;
				}

				mgr.flush();
			}

			VectorCtr ctr(txn.get(), CTR_CREATE);

			auto ctr_data 			= createRandomVector(data_size_);
			snapshot[ctr.name()] 	= ctr_data;

			ctr.seek(0).insert(ctr_data);

			snapshots_data.push_back(snapshot);

			txn->commit();
		}

		AssertEQ(MA_SRC, mgr.total_transactions(TxnStatus::SNAPSHOT), this->size_);
		AssertEQ(MA_SRC, snapshots_data.size(), (UBigInt)this->size_);

		auto iter = mgr.transactions(TxnStatus::SNAPSHOT);

		Int snapshot_idx = 0;
		while (iter->has_next())
		{
			this->out()<<"Check Snapshot "<<snapshot_idx<<endl;

			AssertEQ(MA_SRC, toInt(iter->status()), toInt(TxnStatus::SNAPSHOT));

			DebugCounter++;
			auto txn = iter->txn();

			CtrDirectory roots(txn.get(), CTR_FIND, TxnMgr::CtrDirectoryName);

			for (auto iter = roots.Begin(); !iter.isEnd(); iter++)
			{
				BigInt ctr_name = iter.key();

				VectorCtr ctr(txn.get(), CTR_FIND, ctr_name);

				vector<Int> ctr_data = snapshots_data[snapshot_idx][ctr_name];

				assertVectorData(ctr, ctr_data);
			}

			snapshot_idx++;

			iter->next();
		}
	}


	std::vector<Int> affect(VectorCtr& ctr)
	{
		BigInt start 		= getRandom(ctr.size() / 2);
		BigInt length 		= getRandom(ctr.size() / 4) + 1;

		vector<Int> data(length);

		for (auto& v: data) {
			v = getRandom(200);
		}

		auto iter = ctr.seek(start);
		iter.insert(data);

		return ctr.seek(0).subVector(ctr.size());
	}


	vector<Int> createRandomVector(BigInt max_size)
	{
		BigInt size = getRandom(max_size) + 1;

		vector<Int> data(size);

		for (auto& d: data)
		{
			d = getRandom(100);
		}

		return data;
	}

	void assertVectorData(VectorCtr& ctr, const vector<Int>& data)
	{
		AssertEQ(MA_SRC, ctr.size(), (BigInt)data.size());

		auto iter = ctr.seek(0);

		auto ctr_data = iter.subVector(data.size());

		for (size_t c = 0; c < data.size(); c++)
		{
			AssertEQ(MA_SRC, ctr_data[c], data[c]);
		}
	}
};

}


#endif
