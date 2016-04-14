
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

#include <memoria/v1/tools/tests.hpp>
#include <memoria/v1/tools/tools.hpp>

#include "packed_tree_test_base.hpp"

namespace memoria {
namespace v1 {

using namespace std;

template <
    typename PackedTreeT
>
class PackedTreeInputBufferTest: public PackedTreeTestBase<PackedTreeT> {

    using MyType = PackedTreeInputBufferTest<PackedTreeT>;
    using Base   = PackedTreeTestBase<PackedTreeT>;

    typedef typename Base::Tree                                                 Tree;
    typedef typename Base::Values                                               Values;

    using InputBuffer = typename Tree::InputBuffer;
    using InputBufferPtr = PkdStructSPtr<InputBuffer>;

    using SizesT      = typename Tree::InputBuffer::SizesT;

    static constexpr Int Blocks = Base::Blocks;
    static constexpr Int SafetyMargin = InputBuffer::SafetyMargin;

    Int iterations_ = 10;

public:

    using Base::createEmptyTree;
    using Base::getRandom;
    using Base::fillRandom;
    using Base::assertEqual;
    using Base::out;
    using Base::MEMBUF_SIZE;


    PackedTreeInputBufferTest(StringRef name): Base(name)
    {
        this->size_ = 8192*8;

        MEMORIA_ADD_TEST_PARAM(iterations_);

        MEMORIA_ADD_TEST(testCreate);
        MEMORIA_ADD_TEST(testValue);
        MEMORIA_ADD_TEST(testPosition);
        MEMORIA_ADD_TEST(testInsertion);
    }

    virtual ~PackedTreeInputBufferTest() noexcept {}

    InputBufferPtr createInputBuffer(Int capacity, Int free_space = 0)
    {
        Int object_block_size = InputBuffer::block_size(capacity);

        return MakeSharedPackedStructByBlock<InputBuffer>(object_block_size + free_space, SizesT(capacity));
    }

    std::vector<Values> fillBuffer(InputBufferPtr& buffer, Int max_value = 500)
    {
        std::vector<Values> data;

        while(true)
        {
            constexpr Int BUF_SIZE = 10;
            Values values[BUF_SIZE];

            for (Int c = 0; c < 10; c++)
            {
                for (Int b = 0; b < Blocks; b++) {
                    values[c][b] = this->getRandom(max_value);
                }
            }

            Int size = buffer->append(BUF_SIZE, [&](Int block, Int idx) {
                return values[idx][block];
            });

            data.insert(data.end(), values, values + size);

            if (size < BUF_SIZE) {
                break;
            }
        }

        buffer->reindex();

        AssertEQ(MA_SRC, buffer->size(), (Int)data.size());

        buffer->check();

        Int cnt = 0;
        buffer->scan(0, buffer->size(), [&](const auto& values){
            AssertEQ(MA_SRC, values, data[cnt], SBuf()<<cnt);
            cnt++;
        });

        return data;
    }

    void testCreate()
    {
        testCreate(0);

        for (Int c = 1; c <= this->size_; c *= 2)
        {
            testCreate(c);
        }
    }

    void testCreate(Int size)
    {
        out()<<"Buffer capacity: "<<size<<std::endl;

        auto buffer = createInputBuffer(size);

        out()<<"Block size="<<buffer->block_size()<<endl;

        try {
            fillBuffer(buffer);
        }
        catch (...) {
            buffer->dump(out());
            throw;
        }
    }

    void testValue()
    {
        testValue(0);

        for (Int c = 1; c <= this->size_; c *= 2)
        {
            testValue(c);
        }
    }

    void testValue(Int size)
    {
        out()<<"Buffer capacity: "<<size<<std::endl;

        auto buffer = createInputBuffer(size);

        auto values = fillBuffer(buffer);

        for (Int b = 0; b < Blocks; b++)
        {
            for (size_t c = 0; c < values.size(); c++)
            {
                AssertEQ(MA_SRC, values[c][b], buffer->value(b, c));
            }
        }
    }

    void testPosition()
    {
        testPosition(0);

        for (Int c = 1; c <= this->size_; c *= 2)
        {
            testPosition(c);
        }
    }

    void testPosition(Int size)
    {
        out()<<"Buffer capacity: "<<size<<std::endl;

        auto buffer = createInputBuffer(size);

        auto values = fillBuffer(buffer);

        for (size_t c = 0; c < values.size(); c++)
        {
            auto pos1 = buffer->scan(0, c, [](const auto& ){});

            auto pos2 = buffer->positions(c);

            AssertEQ(MA_SRC, pos1, pos2);
        }
    }


    void testInsertion()
    {
        for (Int c = 256; c <= this->size_; c *= 2)
        {
            testInsertion(c);
        }
    }

    void testInsertion(Int size)
    {
        out()<<"Buffer capacity: "<<size<<std::endl;

        auto tree = createEmptyTree();
        auto tree_data = fillRandom(tree, size);

        auto buffer = createInputBuffer(size);


        auto values = fillBuffer(buffer);

        for (Int c = 0; c < 5; c++)
        {
            Int pos = getRandom(tree->size());

            SizesT at = tree->positions(pos);

            Int buffer_size = buffer->size();
            auto buffer_starts = buffer->positions(0);
            auto buffer_ends = buffer->positions(buffer_size);

            tree->insert_buffer(at, buffer.get(), buffer_starts, buffer_ends, buffer_size);

            tree_data.insert(tree_data.begin() + pos, values.begin(), values.end());

            assertEqual(tree, tree_data);
        }
    }
};


}}