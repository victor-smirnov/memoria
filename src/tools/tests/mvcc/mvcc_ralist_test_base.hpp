
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_MVCCALLOC_RANDOMACCESSLIST_TEST_BASE_HPP_
#define MEMORIA_TESTS_MVCCALLOC_RANDOMACCESSLIST_TEST_BASE_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/profile_tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/allocators/file/factory.hpp>
#include <memoria/allocators/mvcc/mvcc_allocator.hpp>

#include "../shared/abstract_sequence_test_base.hpp"

#include <functional>

namespace memoria {

using namespace std;

template <
	typename ContainerTypeName,
    typename MemBuffer
>
class MVCCRandomAccessListTestBase: public AbstractSequenceTestBase<
	FileProfile<>,
	typename GenericFileAllocator::WalkableAllocator,
	ContainerTypeName,
	MemBuffer
> {

    typedef MVCCRandomAccessListTestBase<
                ContainerTypeName,
                MemBuffer
    >                                                                           MyType;

    typedef AbstractSequenceTestBase<
    		FileProfile<>,
    		typename GenericFileAllocator::WalkableAllocator,
    		ContainerTypeName,
    		MemBuffer
    >                                                            				Base;

protected:
    typedef typename Base::Ctr                            						Ctr;
    typedef typename Ctr::Iterator                                              Iterator;
    typedef typename Ctr::ID                                                    ID;
    typedef typename Ctr::Accumulator											Accumulator;


    typedef GenericFileAllocator												FileAllocator;
    typedef MVCCAllocator<FileProfile<>, FileAllocator::Page>					TxnMgr;
    typedef typename Base::Allocator											Allocator;



    OpenMode mode_ = OpenMode::READ | OpenMode::WRITE | OpenMode::CREATE | OpenMode::TRUNC;

    typedef typename Base::TestFn                     							TestFn;

    bool clear_cache_ = false;
    BigInt txn_id_;

public:

    MVCCRandomAccessListTestBase(StringRef name):
        Base(name)
    {
    	MEMORIA_ADD_TEST_PARAM(clear_cache_);

    	MEMORIA_ADD_TEST_PARAM(txn_id_)->state();
    }

    virtual ~MVCCRandomAccessListTestBase() throw() {}

    virtual String Store(FileAllocator& allocator) const
    {
    	String new_name = allocator.file_name() + ".valid";

    	allocator.sync();

    	File file(allocator.file_name());
    	file.copy(new_name);

    	allocator.flush();

    	return new_name;
    }


    virtual void testInsert(TestFn test_fn)
    {
    	typename FileAllocator::Cfg cfg;

    	cfg.pages_buffer_size(10240);
    	cfg.sync_on_commit(false);

        FileAllocator file_allocator(this->getResourcePath("insert.db"), mode_, cfg);
        DefaultLogHandlerImpl logHandler(this->out());
        file_allocator.getLogger()->setHandler(&logHandler);
        file_allocator.getLogger()->level() = Logger::ERROR;

        TxnMgr txn_mgr(&file_allocator);

        try {
        	BigInt size = 0;

        	this->ctr_name_ = txn_mgr.createCtrName();

            while (size < this->size_)
            {
            	auto txn = txn_mgr.begin();

            	bool snapshot = getRandom(10) == 0; //1 from 10 is a snapshot
            	txn->setSnapshot(snapshot);

            	txn_id_ = txn->currentTxnId();

            	Ctr ctr(txn.get(), CTR_CREATE | CTR_FIND, this->ctr_name_);

            	test_fn(this, *txn.get(), ctr);

            	this->out()<<"Size: "
                	 <<ctr.size()
                	 <<" "<<file_allocator.shared_pool_size()
                	 <<" "<<file_allocator.shared_created()
                	 <<" "<<file_allocator.shared_deleted()
                	 <<" txn_id="<<txn->currentTxnId()
                	 <<" snapshot="<<txn->is_snapshot()
                	 <<endl;

            	this->checkAllocator(*txn.get(), "Insert: Txn Check Failed", MA_SRC);

                size = ctr.size();

                txn->commit();

                this->out()<<"Commited: "<<txn_mgr.currentTxnId()<<endl;
                this->checkAllocator(txn_mgr, "Insert: TxnMgr Check Failed", MA_SRC);

                if (clear_cache_) {
                	file_allocator.clearCache();
                }
            }

            txn_mgr.compactifyCommitHistory();
            txn_mgr.flush();
        }
        catch (...) {
        	this->dump_name_ = Store(file_allocator);
            throw;
        }
    }


    virtual void testInsert1(TestFn test_fn)
    {
    	typename FileAllocator::Cfg cfg;

    	cfg.pages_buffer_size(10240);
    	cfg.sync_on_commit(false);

    	FileAllocator file_allocator(this->getResourcePath("insert.db"), mode_, cfg);
    	DefaultLogHandlerImpl logHandler(this->out());
    	file_allocator.getLogger()->setHandler(&logHandler);
    	file_allocator.getLogger()->level() = Logger::ERROR;

    	TxnMgr txn_mgr(&file_allocator);

    	try {
    		BigInt size = 0;

    		this->ctr_name_ = txn_mgr.createCtrName();

    		while (size < this->size_)
    		{
    			txn_id_ = txn_mgr.currentTxnId();

    			Ctr ctr(&txn_mgr, CTR_CREATE | CTR_FIND, this->ctr_name_);

    			test_fn(this, txn_mgr, ctr);

    			this->out()<<"Size: "
    					<<ctr.size()
    					<<" "<<file_allocator.shared_pool_size()
    					<<" "<<file_allocator.shared_created()
    					<<" "<<file_allocator.shared_deleted()
    					<<endl;

    			this->checkAllocator(txn_mgr, "Insert: TxnMgr Check Failed", MA_SRC);

    			size = ctr.size();

    			txn_mgr.flush();

    			if (clear_cache_) {
    				file_allocator.clearCache();
    			}
    		}
    	}
    	catch (...) {
    		this->dump_name_ = Store(file_allocator);
    		throw;
    	}
    }


    virtual void testRemove(TestFn test_fn)
    {
    	typename FileAllocator::Cfg cfg;

    	cfg.pages_buffer_size(10240);
    	cfg.sync_on_commit(false);

        FileAllocator file_allocator(this->getResourcePath("remove.db"), mode_, cfg);
        DefaultLogHandlerImpl logHandler(this->out());
        file_allocator.getLogger()->setHandler(&logHandler);

        TxnMgr mgr(&file_allocator);

        this->ctr_name_ = mgr.createCtrName();

        try {
        	BigInt size;

        	{
        		auto txn = mgr.begin();

        		Ctr ctr(txn.get(), CTR_CREATE | CTR_FIND, this->ctr_name_);

        		this->fillRandom(*txn.get(), ctr, Base::size_);

        		size = ctr.size();

        		txn->commit();
        	}

        	while (size > 0)
            {
                auto txn = mgr.begin();

                bool snapshot = getRandom(10) == 0; //1 from 10 is a snapshot
                txn->setSnapshot(snapshot);

                Ctr ctr(txn.get(), CTR_CREATE | CTR_FIND, this->ctr_name_);

            	test_fn(this, *txn.get(), ctr);

            	this->out()<<"Size: "<<ctr.size()<<endl;

            	this->checkAllocator(*txn.get(), "Remove: Txn Check Failed", MA_SRC);

                size = ctr.size();

                txn->commit();

                this->checkAllocator(mgr, "Remove: TxnMgr Check Failed", MA_SRC);

                if (clear_cache_)
                {
                	file_allocator.clearCache();
                }
            }

        	mgr.compactifyCommitHistory();
        	mgr.flush();
        }
        catch (...) {
        	this->dump_name_ = Store(file_allocator);
            throw;
        }
    }


    virtual void testRemove1(TestFn test_fn)
    {
    	typename FileAllocator::Cfg cfg;

    	cfg.pages_buffer_size(10240);
    	cfg.sync_on_commit(false);

    	FileAllocator file_allocator(this->getResourcePath("remove.db"), mode_, cfg);
    	DefaultLogHandlerImpl logHandler(this->out());
    	file_allocator.getLogger()->setHandler(&logHandler);

    	TxnMgr mgr(&file_allocator);

    	this->ctr_name_ = mgr.createCtrName();

    	try {
    		BigInt size;

    		{
    			Ctr ctr(&mgr, CTR_CREATE | CTR_FIND, this->ctr_name_);

    			this->fillRandom(mgr, ctr, Base::size_);

    			size = ctr.size();
    		}


    		while (size > 0)
    		{
    			Ctr ctr(&mgr, CTR_CREATE | CTR_FIND, this->ctr_name_);

    			test_fn(this, mgr, ctr);

    			this->out()<<"Size: "<<ctr.size()<<endl;

    			this->checkAllocator(mgr, "Remove: TxnMgr Check Failed", MA_SRC);

    			size = ctr.size();

    			mgr.flush();

    			if (clear_cache_)
    			{
    				file_allocator.clearCache();
    			}
    		}
    	}
    	catch (...) {
    		this->dump_name_ = Store(file_allocator);
    		throw;
    	}
    }


    void replay(TestFn test_fn)
    {
        FileAllocator file_allocator(this->dump_name_, OpenMode::RW);

        DefaultLogHandlerImpl logHandler(this->out());
        file_allocator.getLogger()->setHandler(&logHandler);

        TxnMgr mgr(&file_allocator);

        auto txn = mgr.begin();

        Ctr ctr(txn.get(), CTR_FIND, this->ctr_name_);

        test_fn(this, *txn.get(), ctr);

        this->checkAllocator(*txn.get(), "Replay: Container Check Failed", MA_SRC);
    }
};

}


#endif
