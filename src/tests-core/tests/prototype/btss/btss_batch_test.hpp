// Copyright 2015 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#pragma once

#include <memoria/v1/memoria.hpp>

#include <memoria/v1/tests/tests.hpp>
#include <memoria/v1/tests/tools.hpp>

#include "btss_test_base.hpp"
#include "btss_test_factory.hpp"

#include <vector>
#include <algorithm>
#include <sstream>
#include <memory>

namespace memoria {
namespace v1 {

template <
    typename CtrName,
    typename AllocatorT     = ThreadInMemAllocator<>,
    typename ProfileT       = DefaultProfile<>
>
class BTSSBatchTest: public BTSSTestBase<CtrName, AllocatorT, ProfileT> {

    using Base   = BTSSTestBase<CtrName, AllocatorT, ProfileT>;
    using MyType = BTSSBatchTest<CtrName, AllocatorT, ProfileT>;

    using typename Base::Allocator;
    using typename Base::AllocatorPtr;
    using typename Base::Ctr;
    using typename Base::MemBuffer;
    using typename Base::Entry;
    using typename Base::Iterator;


    using Base::commit;
    using Base::drop;
    using Base::branch;
    using Base::allocator;
    using Base::snapshot;
    using Base::check;
    using Base::out;
    using Base::fillRandom;
    using Base::size_;
    using Base::storeAllocator;
    using Base::isReplayMode;
    using Base::getResourcePath;
    using Base::createBuffer;
    using Base::compareBuffers;
    using Base::getRandom;

public:

    Int max_block_size_ = 1024;
    Int check_size_     = 1000;

    UUID ctr_name_;
    Int prefix_size_;
    Int suffix_size_;
    Int block_size_;
    Int random_position_;

    BigInt iteration_ = 0;

    Int check_count_ = 0;

    Int cnt_i_ = 0;
    Int cnt_r_ = 0;

    typedef std::function<void (MyType*, Ctr&)> TestFn;

public:

    BTSSBatchTest(StringRef name):
        Base(name)
    {
        size_ = 1024 * 1024;

        MEMORIA_ADD_TEST_PARAM(max_block_size_);
        MEMORIA_ADD_TEST_PARAM(check_size_);

        MEMORIA_ADD_TEST_PARAM(ctr_name_)->state();
        MEMORIA_ADD_TEST_PARAM(block_size_)->state();
        MEMORIA_ADD_TEST_PARAM(prefix_size_)->state();
        MEMORIA_ADD_TEST_PARAM(suffix_size_)->state();
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

    virtual ~BTSSBatchTest() noexcept {}

    virtual MemBuffer createRandomBuffer(Int size)
    {
        auto buffer = MemBuffer(size);

        for (auto& v: buffer)
        {
            v = Entry(getRandom(100));
        }

        return buffer;
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

        //MemBuffer buf = this->createBuffer(length);

        MemBuffer buf = iter.read(length);

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

        iter.skip(-length);

        checkIterator(iter, MA_SRC);

        MemBuffer buf = iter.read(length);

        checkIterator(iter, MA_SRC);

        return buf;
    }


    virtual void checkBufferWritten(Iterator& iter, const MemBuffer& buffer, const char* source)
    {
        MemBuffer data = iter.read(buffer.size());

        compareBuffers(buffer, data, source);
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
        iter.check(out(), source);
    }




    void insertFromStart(Ctr& ctr)
    {
        auto iter = ctr.seek(0);

        MemBuffer suffix = createSuffixCheckBuffer(iter);
        MemBuffer data   = createDataBuffer();

        BigInt size = ctr.size();

        iter.insert(data);

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
        auto iter = ctr.seek(ctr.size());

        checkIterator(iter, MA_SRC);

        MemBuffer prefix = createPrefixCheckBuffer(iter);
        MemBuffer data   = createDataBuffer();

        BigInt position = iter.pos();

//         BTSSTestInputProvider<Ctr, MemBuffer> provider(data);
//         iter->insert_iobuffer(&provider);
        
        iter.insert(data);

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
        auto iter = ctr.seek(getRandomPosition(ctr));

        MemBuffer prefix = createPrefixCheckBuffer(iter);
        MemBuffer suffix = createSuffixCheckBuffer(iter);

        MemBuffer data   = createDataBuffer();

//         BTSSTestInputProvider<Ctr, MemBuffer> provider(data);
//         iter->insert_iobuffer(&provider);

        iter.insert(data);
        
        checkIterator(iter, MA_SRC);

        iter.skip(-data.size());
        iter.skip(-prefix.size());

        try {
            checkBufferWritten(iter, prefix, MA_SRC);
        }
        catch (...) {
            iter.dump_path();
            throw;
        }

        try{
            checkBufferWritten(iter, data,   MA_SRC);
        }
        catch (...) {
            iter.dump_path();
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

        auto iter = ctr.seek(size);

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

        auto iter = ctr.seek(ctr_size - size);

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
        auto iter = ctr.seek(getRandomPosition(ctr));

        BigInt size;

        if (this->isReplayMode()) {
            size = block_size_;
        }
        else {
            auto pos = iter.pos();
            auto ctr_size  = ctr.size();
            auto remainder = ctr_size - pos;

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
        ctr_name_ = UUID::make_random();

        iteration_ = 0;

        BigInt size = 0;

        while (size < this->size_)
        {
            auto snp = branch();

            auto ctr = find_or_create<CtrName>(snp, ctr_name_);

            test_fn(this, ctr);

            out()<<"Size: "<<ctr.size()<<endl;

            check(snp, "Insert: Container Check Failed", MA_SRC);

            iteration_++;

            commit();

            size = ctr.size();
        }

        if (!isReplayMode())
        {
            storeAllocator(getResourcePath((SBuf()<<"Insert_"<<(++cnt_i_)<<".dump").str()));
        }
    }


    virtual void testRemove(TestFn test_fn)
    {
        ctr_name_ = UUID::make_random();

        auto snp = branch();
        auto ctr = find_or_create<CtrName>(snp, ctr_name_);

        fillRandom(ctr, size_);

        commit();

        iteration_ = 0;

        BigInt size = ctr.size();

        while (size > 0)
        {
            snp = branch();
            ctr = find<CtrName>(snp, ctr_name_);

            test_fn(this, ctr);

            out()<<"Size: "<<ctr.size()<<endl;

            check("Remove: Container Check Failed", MA_SRC);

            size = ctr.size();

            iteration_++;
            commit();
        }

        if (!isReplayMode())
        {
            storeAllocator(getResourcePath((SBuf()<<"Remove_"<<(++cnt_i_)<<".dump").str()));
        }
    }

    virtual void replay(TestFn test_fn)
    {
        auto snp = branch();
        auto ctr = find_or_create<CtrName>(snp, ctr_name_);

        test_fn(this, ctr);

        check(snp, "Replay: Container Check Failed", MA_SRC);

        commit();
    }
};

}}
