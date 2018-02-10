
// Copyright 2013 Victor Smirnov
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

#include <memoria/v1/tests/tests.hpp>
#include <memoria/v1/tests/tools.hpp>

#include <memory>
#include <vector>
#include <functional>

#include "rleseq_test_base.hpp"

namespace memoria {
namespace v1 {

using namespace std;

template <int32_t Symbols>
class PackedRLESearchableSequenceBufferTest: public PackedRLESequenceTestBase<Symbols> {

    using MyType = PackedRLESearchableSequenceBufferTest<Symbols>;
    using Base = PackedRLESequenceTestBase<Symbols>;



    using typename Base::Seq;
    using typename Base::SeqPtr;
    using Value = typename Seq::Value;

    using Buffer = typename Seq::InputBuffer;


    static const int32_t Blocks                 = Seq::Indexes;
    static const int32_t Bits                   = NumberOfBits(Symbols);

    using Base::getRandom;
    using Base::createEmptySequence;
    using Base::fillRandom;
    using Base::populate;
    using Base::populateRandom;
    using Base::assertIndexCorrect;
    using Base::assertEqual;
    using Base::assertEmpty;
    using Base::out;
    using Base::size_;
    using Base::iterations_;
    using Base::dumpAsSymbols;

public:

    PackedRLESearchableSequenceBufferTest(U16StringRef name): Base(name)
    {
        size_ = 300;

//        MEMORIA_ADD_TEST(runCreateBuffer);
        MEMORIA_ADD_TEST(runInsertBufferRandom);
    }

    virtual ~PackedRLESearchableSequenceBufferTest() noexcept {}

    auto createEmptyBuffer(int32_t capacity = 65536)
    {
        auto capacityv = typename Buffer::SizesT(capacity);

        int32_t block_size = Buffer::block_size(capacityv);

        return MakeSharedPackedStructByBlock<Buffer>(block_size, capacityv);
    }


    void runCreateBuffer()
    {
        for (int32_t s = 1; s < size_; s++)
        {
            auto buf = createEmptyBuffer();

            populateRandom(buf, s);
        }
    }

    void runInsertBufferRandom()
    {
        for (int32_t s = 0; s < 100; s++)
        {
            auto buf = createEmptyBuffer();
            auto seq = createEmptySequence();

            auto buf_data = populateRandom(buf, size_);
            auto seq_data = populateRandom(seq, size_);

            int32_t at = getRandom(seq->size());

            int32_t start   = getRandom(buf->size() / 2);
            int32_t size    = getRandom(buf->size() - start);

            tryInsertBuffer(seq, buf, at, start, size, seq_data, buf_data);
        }
    }





    template <typename T1, typename T2, typename T3>
    void tryInsertBuffer(T1& seq, T2& buf, int32_t at, int32_t start, int32_t size, vector<T3>& seq_data, vector<T3>& buf_data)
    {
        out() << "Insert Buffer at " << at << " start: " << start << " size: " << size << endl;

        seq->insert_buffer(at, buf.get(), start, size);

        seq_data.insert(
                seq_data.begin() + at,
                buf_data.begin() + start,
                buf_data.begin() + (start + size)
        );

        assertEqual(seq, seq_data);
    }
};


}}
