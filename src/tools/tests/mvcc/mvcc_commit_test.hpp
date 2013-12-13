
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_TESTS_MVCC_COMMIT_TEST_HPP_
#define MEMORIA_TESTS_MVCC_COMMIT_TEST_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/allocators/file/factory.hpp>
#include <memoria/allocators/mvcc/mvcc_allocator.hpp>

#include "../dump.hpp"

namespace memoria {

class MVCCCommitTest: public TestTask {
    typedef TestTask                                                            Base;
    typedef MVCCCommitTest                                                      MyType;

protected:

    typedef GenericFileAllocator                                                Allocator;

    typedef MVCCAllocator<FileProfile<>, Allocator::Page>                       TxnMgr;
    typedef typename TxnMgr::TxnPtr                                             TxnPtr;

    typedef CtrTF<FileProfile<>, Vector<Int>>::Type                             VectorCtr;

    typedef typename Allocator::ID                                              ID;

    struct TxnData {
        vector<Int> data_;
        TxnPtr      txn_;
        BigInt      ctr_name_;

        TxnData(vector<Int> data, TxnPtr txn, BigInt ctr_name):
            data_(data), txn_(txn), ctr_name_(ctr_name)
        {}
    };

    BigInt data_size_ = 1024*40;

public:

    MVCCCommitTest(StringRef name): Base(name)
    {
        MetadataRepository<FileProfile<>>::init();
        Allocator::initMetadata();
        TxnMgr::initMetadata();
        VectorCtr::initMetadata();

        this->size_ = 100;

        MEMORIA_ADD_TEST_PARAM(data_size_);

        MEMORIA_ADD_TEST(testCreate);
    }


    void testCreate()
    {
        String name = getResourcePath("txn.db");
        Allocator allocator(name, OpenMode::RWCT);

        TxnMgr mgr(&allocator);

        mgr.setCompactifyHistory(false);

        vector<TxnData> txn_data;

        for (Int c = 0; c < this->size_; c++)
        {
            txn_data.push_back(
                    createTxnData(mgr)
            );

            mgr.flush();
        }

        AssertEQ(MA_SRC, mgr.total_transactions(TxnStatus::ACTIVE), this->size_);

        for (Int c = 0; c < this->size_; c++)
        {
            txn_data[c].txn_->commit();
        }

        AssertEQ(MA_SRC, mgr.total_transactions(TxnStatus::ACTIVE), 0);
        AssertEQ(MA_SRC, mgr.total_transactions(TxnStatus::COMMITED), this->size_);

        auto txn = mgr.begin();

        for (Int c = 0; c < this->size_; c++)
        {
            auto& data = txn_data[c];

            VectorCtr ctr(txn.get(), CTR_FIND, data.ctr_name_);
            assertVectorData(ctr, data.data_);
        }

        AssertEQ(MA_SRC, mgr.total_transactions(TxnStatus::ACTIVE), 1);

        txn->rollback();

        AssertEQ(MA_SRC, mgr.total_transactions(TxnStatus::ACTIVE), 0);

        mgr.compactifyCommitHistory();

        AssertEQ(MA_SRC, mgr.total_transactions(TxnStatus::COMMITED), 1);
    }

    TxnData createTxnData(TxnMgr& mgr)
    {
        BigInt ctr_name = mgr.createCtrName();
        auto txn = mgr.begin();

        TxnData data(
                createRandomVector(data_size_),
                txn,
                ctr_name
        );

        VectorCtr ctr(txn.get(), CTR_CREATE, ctr_name);

        ctr.seek(0).insert(data.data_);

        assertVectorData(ctr, data.data_);

        return data;
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
