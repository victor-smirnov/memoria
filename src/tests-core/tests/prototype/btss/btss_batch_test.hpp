// Copyright 2015-2020 Victor Smirnov
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

#include <memoria/core/datatypes/buffer/buffer.hpp>

#include <memoria/api/store/memory_store_api.hpp>
#include <memoria/core/tools/random.hpp>

#include <memoria/core/strings/format.hpp>

#include <vector>
#include <algorithm>
#include <sstream>
#include <memory>

namespace memoria {
namespace tests {

using namespace reactor;


namespace internal_ {

    template <typename DataType> struct RandomValueGen;

    template <>
    struct RandomValueGen<Varchar> {
        static void append_random(DataTypeBuffer<Varchar>& buffer, int32_t)
        {
            U8String rnd_string = create_random_string(10);
            buffer.append(rnd_string);
        }
    };

    template <>
    struct RandomValueGen<UTinyInt> {
        static void append_random(DataTypeBuffer<UTinyInt>& buffer, int32_t)
        {
            uint8_t rnd_val = static_cast<uint8_t>(getRandomG(256));
            buffer.append(rnd_val);
        }
    };

    template <>
    struct RandomValueGen<UUID> {
        static void append_random(DataTypeBuffer<UUID>& buffer, int32_t)
        {
            uint64_t hi_val = static_cast<uint64_t>(getBIRandomG());
            uint64_t lo_val = static_cast<uint64_t>(getBIRandomG());
            buffer.append(UUID(hi_val, lo_val));
        }
    };
}


template <
    typename CtrName,
    typename ProfileT       = CoreApiProfile<>,
    typename StoreT     = IMemoryStorePtr<ProfileT>
>
class BTSSBatchTest: public BTSSTestBase<CtrName, ProfileT, StoreT> {

    using Base    = BTSSTestBase<CtrName, ProfileT, StoreT>;
    using MyType  = BTSSBatchTest<CtrName, ProfileT, StoreT>;

protected:

    using typename Base::StorePtr;
    using typename Base::CtrApi;
    using typename Base::MemBuffer;
    using typename Base::CtrID;

    using DataType = typename CtrApi::DataTypeT;

    using Base::commit;
    using Base::drop;
    using Base::branch;
    using Base::store;
    using Base::snapshot;
    using Base::check;
    using Base::out;
    using Base::coutln;
    using Base::fillRandom;
    using Base::size_;
    using Base::storeAllocator;
    using Base::isReplayMode;
    using Base::getResourcePath;

    using Base::compareBuffers;
    using Base::getRandom;

public:

    int32_t max_block_size_ = 1024 * 256;
    int32_t check_size_     = 1000;

    CtrID ctr_name_;
    int32_t prefix_size_;
    int32_t suffix_size_;
    int32_t block_size_;
    int32_t random_position_;

    int64_t iteration_ = 0;

    int32_t check_count_ = 0;

    int32_t cnt_i_ = 0;
    int32_t cnt_r_ = 0;

    typedef std::function<void (MyType*, CtrApi&)> TestFn;

public:

    MMA_STATE_FILEDS(max_block_size_, check_size_, ctr_name_, prefix_size_, suffix_size_, block_size_, random_position_, iteration_)

    BTSSBatchTest()
    {
        size_ = 1024 * 1024 * 4;
    }

    static void init_suite(TestSuite& suite)
    {
        MMA_CLASS_TEST_WITH_REPLAY(suite, testInsertFromStart,   replayInsertFromStart);
        MMA_CLASS_TEST_WITH_REPLAY(suite, testInsertAtEnd,       replayInsertAtEnd);
        MMA_CLASS_TEST_WITH_REPLAY(suite, testInsertInTheMiddle, replayInsertInTheMiddle);

        MMA_CLASS_TEST_WITH_REPLAY(suite, testRemoveFromStart,   replayRemoveFromStart);
        MMA_CLASS_TEST_WITH_REPLAY(suite, testRemoveAtEnd,       replayRemoveAtEnd);
        MMA_CLASS_TEST_WITH_REPLAY(suite, testRemoveInTheMiddle, replayRemoveInTheMiddle);
    }

    int64_t iteration() const {
        return iteration_;
    }

    virtual CtrSharedPtr<MemBuffer> createRandomBuffer(int32_t size)
    {
        auto buffer = ctr_make_shared<MemBuffer>();

        for (int32_t c = 0; c < size; c++)
        {
            internal_::RandomValueGen<DataType>::append_random(*buffer.get(), c);
        }

        return buffer;
    }



    virtual int64_t getRandomPosition(CtrApi& array)
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

    CtrSharedPtr<MemBuffer> createSuffixCheckBuffer(CtrApi& ctr, int64_t pos)
    {
        int64_t length;

        if (this->isReplayMode()) {
            length              = suffix_size_;
        }
        else {
            int64_t current_pos  = pos;
            int64_t size         = ctr.size();
            int64_t remainder    = size - current_pos;

            suffix_size_ = length = check_size_ >= remainder ? remainder : check_size_;
        }

        auto buf = ctr_make_shared<MemBuffer>();
        ctr.read_to(*buf, pos, length);
        return buf;
    }

    CtrSharedPtr<MemBuffer> createPrefixCheckBuffer(CtrApi& ctr, int64_t pos)
    {
        int64_t length;

        if (this->isReplayMode()) {
            length = prefix_size_;
        }
        else {
            int64_t current_pos = pos;
            prefix_size_ = length = check_size_ >= current_pos ? current_pos : check_size_;
        }

        auto buf = ctr_make_shared<MemBuffer>();
        ctr.read_to(*buf, pos - length, length);
        return buf;
    }


    virtual void checkBufferWritten(CtrApi& ctr, int64_t pos, const MemBuffer& buffer, const char* source)
    {
        MemBuffer buf;
        ctr.read_to(buf, pos, buffer.size());

        compareBuffers(buffer, buf, source);
    }

    CtrSharedPtr<MemBuffer> createDataBuffer()
    {
        if (this->isReplayMode()) {
            return this->createRandomBuffer(block_size_);
        }
        else {
            block_size_ = getRandomBufferSize(max_block_size_);
            return this->createRandomBuffer(block_size_);
        }
    }


    void insertFromStart(CtrApi& ctr)
    {
        auto suffix = createSuffixCheckBuffer(ctr, 0);
        auto data   = createDataBuffer();

        int64_t size = ctr.size();

        ctr.insert(0, *data);

        int64_t size2 = ctr.size();

        assert_equals(size2, size + data->size());

        checkBufferWritten(ctr, 0, *data.get(), MA_SRC);

        checkBufferWritten(ctr, data->size(), *suffix.get(), MA_SRC);
    }

    void testInsertFromStart() {
        testInsert(&MyType::insertFromStart);
    }

    void replayInsertFromStart() {
        replay(&MyType::insertFromStart);
    }

    void insertAtEnd(CtrApi& ctr)
    {
        int64_t pos = ctr.size();
        auto prefix = createPrefixCheckBuffer(ctr, pos);
        auto data   = createDataBuffer();

        ctr.insert(pos, *data);

        checkBufferWritten(ctr, pos - prefix->size(), *prefix.get(), MA_SRC);
        checkBufferWritten(ctr, pos, *data.get(), MA_SRC);
    }


    void testInsertAtEnd()
    {
        testInsert(&MyType::insertAtEnd);
    }

    void replayInsertAtEnd()
    {
        replay(&MyType::insertAtEnd);
    }

    void insertInTheMiddle(CtrApi& ctr)
    {
        int64_t pos = getRandomPosition(ctr);

        auto prefix = createPrefixCheckBuffer(ctr, pos);
        auto suffix = createSuffixCheckBuffer(ctr, pos);

        auto data   = createDataBuffer();



        ctr.insert(pos, *data);

        checkBufferWritten(ctr, pos - prefix->size(), *prefix.get(), MA_SRC);
        checkBufferWritten(ctr, pos, *data.get(), MA_SRC);
        checkBufferWritten(ctr, pos + data->size(), *suffix.get(), MA_SRC);
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

    void removeFromStart(CtrApi& ctr)
    {
        int64_t size;

        if (this->isReplayMode()) {
            size = block_size_;
        }
        else {
            int64_t ctr_size = ctr.size();
            block_size_ = size = getRandomBufferSize(ctr_size < max_block_size_ ? ctr_size : max_block_size_);
        }

        auto suffix = createSuffixCheckBuffer(ctr, size);

        ctr.remove_up_to(size);

        checkBufferWritten(ctr, 0, *suffix, MA_SRC);
    }

    void testRemoveFromStart() {
        testRemove(&MyType::removeFromStart);
    }

    void replayRemoveFromStart() {
        replay(&MyType::removeFromStart);
    }



    void removeAtEnd(CtrApi& ctr)
    {
        int32_t size;

        int64_t ctr_size = ctr.size();

        if (this->isReplayMode()) {
            size = block_size_;
        }
        else {
            block_size_ = size = getRandomBufferSize(ctr_size < max_block_size_ ? ctr_size : max_block_size_);
        }

        auto prefix = createPrefixCheckBuffer(ctr, ctr_size - size);

        int64_t last_size = ctr.size();

        ctr.remove_from(ctr_size - size);

        assert_equals(last_size - size, ctr.size());

        checkBufferWritten(ctr, ctr_size - size - prefix->size(), *prefix.get(), MA_SRC);
    }

    void testRemoveAtEnd() {
        testRemove(&MyType::removeAtEnd);
    }

    void replayRemoveAtEnd() {
        replay(&MyType::removeAtEnd);
    }



    void removeInTheMiddle(CtrApi& ctr)
    {
        int64_t tgt_pos = getRandomPosition(ctr);

        int64_t size;

        if (this->isReplayMode()) {
            size = block_size_;
        }
        else {
            auto ctr_size  = ctr.size();
            auto remainder = ctr_size - tgt_pos;

            if (max_block_size_ < remainder) {
                size = getRandomBufferSize(max_block_size_);
            }
            else {
                size = getRandomBufferSize(remainder);
            }

            block_size_ = size;
        }

        auto prefix = createPrefixCheckBuffer(ctr, tgt_pos);
        auto suffix = createSuffixCheckBuffer(ctr, tgt_pos + size);


        ctr.remove(tgt_pos, tgt_pos + size);

        checkBufferWritten(ctr, tgt_pos - prefix->size(), *prefix, MA_SRC);
        checkBufferWritten(ctr, tgt_pos, *suffix, MA_SRC);
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
        ctr_name_ = CtrID::make_random();

        iteration_ = 0;

        int64_t size = 0;

        while (size < this->size_)
        {
            auto snp = branch();

            auto ctr = find_or_create(snp, CtrName{}, ctr_name_);
            ctr->set_new_block_size(1024);

            test_fn(this, *ctr.get());

            if (iteration_ % 100 == 0)
            {
                out() << "Size: " << ctr->size() << std::endl;
                //this->check("Store structure checking", MMA_SRC);
            }

            //check(snp, "Insert: Container Check Failed", MA_SRC);

            iteration_++;

            commit();


            size = ctr->size();
        }

        if (!isReplayMode())
        {
            storeAllocator(getResourcePath(fmt::format("Insert_{}.mma1", ++cnt_i_)));
        }
    }


    virtual void testRemove(TestFn test_fn)
    {
        ctr_name_ = CtrID::make_random();

        auto snp = branch();
        auto ctr = find_or_create(snp, CtrName{}, ctr_name_);
        ctr->set_new_block_size(1024);

        fillRandom(*ctr.get(), size_);

        commit();

        iteration_ = 0;

        int64_t size = ctr->size();

        while (size > 0)
        {
            snp = branch();
            ctr = find<CtrName>(snp, ctr_name_);

            test_fn(this, *ctr.get());

            if (iteration_ % 100 == 0)
            {
                out() << "Size: " << ctr->size() << std::endl;
                //this->check("Store structure checking", MMA_SRC);
            }

            //check("Remove: Container Check Failed", MA_SRC);

            size = ctr->size();

            iteration_++;
            commit();
        }

        if (!isReplayMode())
        {
            storeAllocator(getResourcePath(U8String((SBuf() << "Remove_" << (++cnt_i_) << ".dump").str())));
        }
    }

    virtual void replay(TestFn test_fn)
    {
        auto snp = branch();
        auto ctr = find_or_create(snp, CtrName{}, ctr_name_);

        test_fn(this, *ctr.get());

        check(snp, "Replay: Container Check Failed", MA_SRC);

        commit();
    }
};

}}
