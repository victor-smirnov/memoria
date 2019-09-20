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

#include "btss_test_base.hpp"
#include "btss_test_factory.hpp"

#include <memoria/v1/core/strings/format.hpp>

#include <vector>
#include <algorithm>
#include <sstream>
#include <memory>

namespace memoria {
namespace v1 {
namespace tests {

using namespace reactor;

template <
    typename CtrName,
    typename AllocatorT     = InMemAllocator<>,
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
    using Base::coutln;
    using Base::fillRandom;
    using Base::size_;
    using Base::storeAllocator;
    using Base::isReplayMode;
    using Base::getResourcePath;
    using Base::createBuffer;
    using Base::compareBuffers;
    using Base::getRandom;

public:

    int32_t max_block_size_ = 1024;
    int32_t check_size_     = 1000;

    UUID ctr_name_;
    int32_t prefix_size_;
    int32_t suffix_size_;
    int32_t block_size_;
    int32_t random_position_;

    int64_t iteration_ = 0;

    int32_t check_count_ = 0;

    int32_t cnt_i_ = 0;
    int32_t cnt_r_ = 0;

    typedef std::function<void (MyType*, Ctr&)> TestFn;

public:

    MMA1_STATE_FILEDS(max_block_size_, check_size_, ctr_name_, prefix_size_, suffix_size_, block_size_, random_position_, iteration_);

    BTSSBatchTest()
    {
        size_ = 1024 * 1024;
    }

    static void init_suite(TestSuite& suite)
    {
        MMA1_CLASS_TEST_WITH_REPLAY(suite, testInsertFromStart,   replayInsertFromStart);
        MMA1_CLASS_TEST_WITH_REPLAY(suite, testInsertAtEnd,       replayInsertAtEnd);
        MMA1_CLASS_TEST_WITH_REPLAY(suite, testInsertInTheMiddle, replayInsertInTheMiddle);

        MMA1_CLASS_TEST_WITH_REPLAY(suite, testRemoveFromStart,   replayRemoveFromStart);
        MMA1_CLASS_TEST_WITH_REPLAY(suite, testRemoveAtEnd,       replayRemoveAtEnd);
        MMA1_CLASS_TEST_WITH_REPLAY(suite, testRemoveInTheMiddle, replayRemoveInTheMiddle);
    }

    int64_t iteration() const {
        return iteration_;
    }

    virtual MemBuffer createRandomBuffer(int32_t size)
    {
        auto buffer = MemBuffer(size);

        for (auto& v: buffer)
        {
            v = Entry(getRandom(100));
        }

        return buffer;
    }


    virtual int64_t getRandomPosition(Ctr& array)
    {
        if (this->isReplayMode())
        {
            return random_position_;
        }
        else {
            int64_t size = array.size();
            return random_position_ = this->getBIRandom(size);
        }
    }


    int32_t getRandomBufferSize(int32_t max)
    {
        return this->getRandom(max - 1) + 1;
    }

    MemBuffer createSuffixCheckBuffer(Iterator& iter)
    {
        int64_t length;

        if (this->isReplayMode()) {
            length              = suffix_size_;
        }
        else {
            int64_t current_pos  = iter.pos();
            int64_t size         = iter.ctr().size();
            int64_t remainder    = size - current_pos;

            suffix_size_ = length = check_size_ >= remainder ? remainder : check_size_;
        }

        MemBuffer buf = iter.read(length);

        ctr_check_iterator(iter, MA_SRC);

        iter.skip(-length);

        ctr_check_iterator(iter, MA_SRC);

        return buf;
    }

    MemBuffer createPrefixCheckBuffer(Iterator& iter)
    {
        int64_t length;

        if (this->isReplayMode()) {
            length              = prefix_size_;
        }
        else {
            int64_t current_pos  = iter.pos();
            prefix_size_ = length = check_size_ >= current_pos ? current_pos : check_size_;
        }

        iter.skip(-length);

        ctr_check_iterator(iter, MA_SRC);

        MemBuffer buf = iter.read(length);

        ctr_check_iterator(iter, MA_SRC);

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

    virtual void ctr_check_iterator(Iterator& iter, const char* source)
    {
        ctr_check_iteratorPrefix(iter, source);
    }

    virtual void ctr_check_iteratorPrefix(Iterator& iter, const char* source)
    {
        iter.check(out(), source);
    }




    void insertFromStart(Ctr& ctr)
    {
        auto iter = ctr.seek(0);

        MemBuffer suffix = createSuffixCheckBuffer(iter);
        MemBuffer data   = createDataBuffer();

        int64_t size = ctr.size();

        iter.insert(data);

        int64_t size2 = ctr.size();

        assert_equals(size2, size + data.size());

        assert_equals(iter.pos(), data.size());

        ctr_check_iterator(iter, MA_SRC);

        iter.skip(-data.size());

        ctr_check_iterator(iter, MA_SRC);

        checkBufferWritten(iter, data, MA_SRC);

        ctr_check_iterator(iter, MA_SRC);

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

        ctr_check_iterator(iter, MA_SRC);

        MemBuffer prefix = createPrefixCheckBuffer(iter);
        MemBuffer data   = createDataBuffer();

        int64_t position = iter.pos();

//         BTSSTestInputProvider<Ctr, MemBuffer> provider(data);
//         iter->insert_iobuffer(&provider);
        
        iter.insert(data);

        ctr_check_iterator(iter, MA_SRC);

        assert_equals(iter.pos(), position + data.size());

        iter.skip(-data.size() - prefix.size());

        ctr_check_iterator(iter, MA_SRC);

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

        iter.insert(data);
        
        ctr_check_iterator(iter, MA_SRC);

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
        int32_t size;

        if (this->isReplayMode()) {
            size = block_size_;
        }
        else {
            int64_t ctr_size = ctr.size();
            block_size_ = size = getRandomBufferSize(ctr_size < max_block_size_ ? ctr_size : max_block_size_);
        }

        auto iter = ctr.seek(size);

        MemBuffer suffix = createSuffixCheckBuffer(iter);

        iter.skip(-size);

        iter.remove(size);

        assert_equals(0, iter.pos());

        ctr_check_iterator(iter, MA_SRC);

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
        int32_t size;

        int64_t ctr_size = ctr.size();

        if (this->isReplayMode()) {
            size = block_size_;
        }
        else {
            block_size_ = size = getRandomBufferSize(ctr_size < max_block_size_ ? ctr_size : max_block_size_);
        }

        auto iter = ctr.seek(ctr_size - size);

        ctr_check_iterator(iter, MA_SRC);

        MemBuffer prefix = createPrefixCheckBuffer(iter);

        int64_t last_size = ctr.size();

        iter.remove(size);

        ctr_check_iterator(iter, MA_SRC);

        assert_equals(last_size - size, ctr.size());

        ctr_check_iterator(iter, MA_SRC);

        assert_equals(iter.pos(), ctr.size());

        iter.skip(-prefix.size());

        assert_equals(iter.pos(), ctr.size() - prefix.size());

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

        int64_t size;

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

        int64_t position = iter.pos();

        iter.skip(size);

        MemBuffer suffix = createSuffixCheckBuffer(iter);

        iter.skip(-size);

        iter.remove(size);

        ctr_check_iterator(iter, MA_SRC);

        assert_equals(iter.pos(), position);

        iter.skip(-prefix.size());

        checkBufferWritten(iter, prefix, MA_SRC);

        assert_equals(iter.pos(), position);
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

        int64_t size = 0;

        while (size < this->size_)
        {
            auto snp = branch();

            auto ctr = find_or_create<CtrName>(snp, ctr_name_);

            test_fn(this, ctr);

            out() << "Size: " << ctr.size() << std::endl;

            check(snp, "Insert: Container Check Failed", MA_SRC);

            iteration_++;

            commit();

            size = ctr.size();
        }

        if (!isReplayMode())
        {
            storeAllocator(getResourcePath(fmt::format(u"Insert_{}.mma1", ++cnt_i_)));
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

        int64_t size = ctr.size();

        while (size > 0)
        {
            snp = branch();
            ctr = find<CtrName>(snp, ctr_name_);

            test_fn(this, ctr);

            out() << "Size: " << ctr.size() << std::endl;

            check("Remove: Container Check Failed", MA_SRC);

            size = ctr.size();

            iteration_++;
            commit();
        }

        if (!isReplayMode())
        {
            storeAllocator(getResourcePath(U16String((SBuf() << "Remove_" << (++cnt_i_) << ".dump").str())));
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

}}}
