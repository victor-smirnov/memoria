
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

#include <memoria/v1/core/packed/tools/packed_allocator_types.hpp>
#include <memoria/v1/core/tools/accessors.hpp>
#include <memoria/v1/core/tools/assert.hpp>
#include <memoria/v1/core/tools/iobuffer/io_buffer.hpp>

namespace memoria {
namespace v1 {



template <
    typename V,
    Int Blocks_ = 1
>
struct PackedFSERowOrderInputBufferTypes {
    typedef V                   Value;
    static const Int Blocks     = Blocks_;
};



template <typename Types_>
class PackedFSERowOrderInputBuffer: public PackedAllocatable {

    typedef PackedAllocatable                                                   Base;

public:
    static const UInt VERSION                                                   = 1;

    typedef Types_                                                              Types;
    typedef PackedFSERowOrderInputBuffer<Types>                                 MyType;

    typedef PackedAllocator                                                     Allocator;
    typedef typename Types::Value                                               Value;

    static constexpr Int Indexes    = 0;
    static constexpr Int Blocks     = Types::Blocks;

    static constexpr Int SafetyMargin = 0;

    using InputType = Value;
    using InputBuffer = MyType;

    using Values = core::StaticVector<Value, Blocks>;
    using SizesT = core::StaticVector<Int, Blocks>;


    class AppendState {
        Int size_;
    public:
        AppendState(): size_(0) {}
        AppendState(Int size): size_(size) {}

        Int& size() {return size_;}
        const Int& size() const {return size_;}
    };

private:

    Int size_;
    Int max_size_;

    Value buffer_[];

public:
    PackedFSERowOrderInputBuffer() {}

    Int& size() {return size_;}
    const Int& size() const {return size_;}

    Int& max_size() {return max_size_;}
    const Int& max_size() const {return max_size_;}

    Int capacity() const {return max_size_ - size_;}

    Int total_capacity() const
    {
        Int my_size     = allocator()->element_size(this);
        Int free_space  = allocator()->free_space();
        Int data_size   = sizeof(Value) * size_ * Blocks;

        return (my_size + free_space - data_size) / (sizeof(Value) * Blocks);
    }

    Int block_size() const
    {
        return sizeof(MyType) + max_size_ * sizeof(Value) * Blocks;
    }


public:

    static constexpr Int block_size(Int array_size)
    {
        return PackedAllocator::roundUpBytesToAlignmentBlocks(sizeof(MyType) + array_size * sizeof(Value) * Blocks);
    }

    static constexpr Int block_size(const SizesT& array_size)
    {
        return PackedAllocator::roundUpBytesToAlignmentBlocks(sizeof(MyType) + array_size[0] * sizeof(Value) * Blocks);
    }

    void init(Int block_size)
    {
        size_ = 0;
        max_size_ = max_size_for(block_size);
    }

    void init(const SizesT& capacities)
    {
        size_ = 0;
        max_size_ = capacities[0];
    }

    SizesT data_capacity() const
    {
        return SizesT(max_size_);
    }

    void copyTo(MyType* other) const
    {
        other->size() = this->size();
        CopyBuffer(buffer_, other->buffer_, size_ * Blocks);
    }


    static constexpr Int max_size_for(Int block_size) {
        return (block_size - empty_size()) / (sizeof(Value) * Blocks);
    }

    bool has_capacity_for(const SizesT& sizes) const
    {
        return sizes[0] <= max_size_;
    }

    template <typename SizesBuffer>
    bool has_capacity_for(const SizesBuffer& sizes, int start, int length) const
    {
        return length <= max_size_;
    }

    static constexpr Int empty_size()
    {
        return sizeof(MyType);
    }


    Value& value(Int block, Int idx) {
        return buffer_[idx * Blocks + block];
    }

    const Value& value(Int block, Int idx) const {
        return buffer_[idx * Blocks + block];
    }




    const Value* data() const {
        return buffer_;
    }

    Value* values() {
        return buffer_;
    }

    const Value* values() const {
        return buffer_;
    }





    void reindex() {}
    void check() const {}


    template <typename Adaptor>
    static SizesT calculate_size(Int size, Adaptor&& fn)
    {
        return SizesT(size);
    }




    void reset()
    {
        size_ = 0;
    }


    SizesT positions(Int idx) const {
        return SizesT(idx);
    }



    AppendState append_state()
    {
        return AppendState(size_);
    }


    template <typename IOBuffer>
    bool append_entry_from_iobuffer(AppendState& state, IOBuffer& buffer)
    {
        for (Int block = 0; block < Blocks; block++)
        {
            int capacity = max_size_ - size_;
            int len = sizeof(Value);

            if (len <= capacity)
            {
                this->value(block, size_) = IOBufferAdapter<Value>::get(buffer);
            }
            else {
                return false;
            }
        }

        state.size()++;
        this->size()++;

        return true;
    }



    void restore(const AppendState& state)
    {
        this->size_ = state.size();
    }





    template <typename Adaptor>
    Int append(Int size, Adaptor&& adaptor)
    {
        auto values = this->values();

        Int start = size_;
        Int limit = (start + size) <= max_size_ ? size : max_size_ - start;

        for (Int c = 0; c < limit; c++)
        {
            for (Int block = 0; block < Blocks; block++)
            {
                const auto& value = adaptor(block, c);
                values[(start + c) * Blocks + block] = value;
            }
        }

        size_ += limit;

        return limit;
    }







    template <typename Fn>
    SizesT scan(Int start, Int end, Fn&& fn) const
    {
        MEMORIA_V1_ASSERT(start, <=, size_);
        MEMORIA_V1_ASSERT(start, >=, 0);
        MEMORIA_V1_ASSERT(end, >=, 0);
        MEMORIA_V1_ASSERT(end, <=, size_);

        auto values = this->values();

        for (Int c = start; c < end; c++)
        {
            Values item;
            for (Int b = 0; b < Blocks; b++) {
                item[b] = values[c * Blocks + b];
            }

            fn(item);
        }

        return SizesT(end);
    }


    // ==================================== Dump =========================================== //


    void dump(std::ostream& out = cout) const
    {
        TextPageDumper dumper(out);
        generateDataEvents(&dumper);
    }

    void generateDataEvents(IPageDataEventHandler* handler) const
    {
        handler->startStruct();
        handler->startGroup("FSE_ROW_ORDER_INPUT_BUFFER");

        handler->value("ALLOCATOR",     &Base::allocator_offset());
        handler->value("SIZE",          &size_);
        handler->value("MAX_SIZE",      &max_size_);

        handler->startGroup("DATA", size_);

        if (Blocks == 1)
        {
            ValueHelper<Value>::setup(handler, "DATA_ITEM", buffer_, size_ * Blocks, IPageDataEventHandler::BYTE_ARRAY);
        }
        else {
            auto values = this->values();

            for (Int c = 0; c < size_; c++)
            {
                handler->value("DATA_ITEM", PageValueProviderFactory::provider(Blocks, [&](Int idx) {
                    return values + c * Blocks + idx;
                }));
            }
        }

        handler->endGroup();

        handler->endGroup();
        handler->endStruct();
    }

};





}}
