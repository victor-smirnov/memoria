
// Copyright Victor Smirnov 2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PACKED_PACKED_ARRAY_BUFFER_HPP_
#define MEMORIA_TESTS_PACKED_PACKED_ARRAY_BUFFER_HPP_

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include "packed_array_test_base.hpp"

namespace memoria {

using namespace std;

template <
    typename PackedTreeT
>
class PackedArrayInputBufferTest: public PackedArrayTestBase<PackedTreeT> {

    using MyType = PackedArrayInputBufferTest<PackedTreeT>;
    using Base   = PackedArrayTestBase<PackedTreeT>;

    typedef typename Base::Array                                                Array;
    typedef typename Base::Values                                               Values;

    using InputBuffer    = typename Array::InputBuffer;
    using InputBufferPtr = PkdStructSPtr<InputBuffer>;
    using SizesT         = typename Array::InputBuffer::SizesT;

    static constexpr Int Blocks = Array::Blocks;
    static constexpr Int SafetyMargin = InputBuffer::SafetyMargin;

    Int iterations_ = 10;

public:

    using Base::createEmptyArray;
    using Base::getRandom;
    using Base::fillRandom;
    using Base::assertEqual;
    using Base::out;



    PackedArrayInputBufferTest(StringRef name): Base(name)
    {
        this->size_ = 8192*8;

        MEMORIA_ADD_TEST_PARAM(iterations_);

        MEMORIA_ADD_TEST(testCreate);

        if (PkdStructSizeType<Array>::Value == PackedSizeType::VARIABLE)
        {
            MEMORIA_ADD_TEST(testValue);
            MEMORIA_ADD_TEST(testPosition);
        }

        MEMORIA_ADD_TEST(testInsertion);
    }

    virtual ~PackedArrayInputBufferTest() throw() {}

    InputBufferPtr createInputBuffer(Int capacity, Int free_space = 0)
    {
        Int object_block_size = InputBuffer::block_size(capacity);

        return MakeSharedPackedStructByBlock<InputBuffer>(object_block_size, SizesT(capacity));

//      Int allocator_size  = PackedAllocator::block_size(object_block_size + free_space, 1);
//
//      void* block = malloc(allocator_size);
//      PackedAllocator* allocator = T2T<PackedAllocator*>(block);
//      allocator->init(allocator_size, 1);
//      allocator->setTopLevelAllocator();
//
//      InputBuffer* buffer = allocator->allocateSpace<InputBuffer>(0, object_block_size);
//
//      buffer->init(SizesT(capacity));
//
//      return buffer;
    }

    std::vector<Values> fillBuffer(const InputBufferPtr& buffer, Int max_value = 500)
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

        fillBuffer(buffer);
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
                AssertEQ(MA_SRC, values[c][b], buffer->value(b, c), SBuf()<<b<<", "<<c);
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

            AssertEQ(MA_SRC, pos1[0], pos2[0]);
        }
    }


    void testInsertion()
    {
        for (Int c = 32; c <= this->size_; c *= 2)
        {
            testInsertion(c);
        }
    }

    void testInsertion(Int size)
    {
        out()<<"Buffer capacity: "<<size<<std::endl;

        auto array = createEmptyArray();
        auto tree_data = fillRandom(array, size);

        auto buffer = createInputBuffer(size);

        auto values = fillBuffer(buffer);

        for (Int c = 0; c < 5; c++)
        {
            Int pos = getRandom(array->size());

            Int buffer_size = buffer->size();

            auto array_size = array->size();

            array->insert_buffer(pos, buffer.get(), 0, buffer_size);

            AssertEQ(MA_SRC, array->size(), buffer->size() + array_size);

            tree_data.insert(tree_data.begin() + pos, values.begin(), values.end());

            assertEqual(array, tree_data);
        }
    }
};


}


#endif
