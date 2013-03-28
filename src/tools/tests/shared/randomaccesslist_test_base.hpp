
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_SHARED_RANDOMACCESSLIST_TEST_BASE_HPP_
#define MEMORIA_TESTS_SHARED_RANDOMACCESSLIST_TEST_BASE_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include <functional>

namespace memoria {

using namespace std;

template <
    typename ContainerTypeName,
    typename MemBuffer
>
class RandomAccessListTestBase: public SPTestTask {

    typedef RandomAccessListTestBase<
    			ContainerTypeName,
    			MemBuffer
    >                            														MyType;

    typedef SPTestTask 																	Base;

protected:
    typedef typename SCtrTF<ContainerTypeName>::Type                                    Ctr;
    typedef typename Ctr::Iterator                                                      Iterator;
    typedef typename Ctr::Accumulator                                                   Accumulator;
    typedef typename Ctr::ID                                                            ID;

    static const Int Indexes = Ctr::Indexes;

    Int max_block_size_     = 1024*40;
    Int check_size_     	= 1000;

    Int ctr_name_;
    Int prefix_size_;
    Int suffix_size_;
    Int block_size_;
    String dump_name_;

    typedef std::function<void (MyType*, Ctr&)> 										TestFn;

public:

    RandomAccessListTestBase(StringRef name):
        SPTestTask(name)
    {
        size_ = 1024*1024*16;

        MEMORIA_ADD_TEST_PARAM(max_block_size_);
        MEMORIA_ADD_TEST_PARAM(check_size_);

        MEMORIA_ADD_TEST_PARAM(ctr_name_)->state();
        MEMORIA_ADD_TEST_PARAM(block_size_)->state();
        MEMORIA_ADD_TEST_PARAM(prefix_size_)->state();
        MEMORIA_ADD_TEST_PARAM(suffix_size_)->state();
        MEMORIA_ADD_TEST_PARAM(dump_name_)->state();

        MEMORIA_ADD_TEST_WITH_REPLAY(testInsertFromStart, 	replayInsertFromStart);
        MEMORIA_ADD_TEST_WITH_REPLAY(testInsertAtEnd, 		replayInsertAtEnd);
        MEMORIA_ADD_TEST_WITH_REPLAY(testInsertInTheMiddle, replayInsertInTheMiddle);

        MEMORIA_ADD_TEST_WITH_REPLAY(testRemoveFromStart, 	replayRemoveFromStart);
        MEMORIA_ADD_TEST_WITH_REPLAY(testRemoveAtEnd, 		replayRemoveAtEnd);
        MEMORIA_ADD_TEST_WITH_REPLAY(testRemoveInTheMiddle, replayRemoveInTheMiddle);
    }

    virtual ~RandomAccessListTestBase() throw() {}

    virtual Iterator seek(Ctr& ctr, BigInt pos)                       	= 0;
    virtual void insert(Iterator& iter, MemBuffer& data)                = 0;
    virtual void read(Iterator& iter, MemBuffer& data)                  = 0;
    virtual void skip(Iterator& iter, BigInt offset)                    = 0;
    virtual BigInt getSize(Ctr& ctr)                                  	= 0;
    virtual BigInt getPosition(Iterator& iter)                          = 0;

    virtual void remove(Iterator& iter, BigInt size)                    = 0;

    virtual MemBuffer createBuffer(Int size) 							= 0;
    virtual MemBuffer createRandomBuffer(Int size) 						= 0;
    virtual void compareBuffers(const MemBuffer& src, const MemBuffer& tgt, const char* source) = 0;


    virtual void fillRandom(Ctr& ctr, BigInt size)
    {
    	MemBuffer data = createRandomBuffer(size);
    	Iterator iter = seek(ctr, 0);
    	insert(iter, data);
    }


    virtual BigInt getRandomPosition(Ctr& array)
    {
        BigInt size = getSize(array);
        return getBIRandom(size);
    }

    virtual void setUp()
    {
        if (btree_random_branching_)
        {
            btree_branching_ = 8 + getRandom(100);
            out()<<"BTree Branching: "<<btree_branching_<<endl;
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
    		length				= suffix_size_;
    	}
    	else {
    		BigInt current_pos 	= getPosition(iter);
    		BigInt size 		= getSize(iter.model());
    		BigInt remainder 	= size - current_pos;

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
    		length				= prefix_size_;
    	}
    	else {
    		BigInt current_pos 	= getPosition(iter);
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

    	auto& path = iter.path();

    	for (Int level = path.getSize() - 1; level > 0; level--)
    	{
    		bool found = false;

    		for (Int idx = 0; idx < path[level]->children_count(); idx++)
    		{
    			ID id = iter.model().getChildID(path[level].node(), idx);
    			if (id == path[level - 1]->id())
    			{
    				if (path[level - 1].parent_idx() != idx)
    				{
    					iter.dump(out());
    					throw TestException(source, SBuf()<<"Invalid parent-child relationship for node:"
    							<<path[level]->id()
    							<<" child: "
    							<<path[level - 1]->id()
    							<<" idx="<<idx
    							<<" parent_idx="
    							<<path[level-1].parent_idx());
    				}
    				else {
    					found = true;
    					break;
    				}
    			}
    		}

    		if (!found)
    		{
    			iter.dump(out());
    			throw TestException(source, SBuf()<<"Child: "
    					<<path[level - 1]->id()
    					<<" is not fount is it's parent, parent_idx="
    					<<path[level - 1].parent_idx());
    		}
    	}
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
    	Allocator allocator;
    	DefaultLogHandlerImpl logHandler(out());
    	allocator.getLogger()->setHandler(&logHandler);

    	Ctr ctr(&allocator);
    	ctr_name_ = ctr.name();

    	try {
    		while (ctr.size() < size_)
    		{
    			test_fn(this, ctr);

    			allocator.commit();
    		}
    	}
    	catch (...) {
    		dump_name_ = Store(allocator);
    		throw;
    	}
    }


    void testRemove(TestFn test_fn)
    {
    	Allocator allocator;
    	DefaultLogHandlerImpl logHandler(out());
    	allocator.getLogger()->setHandler(&logHandler);

    	Ctr ctr(&allocator);
    	ctr_name_ = ctr.name();

    	try {

    		fillRandom(ctr, Base::size_);

    		allocator.commit();

    		while (ctr.size() > 0)
    		{
    			test_fn(this, ctr);

    			allocator.commit();
    		}
    	}
    	catch (...) {
    		dump_name_ = Store(allocator);
    		throw;
    	}
    }

    void replay(TestFn test_fn)
    {
    	Allocator allocator;
    	DefaultLogHandlerImpl logHandler(out());
    	allocator.getLogger()->setHandler(&logHandler);

    	LoadAllocator(allocator, dump_name_);

    	Ctr ctr(&allocator, CTR_FIND, ctr_name_);

    	test_fn(this, ctr);
    }


    void insertFromStart(Ctr& ctr)
    {
    	Iterator iter = seek(ctr, 0);

    	MemBuffer suffix = createSuffixCheckBuffer(iter);
    	MemBuffer data 	 = createDataBuffer();

    	insert(iter, data);

    	checkIterator(iter, MA_SRC);

    	skip(iter, -data.size());

    	checkBufferWritten(iter, data, MA_SRC);
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
    	Iterator iter = seek(ctr, getSize(ctr));

    	MemBuffer prefix = createPrefixCheckBuffer(iter);
    	MemBuffer data 	 = createDataBuffer();

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





    void insertInTheMiddle(Ctr& ctr)
    {
    	Iterator iter 	 = seek(ctr, getRandomPosition(ctr));

    	MemBuffer prefix = createPrefixCheckBuffer(iter);
    	MemBuffer suffix = createSuffixCheckBuffer(iter);

    	MemBuffer data 	 = createDataBuffer();

    	insert(iter, data);

    	checkIterator(iter, MA_SRC);

    	skip(iter, -data.size());
    	skip(iter, -prefix.size());

    	checkBufferWritten(iter, prefix, MA_SRC);
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





    void removeFromStart(Ctr& ctr)
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



    void removeAtEnd(Ctr& ctr)
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



    void removeInTheMiddle(Ctr& ctr)
    {
    	Iterator iter 	 = seek(ctr, getRandomPosition(ctr));

    	BigInt size;

    	if (isReplayMode()) {
    		size = block_size_;
    	}
    	else {
    		BigInt pos   	 = getPosition(iter);
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
