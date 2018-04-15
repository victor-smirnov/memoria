
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

#include "packed_tree_test_base.hpp"

namespace memoria {
namespace v1 {
namespace tests {

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

    static constexpr int32_t Blocks = Base::Blocks;
    static constexpr int32_t SafetyMargin = InputBuffer::SafetyMargin;

public:

    using Base::createEmptyTree;
    using Base::getRandom;
    using Base::fillRandom;
    using Base::assertEqual;
    using Base::out;
    using Base::MEMBUF_SIZE;



    static void init_suite(TestSuite& suite)
    {
        MMA1_CLASS_TESTS(suite, testCreate, testValue, testPosition); //, testInsertion
    }

    InputBufferPtr createInputBuffer(int32_t capacity, int32_t free_space = 0)
    {
        int32_t object_block_size = InputBuffer::block_size(capacity);

        return MakeSharedPackedStructByBlock<InputBuffer>(object_block_size + free_space, SizesT(capacity));
    }

    std::vector<Values> fillBuffer(InputBufferPtr& buffer, int32_t max_value = 500)
    {
        std::vector<Values> data;

        while(true)
        {
            constexpr int32_t BUF_SIZE = 10;
            Values values[BUF_SIZE];

            for (int32_t c = 0; c < 10; c++)
            {
                for (int32_t b = 0; b < Blocks; b++) {
                    values[c][b] = this->getRandom(max_value);
                }
            }

            int32_t size = buffer->append(BUF_SIZE, [&](int32_t block, int32_t idx) {
                return values[idx][block];
            });

            data.insert(data.end(), values, values + size);

            if (size < BUF_SIZE) {
                break;
            }
        }

        buffer->reindex();

        assert_equals(buffer->size(), (int32_t)data.size());

        buffer->check();

        int32_t cnt = 0;
        buffer->scan(0, buffer->size(), [&](const auto& values){
            assert_equals(values, data[cnt], u"{}", cnt);
            cnt++;
        });

        return data;
    }

    void testCreate()
    {
        testCreate(0);

        for (int32_t c = 1; c <= this->size_; c *= 2)
        {
            testCreate(c);
        }
    }

    void testCreate(int32_t size)
    {
        out() << "Buffer capacity: " << size << std::endl;

        auto buffer = createInputBuffer(size);

        out() << "Block size=" << buffer->block_size() << std::endl;

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

        for (int32_t c = 1; c <= this->size_; c *= 2)
        {
            testValue(c);
        }
    }

    void testValue(int32_t size)
    {
        out() << "Buffer capacity: " << size << std::endl;

        auto buffer = createInputBuffer(size);

        auto values = fillBuffer(buffer);

        for (int32_t b = 0; b < Blocks; b++)
        {
            for (size_t c = 0; c < values.size(); c++)
            {
                assert_equals(values[c][b], buffer->value(b, c));
            }
        }
    }

    void testPosition()
    {
        testPosition(0);

        for (int32_t c = 1; c <= this->size_; c *= 2)
        {
            testPosition(c);
        }
    }

    void testPosition(int32_t size)
    {
        out() << "Buffer capacity: " << size << std::endl;

        auto buffer = createInputBuffer(size);

        auto values = fillBuffer(buffer);

        for (size_t c = 0; c < values.size(); c++)
        {
            auto pos1 = buffer->scan(0, c, [](const auto& ){});

            auto pos2 = buffer->positions(c);

            assert_equals(pos1, pos2);
        }
    }


    void testInsertion()
    {
        for (int32_t c = 256; c <= this->size_; c *= 2)
        {
            testInsertion(c);
        }
    }

    void testInsertion(int32_t size)
    {
        out() << "Buffer capacity: " << size << std::endl;

        auto tree = createEmptyTree();
        auto tree_data = fillRandom(tree, size);

        auto buffer = createInputBuffer(size);


        auto values = fillBuffer(buffer);

        for (int32_t c = 0; c < 5; c++)
        {
            int32_t pos = getRandom(tree->size());

            auto at = tree->positions(pos);

            int32_t buffer_size = buffer->size();
            auto buffer_starts = buffer->positions(0);
            auto buffer_ends = buffer->positions(buffer_size);

            tree->insert_buffer(at.idx(), buffer.get(), buffer_starts, buffer_ends, buffer_size);

            tree_data.insert(tree_data.begin() + pos, values.begin(), values.end());

            assertEqual(tree, tree_data);
        }
    }
};


}}}
