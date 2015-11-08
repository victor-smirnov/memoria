// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_BTSS_BATCH_DELETION_TEST_HPP_
#define MEMORIA_TESTS_BTSS_BATCH_DELETION_TEST_HPP_

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
	typename ProfileT		= DefaultProfile<>
>
class BTSSBatchDeletionTest: public BTSSTestBase<CtrName, AllocatorT, ProfileT> {

    using Base 	 = BTSSTestBase<CtrName, AllocatorT, ProfileT>;
    using MyType = BTSSBatchDeletionTest<CtrName, AllocatorT, ProfileT>;

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

    OpenMode mode_ = OpenMode::READ | OpenMode::WRITE | OpenMode::CREATE | OpenMode::TRUNC;

    typedef std::function<void (MyType*, Allocator&, Ctr&)>                     TestFn;

public:

    BTSSBatchDeletionTest(StringRef name):
        TestTask(name)
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

    virtual ~BTSSBatchDeletionTest() throw() {}

    void createAllocator(AllocatorSPtr& allocator) {
    	allocator = std::make_shared<Allocator>();
    }

    virtual Iterator seek(Ctr& ctr, BigInt pos)                                 = 0;
    virtual void insert(Iterator& iter, MemBuffer& data)                        = 0;
    virtual void read(Iterator& iter, MemBuffer& data)                          = 0;
    virtual void skip(Iterator& iter, BigInt offset)                            = 0;
    virtual BigInt getSize(Ctr& ctr)                                            = 0;
    virtual BigInt getPosition(Iterator& iter)                                  = 0;

    virtual void remove(Iterator& iter, BigInt size)                            = 0;

    virtual void testInsert(TestFn test_fn)                                     = 0;
    virtual void testRemove(TestFn test_fn)                                     = 0;
    virtual void replay(TestFn test_fn)                                         = 0;


    virtual void checkAllocator(Allocator& allocator, const char* msg, const char* source)
    {
//        Int step_count = getcheckStep();
//
//        if (step_count > 0 && (check_count_ % step_count == 0))
//        {
//
//        }
//
//        check_count_++;
//
        ::memoria::check<Allocator>(allocator, msg, source);
    }

    virtual void fillRandom(Ctr& ctr, BigInt size)
    {
        MemBuffer data = this->createRandomBuffer(size);
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

            MemBuffer data = this->createRandomBuffer(tmp_size);

            insert(iter, data);

            alloc.flush();

            total += tmp_size;
        }
    }


    virtual BigInt getRandomPosition(Ctr& array)
    {
        if (this->isReplayMode())
        {
            return random_position_;
        }
        else {
            BigInt size = getSize(array);
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
            BigInt current_pos  = getPosition(iter);
            BigInt size         = getSize(iter.model());
            BigInt remainder    = size - current_pos;

            suffix_size_ = length = check_size_ >= remainder ? remainder : check_size_;
        }

        MemBuffer buf = this->createBuffer(length);

        read(iter, buf);

        checkIterator(iter, MA_SRC);

        skip(iter, -length);

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
            BigInt current_pos  = getPosition(iter);
            prefix_size_ = length = check_size_ >= current_pos ? current_pos : check_size_;
        }

        MemBuffer buf = this->createBuffer(length);

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
        if (this->isReplayMode()) {
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




    void insertFromStart(Allocator& alc, Ctr& ctr)
    {
        Iterator iter = seek(ctr, 0);

        MemBuffer suffix = createSuffixCheckBuffer(iter);
        MemBuffer data   = createDataBuffer();

        BigInt size = ctr.size();

        insert(iter, data);

        checkAllocator(alc, "", MA_SRC);

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





    void insertInTheMiddle(Allocator& alloc, Ctr& ctr)
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

    void removeFromStart(Allocator&, Ctr& ctr)
    {
        Int size;

        if (this->isReplayMode()) {
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

        if (this->isReplayMode()) {
            size = block_size_;
        }
        else {
            block_size_ = size = getRandomBufferSize(ctr_size < max_block_size_ ? ctr_size : max_block_size_);
        }

        Iterator iter = seek(ctr, ctr_size - size);

        checkIterator(iter, MA_SRC);

        MemBuffer prefix = createPrefixCheckBuffer(iter);

        BigInt last_size = getSize(ctr);

        remove(iter, size);

        checkIterator(iter, MA_SRC);

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

        if (this->isReplayMode()) {
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
