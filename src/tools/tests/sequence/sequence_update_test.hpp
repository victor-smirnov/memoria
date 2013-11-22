// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_SEQUENCE_SEQUENCE_UPDATE_TEST_HPP_
#define MEMORIA_TESTS_SEQUENCE_SEQUENCE_UPDATE_TEST_HPP_

#include <memoria/memoria.hpp>
#include <memoria/tools/tests.hpp>

#include "sequence_test_base.hpp"

#include <vector>

namespace memoria {

using namespace memoria::vapi;
using namespace std;

template<Int BitsPerSymbol, bool Dense = true>
class SequenceUpdateTest: public SequenceTestBase<BitsPerSymbol, Dense> {

    typedef SequenceUpdateTest<BitsPerSymbol, Dense>                            MyType;
    typedef SequenceTestBase<BitsPerSymbol, Dense>                              Base;

    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::Iterator                                             Iterator;
    typedef typename Base::Ctr                                                  Ctr;

    typedef typename Base::MemBuffer                                            MemBuffer;

    static const Int Symbols = Base::Symbols;

    Int block_size_ = 1024*8;
    Int updates_ 	= 1024*8;

    BigInt pos_;
    BigInt buffer_size_;
    BigInt ctr_name_;

public:

    SequenceUpdateTest(StringRef name): Base(name)
    {
        this->size_ = 1024*1024*4;

        MEMORIA_ADD_TEST_PARAM(block_size_);
        MEMORIA_ADD_TEST_PARAM(updates_);

        MEMORIA_ADD_TEST_PARAM(pos_)->state();
        MEMORIA_ADD_TEST_PARAM(buffer_size_)->state();
        MEMORIA_ADD_TEST_PARAM(ctr_name_)->state();

        MEMORIA_ADD_TEST_WITH_REPLAY(testUpdate, replayUpdate);
    }

    void testUpdate()
    {
    	DefaultLogHandlerImpl logHandler(Base::out());

    	Allocator allocator;
    	allocator.getLogger()->setHandler(&logHandler);

    	Ctr ctr(&allocator);

    	ctr_name_ = ctr.name();

    	allocator.commit();

    	try {

    		MemBuffer buffer = this->createRandomBuffer(this->size_);
    		ctr.begin().insert(buffer);

    		Base::check(allocator, MA_SRC);

    		allocator.commit();

    		this->StoreAllocator(allocator, this->getResourcePath("ctr.dump"));

    		for (Int c = 0; c < updates_; c++)
    		{
    			this->out()<<c<<std::endl;

    			MemBuffer buf 		= this->createRandomBuffer(getRandom(this->block_size_) + 1);
    			BigInt pos = pos_	= getRandom(ctr.size() - buf.getSize());

    			auto iter = ctr.seek(pos);

    			iter.update(buf);

    			Base::check(allocator, MA_SRC);

    			buf.reset();

    			auto iter2 = ctr.seek(pos);

    			Int size = buffer_size_ = buf.getSize();

    			MemBuffer b1(size);
    			MemBuffer b2(size);

    			iter.skipBw(size);

    			iter.read(b1);
    			iter2.read(b2);

    			this->compareBuffers(b1, b2, MA_SRC);
    			this->compareBuffers(b1, buf, MA_SRC);
    			this->compareBuffers(b2, buf, MA_SRC);

    			allocator.commit();
    		}
    	}
    	catch (...) {
    		Base::dump_name_ = Base::Store(allocator);
    		throw;
    	}
    }

    void replayUpdate()
    {
    	Allocator allocator;
    	allocator.commit();

    	this->LoadAllocator(allocator, Base::dump_name_);

    	Base::check(allocator, MA_SRC);

    	Ctr ctr(&allocator, CTR_FIND, ctr_name_);

    	MemBuffer buf = this->createRandomBuffer(this->buffer_size_);

    	auto iter = ctr.seek(pos_);

    	iter.update(buf);

    	Base::check(allocator, MA_SRC);

    	buf.reset();

    	auto iter2 = ctr.seek(pos_);

    	Int size = buf.getSize();

    	MemBuffer b1(size);
    	MemBuffer b2(size);

    	iter.skipBw(size);

    	iter.read(b1);
    	iter2.read(b2);

    	this->compareBuffers(b1, b2, MA_SRC);
    	this->compareBuffers(b1, buf, MA_SRC);
    	this->compareBuffers(b2, buf, MA_SRC);
    }
};



}

#endif
