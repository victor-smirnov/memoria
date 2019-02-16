
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
#include <memoria/v1/tests/assertions.hpp>

#include <memoria/v1/core/memory/malloc.hpp>
#include <memoria/v1/core/packed/tools/packed_allocator.hpp>
#include <memoria/v1/reactor/reactor.hpp>

#include <memory>

namespace memoria {
namespace v1 {
namespace tests {

class PackedAllocatorTest: public TestState {

    using MyType = PackedAllocatorTest;
    using Base = TestState;

    using Allocator = PackedAllocator;

    using AllocatorPtr = std::shared_ptr<Allocator>;

    class SimpleStruct {
        PackedAllocatable header_;
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
            const Allocator* alloc = header_.allocator();
            assert_equals(true, alloc != nullptr);

            return alloc->element_size(this);
        }

        int32_t object_size() const {
            return sizeof(MyType) + size_;
        }

        int32_t size() const {
            return size_;
        }

        OpStatus init(int32_t block_size)
        {
            size_ = block_size - sizeof(SimpleStruct);
            return OpStatus::OK;
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
                assert_equals(content_[c], data_, u"idx = {}", c);
            }
        }

        void dump(std::ostream& out) const
        {
            out << "size_ = " << size_ << std::endl;
            out << "data_ = " << std::hex << (int32_t)data_ << std::dec << std::endl;
            out << "Data:" << std::endl;

            dumpArray<uint8_t>(out, size_, [this](int32_t idx) {
                return this->content_[idx];
            });
        }

        void enlarge(int32_t delta)
        {
            Allocator* alloc = header_.allocator();
            int32_t block_size   = alloc->element_size(this);
            int32_t new_size     = alloc->resizeBlock(this, block_size + delta);

            if (isFail(new_size)) {
                throw PackedOOMException(MMA1_SRC);
            }

            OOM_THROW_IF_FAILED(init(new_size), MMA1_SRC);
            fill(data_);
        }

        void shrink(int32_t delta)
        {
            Allocator* alloc = header_.allocator();
            int32_t block_size   = alloc->element_size(this);
            int32_t new_size     = alloc->resizeBlock(this, block_size - delta);

            OOM_THROW_IF_FAILED(init(new_size), MMA1_SRC);
            fill(data_);
        }
    };

    class ComplexStruct: public PackedAllocator {
    public:
        ComplexStruct() {}

        static int32_t block_size(int32_t client_area) {
            return PackedAllocator::block_size(client_area, 3);
        }

        OpStatus init(int32_t block_size)
        {
            return PackedAllocator::init(block_size, 3);
        }
    };

public:

    static void init_suite(TestSuite& suite)
    {
        MMA1_CLASS_TESTS(suite, testCreate, testEnlarge);
    }

    AllocatorPtr createAllocator(int32_t client_area_size, int32_t elements)
    {
        int32_t block_size = Allocator::block_size(client_area_size, elements);

        Allocator* alloc = allocate_system_zeroed<Allocator>(block_size).release();

        OOM_THROW_IF_FAILED(alloc->init(block_size, elements), MMA1_SRC);
        alloc->allocatable().setTopLevelAllocator();

        assert_equals(alloc->elements(), elements);
        assert_equals(alloc->block_size(), block_size);
        assert_ge(alloc->block_size(), client_area_size);

        assert_gt(alloc->layout_size(), 0);
        assert_gt(alloc->bitmap_size(), 0);

        assert_ge(alloc->client_area(), client_area_size);

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
            assert_ge(alloc->element_size(c), size);

            obj->fill(0x55 + c * 16);

            assert_ge(obj->size(), 111 + c*10);
        }

        AllocationBlock block = alloc->allocate(2, 512, PackedBlockType::ALLOCATABLE);

        ComplexStruct* cx_struct = T2T<ComplexStruct*>(block.ptr());

        OOM_THROW_IF_FAILED(cx_struct->init(512), MMA1_SRC);

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
            sl_struct->dump(std::cout);
        }

        const ComplexStruct* cx_struct = alloc->get<ComplexStruct>(alloc->elements() - 1);

        cx_struct->dump();

        for (int32_t c = 0; c < cx_struct->elements(); c++)
        {
            const SimpleStruct* sl_struct = cx_struct->get<SimpleStruct>(c);
            sl_struct->dump(std::cout);
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

        assert_equals(alloc->element_offset(2), offset2 + alloc->element_size(0) - size0);

        checkStructure(alloc);

        sl_struct->shrink(100);

        int32_t size0_delta = alloc->element_size(0) - size0;

        assert_equals(alloc->element_offset(2) - size0_delta, offset2);

        assert_throws<PackedOOMException>([alloc](){
            OOM_THROW_IF_FAILED(toOpStatus(alloc->enlarge(10000)), MMA1_SRC);
        });

        assert_throws<PackedOOMException>([sl_struct](){
            sl_struct->enlarge(10000);
        });

        assert_throws<Exception>([sl_struct](){
            sl_struct->enlarge(-300);
        });
    }
};


}}}
