// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_BTSS_BATCH_INSERTION_TEST_HPP_
#define MEMORIA_TESTS_BTSS_BATCH_INSERTION_TEST_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include "../../shared/btss_test_base.hpp"
#include "btss_test_factory.hpp"

#include <vector>
#include <algorithm>
#include <sstream>
#include <memory>

namespace memoria {

template <
    typename CtrName,
	typename AllocatorT 	= SmallInMemAllocator,
	typename ProfileT		= SmallProfile<>
>
class BTSSBatchInsertionTest: public BTSSTestBase<CtrName, AllocatorT, ProfileT> {

    using Base 	 = BTSSTestBase<CtrName, AllocatorT, ProfileT>;
    using MyType = BTSSBatchInsertionTest<CtrName, AllocatorT, ProfileT>;

    using Allocator 	= typename Base::Allocator;
    using AllocatorSPtr = typename Base::AllocatorSPtr;
    using Ctr 			= typename Base::Ctr;
    using Iterator 		= typename Ctr::Iterator;

    using Entry 		= typename Ctr::Types::template StreamInputTuple<0>;

    using MemBuffer		= std::vector<Entry>;

public:

    BigInt size_			= 1024*1024;

    Int max_block_size_     = 1024*4;
    Int check_size_         = 1000;

    Int ctr_name_;
    Int prefix_size_;
    Int suffix_size_;
    Int block_size_;
    Int random_position_;
    String dump_name_;

    BigInt iteration_ = 0;

    Int check_count_ = 0;

    Int cnt_i_ = 0;
    Int cnt_r_ = 0;

    OpenMode mode_ = OpenMode::READ | OpenMode::WRITE | OpenMode::CREATE | OpenMode::TRUNC;

    typedef std::function<void (MyType*, Ctr&)> TestFn;

public:

    BTSSBatchInsertionTest(StringRef name):
        Base(name)
    {
        Ctr::initMetadata();

        MEMORIA_ADD_TEST_PARAM(max_block_size_);
        MEMORIA_ADD_TEST_PARAM(check_size_);

        MEMORIA_ADD_TEST_PARAM(ctr_name_)->state();
        MEMORIA_ADD_TEST_PARAM(block_size_)->state();
        MEMORIA_ADD_TEST_PARAM(prefix_size_)->state();
        MEMORIA_ADD_TEST_PARAM(suffix_size_)->state();
        MEMORIA_ADD_TEST_PARAM(dump_name_)->state();
        MEMORIA_ADD_TEST_PARAM(random_position_)->state();

        MEMORIA_ADD_TEST_PARAM(iteration_)->state();

        MEMORIA_ADD_TEST_WITH_REPLAY(testInsertFromStart,   replayInsertFromStart);
        MEMORIA_ADD_TEST_WITH_REPLAY(testInsertAtEnd,       replayInsertAtEnd);
        MEMORIA_ADD_TEST_WITH_REPLAY(testInsertInTheMiddle, replayInsertInTheMiddle);

        MEMORIA_ADD_TEST_WITH_REPLAY(testRemoveFromStart,   replayRemoveFromStart);
        MEMORIA_ADD_TEST_WITH_REPLAY(testRemoveAtEnd,       replayRemoveAtEnd);
        MEMORIA_ADD_TEST_WITH_REPLAY(testRemoveInTheMiddle, replayRemoveInTheMiddle);
    }

    BigInt iteration() const {
    	return iteration_;
    }

    virtual ~BTSSBatchInsertionTest() throw() {}

    virtual void createAllocator(AllocatorSPtr& allocator) {
    	allocator = std::make_shared<Allocator>();
    }

    virtual BigInt getRandomPosition(Ctr& array)
    {
        if (this->isReplayMode())
        {
            return random_position_;
        }
        else {
            BigInt size = array.size();
            return random_position_ = this->getBIRandom(size);
        }
    }


    Int getRandomBufferSize(Int max)
    {
        return this->getRandom(max - 1) + 1;
    }

    MemBuffer createSuffixCheckBuffer(Iterator& iter)
    {
        BigInt length;

        if (this->isReplayMode()) {
            length              = suffix_size_;
        }
        else {
            BigInt current_pos  = iter.pos();
            BigInt size         = iter.ctr().size();
            BigInt remainder    = size - current_pos;

            suffix_size_ = length = check_size_ >= remainder ? remainder : check_size_;
        }

        MemBuffer buf = this->createBuffer(length);

        iter.read(buf.begin(), buf.size());

        checkIterator(iter, MA_SRC);

        iter.skip(-length);

        checkIterator(iter, MA_SRC);

        return buf;
    }

    MemBuffer createPrefixCheckBuffer(Iterator& iter)
    {
        BigInt length;

        if (this->isReplayMode()) {
            length              = prefix_size_;
        }
        else {
            BigInt current_pos  = iter.pos();
            prefix_size_ = length = check_size_ >= current_pos ? current_pos : check_size_;
        }

        MemBuffer buf = this->createBuffer(length);

        iter.skip(-length);

        checkIterator(iter, MA_SRC);

        iter.read(buf.begin(), buf.size());

        checkIterator(iter, MA_SRC);

        return buf;
    }


    virtual void checkBufferWritten(Iterator& iter, const MemBuffer& buffer, const char* source)
    {
        MemBuffer data = this->createBuffer(buffer.size());

        iter.read(data.begin(), data.size());
        this->compareBuffers(buffer, data, source);
    }

    MemBuffer createDataBuffer()
    {
        if (this->isReplayMode()) {
            return this->createRandomBuffer(block_size_);
        }
        else {
            block_size_ = getRandomBufferSize(max_block_size_);
            return this->createRandomBuffer(block_size_);
        }
    }

    virtual void checkIterator(Iterator& iter, const char* source)
    {
        checkIteratorPrefix(iter, source);
    }

    virtual void checkIteratorPrefix(Iterator& iter, const char* source)
    {
    	auto tmp = iter;

    	tmp.refresh();

    	if (iter.cache() != tmp.cache())
    	{
    		throw TestException(
    				source,
    				SBuf()<<"Iterator cache mismatch: having: "<<iter.cache()<<", should be: "<<tmp.cache()
    		);
    	}
    }




    void insertFromStart(Ctr& ctr)
    {
        Iterator iter = ctr.seek(0);

        MemBuffer suffix = createSuffixCheckBuffer(iter);
        MemBuffer data   = createDataBuffer();

        BigInt size = ctr.size();

        iter.insert(data.begin(), data.end());

        this->checkAllocator("", MA_SRC);

        BigInt size2 = ctr.size();

        AssertEQ(MA_SRC, size2, size + data.size());

        AssertEQ(MA_SRC, iter.pos(), data.size());

        checkIterator(iter, MA_SRC);

        iter.skip(-data.size());

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

    void insertAtEnd(Ctr& ctr)
    {
    	Iterator iter = ctr.seek(ctr.size());

        checkIterator(iter, MA_SRC);

        MemBuffer prefix = createPrefixCheckBuffer(iter);
        MemBuffer data   = createDataBuffer();

        BigInt position = iter.pos();

        iter.insert(data.begin(), data.end());

        checkIterator(iter, MA_SRC);

        AssertEQ(MA_SRC, iter.pos(), position + data.size());

        iter.skip(-data.size() - prefix.size());

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





    void insertInTheMiddle(Ctr& ctr)
    {
        Iterator iter    = ctr.seek(getRandomPosition(ctr));

        MemBuffer prefix = createPrefixCheckBuffer(iter);
        MemBuffer suffix = createSuffixCheckBuffer(iter);

        MemBuffer data   = createDataBuffer();

        iter.insert(data.begin(), data.end());

        checkIterator(iter, MA_SRC);

        iter.skip(-data.size());
        iter.skip(-prefix.size());

        try {
            checkBufferWritten(iter, prefix, MA_SRC);
        }
        catch (...) {
            iter.dumpPath();
            throw;
        }

        try{
        	checkBufferWritten(iter, data,   MA_SRC);
        }
        catch (...) {
        	iter.dumpPath();
        	throw;
        }

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

    void removeFromStart(Ctr& ctr)
    {
        Int size;

        if (this->isReplayMode()) {
            size = block_size_;
        }
        else {
            BigInt ctr_size = ctr.size();
            block_size_ = size = getRandomBufferSize(ctr_size < max_block_size_ ? ctr_size : max_block_size_);
        }

        Iterator iter = ctr.seek(size);

        MemBuffer suffix = createSuffixCheckBuffer(iter);

        iter.skip(-size);

        iter.remove(size);

        AssertEQ(MA_SRC, iter.pos(), 0);

        checkIterator(iter, MA_SRC);

        checkBufferWritten(iter, suffix, MA_SRC);
    }

    void testRemoveFromStart() {
        testRemove(&MyType::removeFromStart);
    }

    void replayRemoveFromStart() {
        replay(&MyType::removeFromStart);
    }



    void removeAtEnd(Ctr& ctr)
    {
        Int size;

        BigInt ctr_size = ctr.size();

        if (this->isReplayMode()) {
            size = block_size_;
        }
        else {
            block_size_ = size = getRandomBufferSize(ctr_size < max_block_size_ ? ctr_size : max_block_size_);
        }

        Iterator iter = ctr.seek(ctr_size - size);

        checkIterator(iter, MA_SRC);

        MemBuffer prefix = createPrefixCheckBuffer(iter);

        BigInt last_size = ctr.size();

        iter.remove(size);

        checkIterator(iter, MA_SRC);

        AssertEQ(MA_SRC, last_size - size, ctr.size());

        checkIterator(iter, MA_SRC);

        AssertEQ(MA_SRC, iter.pos(), ctr.size());

        iter.skip(-prefix.size());

        AssertEQ(MA_SRC, iter.pos(), ctr.size() - prefix.size());

        checkBufferWritten(iter, prefix, MA_SRC);
    }

    void testRemoveAtEnd() {
        testRemove(&MyType::removeAtEnd);
    }

    void replayRemoveAtEnd() {
        replay(&MyType::removeAtEnd);
    }



    void removeInTheMiddle(Ctr& ctr)
    {
        Iterator iter    = ctr.seek(getRandomPosition(ctr));

        BigInt size;

        if (this->isReplayMode()) {
            size = block_size_;
        }
        else {
            BigInt pos       = iter.pos();
            BigInt ctr_size  = ctr.size();
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

        BigInt position = iter.pos();

        iter.skip(size);

        MemBuffer suffix = createSuffixCheckBuffer(iter);

        iter.skip(-size);

        iter.remove(size);

        checkIterator(iter, MA_SRC);

        AssertEQ(MA_SRC, iter.pos(), position);

        iter.skip(-prefix.size());

        checkBufferWritten(iter, prefix, MA_SRC);

        AssertEQ(MA_SRC, iter.pos(), position);
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

    std::ostream& out() {
    	return Base::out();
    }


    virtual void testInsert(TestFn test_fn)
    {
    	auto allocator = this->allocator_;

    	DefaultLogHandlerImpl logHandler(out());
    	allocator->getLogger()->setHandler(&logHandler);
    	allocator->getLogger()->level() = Logger::ERROR;

    	Ctr ctr(allocator.get());
    	this->ctr_name_ = ctr.name();

    	allocator->commit();

    	try {
    		this->iteration_ = 0;

    		while (ctr.size() < this->size_)
    		{
    			test_fn(this, ctr);

    			out()<<"Size: "<<ctr.size()<<endl;

    			this->checkAllocator("Insert: Container Check Failed", MA_SRC);

    			this->iteration_++;
    			this->allocator_->commit();
    		}

    		this->storeAllocator(this->getResourcePath(SBuf()<<"insert"<<(++cnt_i_)<<".dump"));
    	}
    	catch (...) {
    		this->dump_name_ = this->Store();
    		throw;
    	}
    }


    virtual void testRemove(TestFn test_fn)
    {
    	auto allocator = this->allocator_;

    	DefaultLogHandlerImpl logHandler(out());
    	allocator->getLogger()->setHandler(&logHandler);

    	Ctr ctr(allocator.get());
    	this->ctr_name_ = ctr.name();

    	allocator->commit();

    	try {

    		this->fillRandom(ctr, Base::size_);

    		allocator->commit();
    		this->iteration_ = 0;
    		while (ctr.size() > 0)
    		{
    			test_fn(this, ctr);

    			out()<<"Size: "<<ctr.size()<<endl;

    			this->checkAllocator("Remove: Container Check Failed", MA_SRC);

    			this->iteration_++;
    			allocator->commit();
    		}

    		this->storeAllocator(this->getResourcePath(SBuf()<<"remove"<<(++cnt_r_)<<".dump"));
    	}
    	catch (...) {
    		this->dump_name_ = this->Store();
    		throw;
    	}
    }

    virtual void replay(TestFn test_fn)
    {
    	this->loadAllocator(this->dump_name_);

    	auto allocator = this->allocator_;

    	DefaultLogHandlerImpl logHandler(out());
    	allocator->getLogger()->setHandler(&logHandler);

    	Ctr ctr = this->findCtr(ctr_name_);

    	test_fn(this, ctr);

    	this->checkAllocator("Replay: Container Check Failed", MA_SRC);
    }
};

}

#endif
