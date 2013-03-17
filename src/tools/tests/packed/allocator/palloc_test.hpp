
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PALLOC_INIT_HPP_
#define MEMORIA_TESTS_PALLOC_INIT_HPP_

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/prototypes/btree/tools.hpp>

#include <memoria/core/packed2/packed_allocator.hpp>

#include <memory>

namespace memoria {

using namespace std;

class PackedAllocatorTest: public TestTask {

	typedef PackedAllocatorTest 											MyType;

	typedef PackedAllocator													Allocator;

	typedef shared_ptr<Allocator>											AllocatorPtr;

	class SimpleStruct: public PackedAllocatable {
		Int size_;
		UByte data_;
		UByte content_[];
	public:
		SimpleStruct() {}

		static Int block_size(Int size) {
			return sizeof(SimpleStruct) + size;
		}

		Int block_size() const
		{
			const Allocator* alloc = allocator();
			AssertTrue(MA_SRC, alloc != nullptr);

			return alloc->element_size(this);
		}

		Int object_size() const {
			return sizeof(MyType) + size_;
		}

		Int size() const {
			return size_;
		}

		void init(Int block_size, UByte data)
		{
			size_ = block_size - sizeof(SimpleStruct);
			data_ = data;
		}

		void fill()
		{
			for (Int c = 0; c < size_; c++)
			{
				content_[c] = data_;
			}
		}

		void check() const
		{
			for (Int c = 0; c < size_; c++)
			{
				AssertEQ(MA_SRC, content_[c], data_, SBuf()<<", idx = "<<c);
			}
		}

		void dump(ostream& out = cout) const
		{
			out<<"size_ = "<<size_<<endl;
			out<<"data_ = "<<hex<<(Int)data_<<dec<<endl;
			out<<"Data:"<<endl;
			dumpArray<UByte>(out, size_, [this](Int idx) {
				return this->content_[idx];
			});
		}

		void enlarge(Int delta)
		{
			Allocator* alloc = allocator();
			if (alloc)
			{
				Int block_size = alloc->element_size(this);

				Int new_size = alloc->enlargeBlock(this, block_size + delta);
				if (new_size)
				{
					init(new_size, data_);
					fill();
				}
			}
		}
	};

	class ComplexStruct: public PackedAllocator {
	public:
		ComplexStruct() {}

		static Int block_size(Int client_area) {
			return PackedAllocator::block_size(client_area, 3);
		}

		void init(Int block_size)
		{
			PackedAllocator::init(block_size, 3);
		}
	};

public:

	PackedAllocatorTest(): TestTask("Alloc")
    {
		MEMORIA_ADD_TEST(testCreate);
		MEMORIA_ADD_TEST(testEnlarge);
    }

    virtual ~PackedAllocatorTest() throw() {}

    AllocatorPtr createAllocator(Int client_area_size, Int elements)
    {
    	Int block_size 		= Allocator::block_size(client_area_size, elements);
    	void* mem_block 	= malloc(block_size);
    	memset(mem_block, 0, block_size);

    	Allocator* alloc 	= T2T<Allocator*>(mem_block);

    	alloc->init(block_size, elements);

    	AssertEQ(MA_SRC, alloc->elements(), elements);
    	AssertEQ(MA_SRC, alloc->block_size(), block_size);
    	AssertGE(MA_SRC, alloc->block_size(), client_area_size);

    	AssertGT(MA_SRC, alloc->layout_size(), 0);
    	AssertGT(MA_SRC, alloc->bitmap_size(), 0);

    	AssertGE(MA_SRC, alloc->client_area(), client_area_size);

    	return AllocatorPtr(alloc);
    }

    AllocatorPtr createSampleAllocator(Int client_area_size, Int elements)
    {
    	AllocatorPtr alloc_ptr 	= createAllocator(client_area_size, elements);
    	Allocator*   alloc 		= alloc_ptr.get();

    	for (Int c = 0; c < 2; c++)
    	{
    		Int size = SimpleStruct::block_size(111 + c*10);

    		AllocationBlock block = alloc->allocate(c, size, PackedBlockType::ALLOCATABLE);
    		AssertTrue(MA_SRC, block);

    		AssertGE(MA_SRC, alloc->element_size(c), size);

    		SimpleStruct* obj = block.cast<SimpleStruct>();

    		obj->init(size, 0x55 + c * 16);
    		obj->setAllocatorOffset(alloc);

    		obj->fill();

    		AssertEQ(MA_SRC, obj->size(), 111 + c*10);
    	}

    	AllocationBlock block = alloc->allocate(2, 512, PackedBlockType::ALLOCATABLE);
    	AssertTrue(MA_SRC, block);

    	ComplexStruct* cx_struct = T2T<ComplexStruct*>(block.ptr());

    	cx_struct->init(512);

    	for (Int c = 0; c < cx_struct->elements(); c++)
    	{
    		AllocationBlock block = cx_struct->allocate(c, 100, PackedBlockType::ALLOCATABLE);
    		AssertTrue(MA_SRC, block);

    		SimpleStruct* sl_struct = block.cast<SimpleStruct>();
    		sl_struct->init(100, 0x22 + c*16);
    		sl_struct->fill();
    	}

    	return alloc_ptr;
    }

    void checkStructure(const Allocator* alloc)
    {
    	for (Int c = 0; c < alloc->elements() - 1; c++)
    	{
    		const SimpleStruct* sl_struct = alloc->get<SimpleStruct>(c);
    		sl_struct->check();
    	}

    	const ComplexStruct* cx_struct = alloc->get<ComplexStruct>(alloc->elements() - 1);

    	for (Int c = 0; c < cx_struct->elements(); c++)
    	{
    		const SimpleStruct* sl_struct = cx_struct->get<SimpleStruct>(c);
    		sl_struct->check();
    	}
    }

    void dumpStructure(const Allocator* alloc)
    {
    	alloc->dump();

    	for (Int c = 0; c < alloc->elements() - 1; c++)
    	{
    		const SimpleStruct* sl_struct = alloc->get<SimpleStruct>(c);
    		sl_struct->dump();
    	}

    	const ComplexStruct* cx_struct = alloc->get<ComplexStruct>(alloc->elements() - 1);

    	cx_struct->dump();

    	for (Int c = 0; c < cx_struct->elements(); c++)
    	{
    		const SimpleStruct* sl_struct = cx_struct->get<SimpleStruct>(c);
    		sl_struct->dump();
    	}
    }


    void testCreate()
    {
    	AllocatorPtr alloc_ptr = createSampleAllocator(4096, 3);

    	Allocator* alloc = alloc_ptr.get();

    	checkStructure(alloc);
    }


    void testEnlarge()
    {
    	AllocatorPtr alloc_ptr = createSampleAllocator(4096, 3);

    	Allocator* alloc = alloc_ptr.get();

    	SimpleStruct* sl_struct = alloc->get<SimpleStruct>(0);

    	sl_struct->enlarge(100);

    	checkStructure(alloc);
    }
};


}


#endif
