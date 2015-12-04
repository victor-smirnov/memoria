
// Copyright Victor Smirnov 2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PACKED_PACKED_TREE_BUFFER_HPP_
#define MEMORIA_TESTS_PACKED_PACKED_TREE_BUFFER_HPP_

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include "packed_tree_test_base.hpp"

namespace memoria {

using namespace std;

template <
	typename PackedTreeT
>
class PackedTreeInputBufferTest: public PackedTreeTestBase<PackedTreeT> {

    using MyType = PackedTreeInputBufferTest<PackedTreeT>;
    using Base 	 = PackedTreeTestBase<PackedTreeT>;

    typedef typename Base::Tree                                                 Tree;
    typedef typename Base::Values                                               Values;

    using InputBuffer = typename Tree::InputBuffer;
    using SizesT 	  = typename Tree::InputBuffer::SizesT;

    static constexpr Int Blocks = Base::Blocks;
    static constexpr Int SafetyMargin = InputBuffer::SafetyMargin;

    Int iterations_ = 10;

public:

    using Base::getRandom;
    using Base::out;
    using Base::MEMBUF_SIZE;


    PackedTreeInputBufferTest(StringRef name): Base(name)
    {
        this->size_ = 8192*8;

        MEMORIA_ADD_TEST_PARAM(iterations_);

        MEMORIA_ADD_TEST(testCreate);
        MEMORIA_ADD_TEST(testValue);
        MEMORIA_ADD_TEST(testPosition);
    }

    virtual ~PackedTreeInputBufferTest() throw() {}

    InputBuffer* createInputBuffer(Int capacity, Int free_space = 0)
    {
    	Int object_block_size = InputBuffer::block_size(capacity);

    	Int allocator_size  = PackedAllocator::block_size(object_block_size + free_space, 1);

    	void* block = malloc(allocator_size);
    	PackedAllocator* allocator = T2T<PackedAllocator*>(block);
    	allocator->init(allocator_size, 1);
    	allocator->setTopLevelAllocator();

    	InputBuffer* buffer = allocator->allocateSpace<InputBuffer>(0, object_block_size);

    	buffer->init(SizesT(capacity));

    	return buffer;
    }

    std::vector<Values> fillBuffer(InputBuffer* buffer, Int max_value = 500) {
    	SizesT pos;

    	std::vector<Values> data;

    	while(buffer->capacity().gtAll((Int)InputBuffer::SafetyMargin))
    	{
    		buffer->append(pos, 1, [&, this](Int idx) {
    			Values values;
    			for (Int b = 0; b < Blocks; b++) {
    				values[b] = this->getRandom(500);
    			}

    			data.push_back(values);

    			return values;
    		});
    	}
    	buffer->reindex();

    	AssertEQ(MA_SRC, buffer->size(), (Int)data.size());

    	try {
    		buffer->check();
    	}
    	catch (...) {
    		buffer->dump(out());
    		throw;
    	}

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

        InputBuffer* buffer = createInputBuffer(size);
        PARemover remover(buffer);

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

        InputBuffer* buffer = createInputBuffer(size);
        PARemover remover(buffer);

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

        InputBuffer* buffer = createInputBuffer(size);
        PARemover remover(buffer);

        auto values = fillBuffer(buffer);

        for (size_t c = 0; c < values.size(); c++)
        {
        	auto pos1 = buffer->scan(0, c, [](const auto& ){});

        	auto pos2 = buffer->positions(c);

        	AssertEQ(MA_SRC, pos1, pos2);
        }

    }
};


}


#endif
