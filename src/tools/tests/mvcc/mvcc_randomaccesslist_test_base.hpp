
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


#include <functional>

namespace memoria {

using namespace std;

template <
	typename ContainerTypeName,
    typename MemBuffer
>
class MVCCRandomAccessListTestBase: public TestTask {

    typedef MVCCRandomAccessListTestBase<
                ContainerTypeName,
                MemBuffer
    >                                                                           MyType;

    typedef TestTask                                                            Base;

protected:
    typedef typename FCtrTF<ContainerTypeName>::Type                            Ctr;
    typedef typename Ctr::Iterator                                              Iterator;
    typedef typename Ctr::ID                                                    ID;
    typedef typename Ctr::Accumulator											Accumulator;



    typedef GenericFileAllocator												FileAllocator;
    typedef MVCCAllocator<FileProfile<>, FileAllocator::Page>					TxnMgr;
    typedef IWalkableAllocator<typename FileAllocator::Page>					Allocator;



    Int max_block_size_     = 1024*4;
    Int check_size_         = 1000;

    Int ctr_name_;
    Int prefix_size_;
    Int suffix_size_;
    Int block_size_;
    Int random_position_;
    String dump_name_;

    BigInt txn_id_;

    Int check_count_ = 0;

    bool clear_cache_ = false;

    OpenMode mode_ = OpenMode::READ | OpenMode::WRITE | OpenMode::CREATE | OpenMode::TRUNC;

    typedef std::function<void (MyType*, Allocator&, Ctr&)>                     TestFn;

public:

    MVCCRandomAccessListTestBase(StringRef name):
        TestTask(name)
    {
        size_ = 1024*1024;

        Ctr::initMetadata();

        MEMORIA_ADD_TEST_PARAM(max_block_size_);
        MEMORIA_ADD_TEST_PARAM(check_size_);
        MEMORIA_ADD_TEST_PARAM(clear_cache_);

        MEMORIA_ADD_TEST_PARAM(ctr_name_)->state();
        MEMORIA_ADD_TEST_PARAM(block_size_)->state();
        MEMORIA_ADD_TEST_PARAM(prefix_size_)->state();
        MEMORIA_ADD_TEST_PARAM(suffix_size_)->state();
        MEMORIA_ADD_TEST_PARAM(dump_name_)->state();
        MEMORIA_ADD_TEST_PARAM(random_position_)->state();
        MEMORIA_ADD_TEST_PARAM(txn_id_)->state();

        MEMORIA_ADD_TEST_WITH_REPLAY(testInsertFromStart,   replayInsertFromStart);
        MEMORIA_ADD_TEST_WITH_REPLAY(testInsertAtEnd,       replayInsertAtEnd);
        MEMORIA_ADD_TEST_WITH_REPLAY(testInsertInTheMiddle, replayInsertInTheMiddle);

        MEMORIA_ADD_TEST_WITH_REPLAY(testRemoveFromStart,   replayRemoveFromStart);
        MEMORIA_ADD_TEST_WITH_REPLAY(testRemoveAtEnd,       replayRemoveAtEnd);
        MEMORIA_ADD_TEST_WITH_REPLAY(testRemoveInTheMiddle, replayRemoveInTheMiddle);
    }

    virtual ~MVCCRandomAccessListTestBase() throw() {}

    virtual Iterator seek(Ctr& ctr, BigInt pos)                         = 0;
    virtual void insert(Iterator& iter, MemBuffer& data)                = 0;
    virtual void read(Iterator& iter, MemBuffer& data)                  = 0;
    virtual void skip(Iterator& iter, BigInt offset)                    = 0;
    virtual BigInt getSize(Ctr& ctr)                                    = 0;
    virtual BigInt getPosition(Iterator& iter)                          = 0;

    virtual void remove(Iterator& iter, BigInt size)                    = 0;

    virtual MemBuffer createBuffer(Int size)                            = 0;
    virtual MemBuffer createRandomBuffer(Int size)                      = 0;
    virtual void compareBuffers(const MemBuffer& src, const MemBuffer& tgt, const char* source) = 0;

    void checkAllocator(Allocator& allocator, const char* msg, const char* source)
    {
    	Int step_count = getcheckStep();

    	if (step_count > 0 && (check_count_ % step_count == 0))
    	{
    		::memoria::check<Allocator>(allocator, msg, source);
    	}

    	check_count_++;
    }

    virtual String Store(FileAllocator& allocator) const
    {
    	String new_name = allocator.file_name() + ".valid";

    	allocator.sync();

    	File file(allocator.file_name());
    	file.copy(new_name);

    	allocator.flush();

    	return new_name;
    }


    virtual void fillRandom(Ctr& ctr, BigInt size)
    {
    	MemBuffer data = createRandomBuffer(size);
    	Iterator iter = seek(ctr, 0);
    	insert(iter, data);
    }

    virtual void fillRandom(Allocator& alloc, Ctr& ctr, BigInt size)
    {
    	BigInt block_size = size > 65536*4 ? 65536*4 : size;

    	BigInt total = 0;

    	Iterator iter = seek(ctr, 0);

    	while (total < size)
    	{
    		BigInt tmp_size = size - total > block_size ? block_size : size - total;

    		MemBuffer data = createRandomBuffer(tmp_size);

    		insert(iter, data);

    		alloc.flush();

    		total += tmp_size;
    	}
    }


    virtual BigInt getRandomPosition(Ctr& array)
    {
        if (isReplayMode())
        {
            return random_position_;
        }
        else {
            BigInt size = getSize(array);
            return random_position_ = getBIRandom(size);
        }
    }


    Int getRandomBufferSize(Int max)
    {
        return memoria::getRandom(max - 1) + 1;
    }

    MemBuffer createSuffixCheckBuffer(Iterator& iter)
    {
        BigInt length;

        if (isReplayMode()) {
            length              = suffix_size_;
        }
        else {
            BigInt current_pos  = getPosition(iter);
            BigInt size         = getSize(iter.model());
            BigInt remainder    = size - current_pos;

            suffix_size_ = length = check_size_ >= remainder ? remainder : check_size_;
        }

        MemBuffer buf = createBuffer(length);

        read(iter, buf);

        checkIterator(iter, MA_SRC);

        skip(iter, -length);

        checkIterator(iter, MA_SRC);

        return buf;
    }

    MemBuffer createPrefixCheckBuffer(Iterator& iter)
    {
        BigInt length;

        if (isReplayMode()) {
            length              = prefix_size_;
        }
        else {
            BigInt current_pos  = getPosition(iter);
            prefix_size_ = length = check_size_ >= current_pos ? current_pos : check_size_;
        }

        MemBuffer buf = createBuffer(length);

        skip(iter, -length);

        checkIterator(iter, MA_SRC);

        read(iter, buf);

        checkIterator(iter, MA_SRC);

        return buf;
    }


    virtual void checkBufferWritten(Iterator& iter, const MemBuffer& buffer, const char* source)
    {
        MemBuffer data = createBuffer(buffer.size());

        read(iter, data);
        compareBuffers(buffer, data, source);
    }

    MemBuffer createDataBuffer()
    {
        if (isReplayMode()) {
            return createRandomBuffer(block_size_);
        }
        else {
            block_size_ = getRandomBufferSize(max_block_size_);
            return createRandomBuffer(block_size_);
        }
    }

    virtual void checkIterator(Iterator& iter, const char* source)
    {
        checkIteratorPrefix(iter, source);
    }

    virtual void checkIteratorPrefix(Iterator& iter, const char* source)
    {
        Accumulator prefixes;
        iter.ComputePrefix(prefixes);

        if (iter.prefixes() != prefixes)
        {
            iter.dump(out());
            throw TestException(source, SBuf()<<"Invalid prefix value. Iterator: "<<iter.prefixes()<<" Actual: "<<prefixes);
        }
    }


    void testInsert(TestFn test_fn)
    {
    	typename FileAllocator::Cfg cfg;

    	cfg.pages_buffer_size(10240);
    	cfg.sync_on_commit(false);

        FileAllocator file_allocator(getResourcePath("insert.db"), mode_, cfg);
        DefaultLogHandlerImpl logHandler(out());
        file_allocator.getLogger()->setHandler(&logHandler);
        file_allocator.getLogger()->level() = Logger::ERROR;

        TxnMgr txn_mgr(&file_allocator);

        try {
        	BigInt size = 0;

        	ctr_name_ = txn_mgr.createCtrName();

            while (size < size_)
            {
            	auto txn = txn_mgr.begin();

            	txn_id_ = txn->currentTxnId();

            	Ctr ctr(txn.get(), CTR_CREATE | CTR_FIND, ctr_name_);

            	test_fn(this, *txn.get(), ctr);

                out()<<"Size: "
                	 <<ctr.size()
                	 <<" "<<file_allocator.shared_pool_size()
                	 <<" "<<file_allocator.shared_created()
                	 <<" "<<file_allocator.shared_deleted()
                	 <<" txn_id="<<txn->currentTxnId()
                	 <<endl;

                checkAllocator(*txn.get(), "Insert: Txn Check Failed", MA_SRC);

                size = ctr.size();

                txn->commit();

                out()<<"Commited: "<<txn_mgr.currentTxnId()<<endl;
                checkAllocator(txn_mgr, "Insert: TxnMgr Check Failed", MA_SRC);

                if (clear_cache_) {
                	file_allocator.clearCache();
                }
            }

            txn_mgr.compactifyCommitHistory();
            txn_mgr.flush();
        }
        catch (...) {
            dump_name_ = Store(file_allocator);
            throw;
        }
    }


    void testInsert1(TestFn test_fn)
    {
    	typename FileAllocator::Cfg cfg;

    	cfg.pages_buffer_size(10240);
    	cfg.sync_on_commit(false);

    	FileAllocator file_allocator(getResourcePath("insert.db"), mode_, cfg);
    	DefaultLogHandlerImpl logHandler(out());
    	file_allocator.getLogger()->setHandler(&logHandler);
    	file_allocator.getLogger()->level() = Logger::ERROR;

    	TxnMgr txn_mgr(&file_allocator);

    	try {
    		BigInt size = 0;

    		ctr_name_ = txn_mgr.createCtrName();

    		while (size < size_)
    		{
    			txn_id_ = txn_mgr.currentTxnId();

    			Ctr ctr(&txn_mgr, CTR_CREATE | CTR_FIND, ctr_name_);

    			test_fn(this, txn_mgr, ctr);

    			out()<<"Size: "
    					<<ctr.size()
    					<<" "<<file_allocator.shared_pool_size()
    					<<" "<<file_allocator.shared_created()
    					<<" "<<file_allocator.shared_deleted()
    					<<endl;

    			checkAllocator(txn_mgr, "Insert: TxnMgr Check Failed", MA_SRC);

    			size = ctr.size();

    			txn_mgr.flush();

    			if (clear_cache_) {
    				file_allocator.clearCache();
    			}
    		}
    	}
    	catch (...) {
    		dump_name_ = Store(file_allocator);
    		throw;
    	}
    }


    void testRemove(TestFn test_fn)
    {
    	typename FileAllocator::Cfg cfg;

    	cfg.pages_buffer_size(10240);
    	cfg.sync_on_commit(false);

        FileAllocator file_allocator(getResourcePath("remove.db"), mode_, cfg);
        DefaultLogHandlerImpl logHandler(out());
        file_allocator.getLogger()->setHandler(&logHandler);

        TxnMgr mgr(&file_allocator);

        ctr_name_ = mgr.createCtrName();

        try {
        	BigInt size;

        	{
        		auto txn = mgr.begin();

        		Ctr ctr(txn.get(), CTR_CREATE | CTR_FIND, ctr_name_);

        		fillRandom(*txn.get(), ctr, Base::size_);

        		size = ctr.size();

        		txn->commit();
        	}

        	while (size > 0)
            {
                auto txn = mgr.begin();

                Ctr ctr(txn.get(), CTR_CREATE | CTR_FIND, ctr_name_);

            	test_fn(this, *txn.get(), ctr);

                out()<<"Size: "<<ctr.size()<<endl;

                checkAllocator(*txn.get(), "Remove: Txn Check Failed", MA_SRC);

                size = ctr.size();

                txn->commit();

                checkAllocator(mgr, "Remove: TxnMgr Check Failed", MA_SRC);

                if (clear_cache_)
                {
                	file_allocator.clearCache();
                }
            }

        	mgr.compactifyCommitHistory();
        	mgr.flush();

        	file_allocator.dumpAllocatedPages(this->getResourcePath("remove-allocated"));
        }
        catch (...) {
            dump_name_ = Store(file_allocator);
            throw;
        }
    }


    void testRemove1(TestFn test_fn)
    {
    	typename FileAllocator::Cfg cfg;

    	cfg.pages_buffer_size(10240);
    	cfg.sync_on_commit(false);

    	FileAllocator file_allocator(getResourcePath("remove.db"), mode_, cfg);
    	DefaultLogHandlerImpl logHandler(out());
    	file_allocator.getLogger()->setHandler(&logHandler);

    	TxnMgr mgr(&file_allocator);

    	ctr_name_ = mgr.createCtrName();

    	try {
    		BigInt size;

    		{
    			Ctr ctr(&mgr, CTR_CREATE | CTR_FIND, ctr_name_);

    			fillRandom(mgr, ctr, Base::size_);

    			size = ctr.size();
    		}


    		while (size > 0)
    		{
    			Ctr ctr(&mgr, CTR_CREATE | CTR_FIND, ctr_name_);

    			test_fn(this, mgr, ctr);

    			out()<<"Size: "<<ctr.size()<<endl;

    			checkAllocator(mgr, "Remove: TxnMgr Check Failed", MA_SRC);

    			size = ctr.size();

    			mgr.flush();

    			if (clear_cache_)
    			{
    				file_allocator.clearCache();
    			}
    		}
    	}
    	catch (...) {
    		dump_name_ = Store(file_allocator);
    		throw;
    	}
    }


    void replay(TestFn test_fn)
    {
        FileAllocator file_allocator(dump_name_, OpenMode::RW);

        DefaultLogHandlerImpl logHandler(out());
        file_allocator.getLogger()->setHandler(&logHandler);

        TxnMgr mgr(&file_allocator);

        auto txn = mgr.begin();

        Ctr ctr(txn.get(), CTR_FIND, ctr_name_);

        test_fn(this, *txn.get(), ctr);

        check(*txn.get(), "Replay: Container Check Failed", MA_SRC);
    }


    void insertFromStart(Allocator&, Ctr& ctr)
    {
        Iterator iter = seek(ctr, 0);

        MemBuffer suffix = createSuffixCheckBuffer(iter);
        MemBuffer data   = createDataBuffer();

        BigInt size = ctr.size();

        insert(iter, data);

        BigInt size2 = ctr.size();

        AssertEQ(MA_SRC, size2, size + data.size());

        AssertEQ(MA_SRC, getPosition(iter), (BigInt)data.size());

        checkIterator(iter, MA_SRC);

        skip(iter, -data.size());

        checkIterator(iter, MA_SRC);

        checkBufferWritten(iter, data, MA_SRC);

        checkIterator(iter, MA_SRC);

        checkBufferWritten(iter, suffix, MA_SRC);
    }

    void testInsertFromStart() {
        testInsert(&MyType::insertFromStart);
    }

    void replayInsertFromStart() {
        replay(&MyType::insertFromStart);
    }



    void insertAtEnd(Allocator&, Ctr& ctr)
    {
        Iterator iter = seek(ctr, getSize(ctr));
        checkIterator(iter, MA_SRC);

        MemBuffer prefix = createPrefixCheckBuffer(iter);
        MemBuffer data   = createDataBuffer();

        BigInt position = getPosition(iter);

        insert(iter, data);

        checkIterator(iter, MA_SRC);

        AssertEQ(MA_SRC, getPosition(iter), position + (BigInt)data.size());

        skip(iter, -data.size() - prefix.size());

        checkIterator(iter, MA_SRC);

        checkBufferWritten(iter, prefix, MA_SRC);
        checkBufferWritten(iter, data, MA_SRC);
    }


    void testInsertAtEnd()
    {
        testInsert(&MyType::insertAtEnd);
    }

    void replayInsertAtEnd()
    {
        replay(&MyType::insertAtEnd);
    }





    void insertInTheMiddle(Allocator&, Ctr& ctr)
    {
        Iterator iter    = seek(ctr, getRandomPosition(ctr));

        MemBuffer prefix = createPrefixCheckBuffer(iter);
        MemBuffer suffix = createSuffixCheckBuffer(iter);

        MemBuffer data   = createDataBuffer();

        insert(iter, data);

        checkIterator(iter, MA_SRC);

        skip(iter, -data.size());
        skip(iter, -prefix.size());

        try {
        	checkBufferWritten(iter, prefix, MA_SRC);
        }
        catch (...) {
        	iter.dumpPath();
        	throw;
        }
        checkBufferWritten(iter, data,   MA_SRC);
        checkBufferWritten(iter, suffix, MA_SRC);
    }


    void testInsertInTheMiddle()
    {
        testInsert(&MyType::insertInTheMiddle);
    }

    void replayInsertInTheMiddle()
    {
        replay(&MyType::insertInTheMiddle);
    }



    int cnt = 0;

    void removeFromStart(Allocator&, Ctr& ctr)
    {
        Int size;

        if (isReplayMode()) {
            size = block_size_;
        }
        else {
            BigInt ctr_size = getSize(ctr);
            block_size_ = size = getRandomBufferSize(ctr_size < max_block_size_ ? ctr_size : max_block_size_);
        }

        Iterator iter = seek(ctr, size);

        MemBuffer suffix = createSuffixCheckBuffer(iter);

        skip(iter, -size);

        remove(iter, size);

        AssertEQ(MA_SRC, getPosition(iter), 0);

        checkIterator(iter, MA_SRC);

        checkBufferWritten(iter, suffix, MA_SRC);
    }

    void testRemoveFromStart() {
        testRemove(&MyType::removeFromStart);
    }

    void replayRemoveFromStart() {
        replay(&MyType::removeFromStart);
    }



    void removeAtEnd(Allocator&, Ctr& ctr)
    {
        Int size;

        BigInt ctr_size = getSize(ctr);

        if (isReplayMode()) {
            size = block_size_;
        }
        else {
            block_size_ = size = getRandomBufferSize(ctr_size < max_block_size_ ? ctr_size : max_block_size_);
        }

        Iterator iter = seek(ctr, ctr_size - size);

        MemBuffer prefix = createPrefixCheckBuffer(iter);

        BigInt last_size = getSize(ctr);

        remove(iter, size);

        AssertEQ(MA_SRC, last_size - size, getSize(ctr));

        checkIterator(iter, MA_SRC);

        AssertEQ(MA_SRC, getPosition(iter), getSize(ctr));

        skip(iter, -prefix.size());

        AssertEQ(MA_SRC, getPosition(iter), getSize(ctr) - (BigInt)prefix.size());

        checkBufferWritten(iter, prefix, MA_SRC);
    }

    void testRemoveAtEnd() {
        testRemove(&MyType::removeAtEnd);
    }

    void replayRemoveAtEnd() {
        replay(&MyType::removeAtEnd);
    }



    void removeInTheMiddle(Allocator& allocator, Ctr& ctr)
    {
        Iterator iter    = seek(ctr, getRandomPosition(ctr));

        BigInt size;

        if (isReplayMode()) {
            size = block_size_;
        }
        else {
            BigInt pos       = getPosition(iter);
            BigInt ctr_size  = getSize(ctr);
            BigInt remainder = ctr_size - pos;

            if (max_block_size_ < remainder) {
                size = getRandomBufferSize(max_block_size_);
            }
            else {
                size = getRandomBufferSize(remainder);
            }

            block_size_ = size;
        }


        MemBuffer prefix = createPrefixCheckBuffer(iter);


        BigInt position = getPosition(iter);

        skip(iter, size);

        MemBuffer suffix = createSuffixCheckBuffer(iter);

        skip(iter,  -size);

        remove(iter, size);

        checkIterator(iter, MA_SRC);

        AssertEQ(MA_SRC, getPosition(iter), position);

        skip(iter, -prefix.size());

        checkBufferWritten(iter, prefix, MA_SRC);

        AssertEQ(MA_SRC, getPosition(iter), position);
        checkBufferWritten(iter, suffix, MA_SRC);
    }


    void testRemoveInTheMiddle()
    {
        testRemove(&MyType::removeInTheMiddle);
    }

    void replayRemoveInTheMiddle()
    {
        replay(&MyType::removeInTheMiddle);
    }
};

}


#endif
