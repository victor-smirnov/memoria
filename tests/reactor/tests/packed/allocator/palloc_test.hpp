
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

#include <memoria/tests/tests.hpp>
#include <memoria/tests/assertions.hpp>

#include <memoria/core/memory/malloc.hpp>
#include <memoria/core/memory/ptr_cast.hpp>
#include <memoria/core/packed/tools/packed_allocator.hpp>
#include <memoria/reactor/reactor.hpp>

#include <memory>

namespace memoria {
namespace tests {

class PackedAllocatorTest: public TestState {

    using MyType = PackedAllocatorTest;
    using Base = TestState;

    using Allocator = PackedAllocator;

    using AllocatorPtr = std::shared_ptr<Allocator>;

    class SimpleStruct {
        PackedAllocatable header_;
        size_t size_;
        uint8_t data_;
        uint8_t content_[];
    public:
        SimpleStruct() {}

        static size_t block_size(size_t size) {
            return sizeof(SimpleStruct) + size;
        }

        size_t block_size() const
        {
            const Allocator* alloc = header_.allocator();
            assert_equals(true, alloc != nullptr);

            return alloc->element_size_by_ptr(this);
        }

        size_t object_size() const {
            return sizeof(MyType) + size_;
        }

        size_t size() const {
            return size_;
        }

        VoidResult init(size_t block_size) noexcept
        {
            size_ = block_size - sizeof(SimpleStruct);
            return VoidResult::of();
        }

        void fill(uint8_t data)
        {
            data_ = data;

            for (size_t c = 0; c < size_; c++)
            {
                content_[c] = data_;
            }
        }

        void check() const
        {
            for (size_t c = 0; c < size_; c++)
            {
                assert_equals(content_[c], data_, "idx = {}", c);
            }
        }

        void dump(std::ostream& out) const
        {
            out << "size_ = " << size_ << std::endl;
            out << "data_ = " << std::hex << (size_t)data_ << std::dec << std::endl;
            out << "Data:" << std::endl;

            dumpArray<uint8_t>(out, size_, [this](size_t idx) {
                return this->content_[idx];
            });
        }

        void enlarge(size_t delta)
        {
            Allocator* alloc = header_.allocator();
            size_t block_size   = alloc->element_size_by_ptr(this);
            size_t new_size     = alloc->resize_block(this, block_size + delta).get_or_throw();
            init(new_size).get_or_throw();
            fill(data_);
        }

        void shrink(size_t delta)
        {
            Allocator* alloc = header_.allocator();
            size_t block_size   = alloc->element_size_by_ptr(this);
            size_t new_size     = alloc->resize_block(this, block_size - delta).get_or_throw();

            init(new_size).get_or_throw();
            fill(data_);
        }
    };

    class ComplexStruct: public PackedAllocator {
    public:
        ComplexStruct() {}

        static size_t block_size(size_t client_area) {
            return PackedAllocator::block_size(client_area, 3);
        }

        VoidResult init(size_t block_size) noexcept
        {
            return PackedAllocator::init(block_size, 3);
        }
    };

public:

    static void init_suite(TestSuite& suite)
    {
        MMA_CLASS_TESTS(suite, testCreate, testEnlarge);
    }

    AllocatorPtr createAllocator(size_t client_area_size, size_t elements)
    {
        size_t block_size = Allocator::block_size(client_area_size, elements);

        Allocator* alloc = allocate_system_zeroed<Allocator>(block_size).release();

        alloc->init(block_size, elements).get_or_throw();
        alloc->allocatable().setTopLevelAllocator();

        assert_equals(alloc->elements(), elements);
        assert_equals(alloc->block_size(), block_size);
        assert_ge(alloc->block_size(), client_area_size);

        assert_gt(alloc->layout_size(), 0);
        assert_gt(alloc->bitmap_size(), 0);

        assert_ge(alloc->client_area(), client_area_size);

        return AllocatorPtr(alloc, free_system);
    }

    AllocatorPtr createSampleAllocator(size_t client_area_size, size_t elements)
    {
        AllocatorPtr alloc_ptr  = createAllocator(client_area_size, elements);
        Allocator*   alloc      = alloc_ptr.get();

        for (size_t c = 0; c < 2; c++)
        {
            size_t size = SimpleStruct::block_size(111 + c*10);

            SimpleStruct* obj = alloc->allocate<SimpleStruct>(c, size).get_or_throw();
            assert_ge(alloc->element_size(c), size);

            obj->fill(0x55 + c * 16);

            assert_ge(obj->size(), 111 + c*10);
        }

        AllocationBlock block = alloc->allocate(2, 512, PackedBlockType::ALLOCATABLE).get_or_throw();

        ComplexStruct* cx_struct = ptr_cast<ComplexStruct>(block.ptr());

        cx_struct->init(512).get_or_throw();

        for (size_t c = 0; c < cx_struct->elements(); c++)
        {
            SimpleStruct* sl_struct = cx_struct->allocate<SimpleStruct>(c, 100).get_or_throw();

            sl_struct->fill(0x22 + c*16);
        }

        return alloc_ptr;
    }

    void checkStructure(const Allocator* alloc)
    {
        for (size_t c = 0; c < alloc->elements() - 1; c++)
        {
            const SimpleStruct* sl_struct = alloc->get<SimpleStruct>(c);
            sl_struct->check();
        }

        const ComplexStruct* cx_struct = alloc->get<ComplexStruct>(alloc->elements() - 1);

        for (size_t c = 0; c < cx_struct->elements(); c++)
        {
            const SimpleStruct* sl_struct = cx_struct->get<SimpleStruct>(c);
            sl_struct->check();
        }
    }

    void dumpStructure(const Allocator* alloc)
    {
        alloc->dump();

        for (size_t c = 0; c < alloc->elements() - 1; c++)
        {
            const SimpleStruct* sl_struct = alloc->get<SimpleStruct>(c);
            sl_struct->dump(std::cout);
        }

        const ComplexStruct* cx_struct = alloc->get<ComplexStruct>(alloc->elements() - 1);

        cx_struct->dump();

        for (size_t c = 0; c < cx_struct->elements(); c++)
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

        size_t size0   = alloc->element_size(0);
        size_t offset2 = alloc->element_offset(2);

        sl_struct->enlarge(100);

        assert_equals(alloc->element_offset(2), offset2 + alloc->element_size(0) - size0);

        checkStructure(alloc);

        sl_struct->shrink(100);

        size_t size0_delta = alloc->element_size(0) - size0;

        assert_equals(alloc->element_offset(2) - size0_delta, offset2);

        assert_fails([alloc](){
            return alloc->enlarge(10000);
        });

        assert_fails([sl_struct](){
            return sl_struct->enlarge(10000);
        });

        assert_fails([sl_struct](){
            return sl_struct->shrink(300);
        });
    }
};


}}
