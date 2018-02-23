
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
#include <memoria/v1/core/memory/malloc.hpp>

#include <memoria/v1/core/packed/tools/packed_allocator.hpp>

#include <memory>

namespace memoria {
namespace v1 {

using namespace std;

class PackedAllocatorTest: public TestTask {

    typedef PackedAllocatorTest                                                 MyType;

    typedef PackedAllocator                                                     Allocator;

    typedef std::shared_ptr<Allocator>                                               AllocatorPtr;

    class SimpleStruct: public PackedAllocatable {
        int32_t size_;
        uint8_t data_;
        uint8_t content_[];
    public:
        SimpleStruct() {}

        static int32_t block_size(int32_t size) {
            return sizeof(SimpleStruct) + size;
        }

        int32_t block_size() const
        {
            const Allocator* alloc = allocator();
            AssertTrue(MA_SRC, alloc != nullptr);

            return alloc->element_size(this);
        }

        int32_t object_size() const {
            return sizeof(MyType) + size_;
        }

        int32_t size() const {
            return size_;
        }

        void init(int32_t block_size)
        {
            size_ = block_size - sizeof(SimpleStruct);
        }

        void fill(uint8_t data)
        {
            data_ = data;

            for (int32_t c = 0; c < size_; c++)
            {
                content_[c] = data_;
            }
        }

        void check() const
        {
            for (int32_t c = 0; c < size_; c++)
            {
                AssertEQ(MA_SRC, content_[c], data_, SBuf()<<", idx = "<<c);
            }
        }

        void dump(ostream& out = cout) const
        {
            out<<"size_ = "<<size_<<endl;
            out<<"data_ = "<<hex<<(int32_t)data_<<dec<<endl;
            out<<"Data:"<<endl;
            dumpArray<uint8_t>(out, size_, [this](int32_t idx) {
                return this->content_[idx];
            });
        }

        void enlarge(int32_t delta)
        {
            Allocator* alloc = allocator();
            int32_t block_size   = alloc->element_size(this);
            int32_t new_size     = alloc->resizeBlock(this, block_size + delta);

            init(new_size);
            fill(data_);
        }

        void shrink(int32_t delta)
        {
            Allocator* alloc = allocator();
            int32_t block_size   = alloc->element_size(this);
            int32_t new_size     = alloc->resizeBlock(this, block_size - delta);

            init(new_size);
            fill(data_);
        }
    };

    class ComplexStruct: public PackedAllocator {
    public:
        ComplexStruct() {}

        static int32_t block_size(int32_t client_area) {
            return PackedAllocator::block_size(client_area, 3);
        }

        void init(int32_t block_size)
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

    virtual ~PackedAllocatorTest() noexcept {}

    AllocatorPtr createAllocator(int32_t client_area_size, int32_t elements)
    {
        int32_t block_size = Allocator::block_size(client_area_size, elements);

        Allocator* alloc = allocate_system_zeroed<Allocator>(block_size).release();

        alloc->init(block_size, elements);
        alloc->setTopLevelAllocator();

        AssertEQ(MA_SRC, alloc->elements(), elements);
        AssertEQ(MA_SRC, alloc->block_size(), block_size);
        AssertGE(MA_SRC, alloc->block_size(), client_area_size);

        AssertGT(MA_SRC, alloc->layout_size(), 0);
        AssertGT(MA_SRC, alloc->bitmap_size(), 0);

        AssertGE(MA_SRC, alloc->client_area(), client_area_size);

        return AllocatorPtr(alloc, free_system);
    }

    AllocatorPtr createSampleAllocator(int32_t client_area_size, int32_t elements)
    {
        AllocatorPtr alloc_ptr  = createAllocator(client_area_size, elements);
        Allocator*   alloc      = alloc_ptr.get();

        for (int32_t c = 0; c < 2; c++)
        {
            int32_t size = SimpleStruct::block_size(111 + c*10);

            SimpleStruct* obj = alloc->allocate<SimpleStruct>(c, size);
            AssertGE(MA_SRC, alloc->element_size(c), size);

            obj->fill(0x55 + c * 16);

            AssertGE(MA_SRC, obj->size(), 111 + c*10);
        }

        AllocationBlock block = alloc->allocate(2, 512, PackedBlockType::ALLOCATABLE);

        ComplexStruct* cx_struct = T2T<ComplexStruct*>(block.ptr());

        cx_struct->init(512);

        for (int32_t c = 0; c < cx_struct->elements(); c++)
        {
            SimpleStruct* sl_struct = cx_struct->allocate<SimpleStruct>(c, 100);

            sl_struct->fill(0x22 + c*16);
        }

        return alloc_ptr;
    }

    void checkStructure(const Allocator* alloc)
    {
        for (int32_t c = 0; c < alloc->elements() - 1; c++)
        {
            const SimpleStruct* sl_struct = alloc->get<SimpleStruct>(c);
            sl_struct->check();
        }

        const ComplexStruct* cx_struct = alloc->get<ComplexStruct>(alloc->elements() - 1);

        for (int32_t c = 0; c < cx_struct->elements(); c++)
        {
            const SimpleStruct* sl_struct = cx_struct->get<SimpleStruct>(c);
            sl_struct->check();
        }
    }

    void dumpStructure(const Allocator* alloc)
    {
        alloc->dump();

        for (int32_t c = 0; c < alloc->elements() - 1; c++)
        {
            const SimpleStruct* sl_struct = alloc->get<SimpleStruct>(c);
            sl_struct->dump();
        }

        const ComplexStruct* cx_struct = alloc->get<ComplexStruct>(alloc->elements() - 1);

        cx_struct->dump();

        for (int32_t c = 0; c < cx_struct->elements(); c++)
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

        int32_t size0   = alloc->element_size(0);
        int32_t offset2 = alloc->element_offset(2);

        sl_struct->enlarge(100);

        AssertEQ(MA_SRC, alloc->element_offset(2), offset2 + alloc->element_size(0) - size0);

        checkStructure(alloc);

        sl_struct->shrink(100);

        int32_t size0_delta = alloc->element_size(0) - size0;

        AssertEQ(MA_SRC, alloc->element_offset(2) - size0_delta, offset2);

        AssertThrows<PackedOOMException>(MA_SRC, [alloc](){
            alloc->enlarge(10000);
        });

        AssertThrows<PackedOOMException>(MA_SRC, [sl_struct](){
            sl_struct->enlarge(10000);
        });

        AssertThrows<Exception>(MA_SRC, [sl_struct](){
            sl_struct->enlarge(-300);
        });
    }
};


}}
