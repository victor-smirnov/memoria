
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
#include <memoria/v1/core/iobuffer/io_buffer.hpp>

namespace memoria {
namespace v1 {



template <
    typename V,
    int32_t Blocks_ = 1
>
struct PackedFSERowOrderInputBufferTypes {
    typedef V                   Value;
    static const int32_t Blocks     = Blocks_;
};



template <typename Types_>
class PackedFSERowOrderInputBuffer: public PackedAllocatable {

    typedef PackedAllocatable                                                   Base;

public:
    static const uint32_t VERSION                                                   = 1;

    typedef Types_                                                              Types;
    typedef PackedFSERowOrderInputBuffer<Types>                                 MyType;

    typedef PackedAllocator                                                     Allocator;
    typedef typename Types::Value                                               Value;

    static constexpr int32_t Indexes    = 0;
    static constexpr int32_t Blocks     = Types::Blocks;

    static constexpr int32_t SafetyMargin = 0;

    using InputType = Value;
    using InputBuffer = MyType;

    using Values = core::StaticVector<Value, Blocks>;
    using SizesT = core::StaticVector<int32_t, Blocks>;


    class AppendState {
        int32_t size_;
    public:
        AppendState(): size_(0) {}
        AppendState(int32_t size): size_(size) {}

        int32_t& size() {return size_;}
        const int32_t& size() const {return size_;}
    };

private:

    int32_t size_;
    int32_t max_size_;

    Value buffer_[];

public:
    PackedFSERowOrderInputBuffer() {}

    int32_t& size() {return size_;}
    const int32_t& size() const {return size_;}

    int32_t& max_size() {return max_size_;}
    const int32_t& max_size() const {return max_size_;}

    int32_t capacity() const {return max_size_ - size_;}

    int32_t total_capacity() const
    {
        int32_t my_size     = allocator()->element_size(this);
        int32_t free_space  = allocator()->free_space();
        int32_t data_size   = sizeof(Value) * size_ * Blocks;

        return (my_size + free_space - data_size) / (sizeof(Value) * Blocks);
    }

    int32_t block_size() const
    {
        return sizeof(MyType) + max_size_ * sizeof(Value) * Blocks;
    }


public:

    static constexpr int32_t block_size(int32_t array_size)
    {
        return PackedAllocator::roundUpBytesToAlignmentBlocks(sizeof(MyType) + array_size * sizeof(Value) * Blocks);
    }

    static constexpr int32_t block_size(const SizesT& array_size)
    {
        return PackedAllocator::roundUpBytesToAlignmentBlocks(sizeof(MyType) + array_size[0] * sizeof(Value) * Blocks);
    }

    OpStatus init(int32_t block_size)
    {
        size_ = 0;
        max_size_ = max_size_for(block_size);

        return OpStatus::OK;
    }

    OpStatus init(const SizesT& capacities)
    {
        size_ = 0;
        max_size_ = capacities[0];

        return OpStatus::OK;
    }

    SizesT data_capacity() const
    {
        return SizesT(max_size_);
    }

    OpStatus copyTo(MyType* other) const
    {
        other->size() = this->size();
        CopyBuffer(buffer_, other->buffer_, size_ * Blocks);

        return OpStatus::OK;
    }


    static constexpr int32_t max_size_for(int32_t block_size) {
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

    static constexpr int32_t empty_size()
    {
        return sizeof(MyType);
    }


    Value& value(int32_t block, int32_t idx) {
        return buffer_[idx * Blocks + block];
    }

    const Value& value(int32_t block, int32_t idx) const {
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





    OpStatus reindex() {return OpStatus::OK;}
    void check() const {}


    template <typename Adaptor>
    static SizesT calculate_size(int32_t size, Adaptor&& fn)
    {
        return SizesT(size);
    }




    OpStatus reset()
    {
        size_ = 0;
        return OpStatus::OK;
    }


    SizesT positions(int32_t idx) const {
        return SizesT(idx);
    }



    AppendState append_state()
    {
        return AppendState(size_);
    }


    template <typename IOBuffer>
    bool append_entry_from_iobuffer(AppendState& state, IOBuffer& buffer)
    {
        for (int32_t block = 0; block < Blocks; block++)
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
    int32_t append(int32_t size, Adaptor&& adaptor)
    {
        auto values = this->values();

        int32_t start = size_;
        int32_t limit = (start + size) <= max_size_ ? size : max_size_ - start;

        for (int32_t c = 0; c < limit; c++)
        {
            for (int32_t block = 0; block < Blocks; block++)
            {
                const auto& value = adaptor(block, c);
                values[(start + c) * Blocks + block] = value;
            }
        }

        size_ += limit;

        return limit;
    }







    template <typename Fn>
    SizesT scan(int32_t start, int32_t end, Fn&& fn) const
    {
        MEMORIA_V1_ASSERT(start, <=, size_);
        MEMORIA_V1_ASSERT(start, >=, 0);
        MEMORIA_V1_ASSERT(end, >=, 0);
        MEMORIA_V1_ASSERT(end, <=, size_);

        auto values = this->values();

        for (int32_t c = start; c < end; c++)
        {
            Values item;
            for (int32_t b = 0; b < Blocks; b++) {
                item[b] = values[c * Blocks + b];
            }

            fn(item);
        }

        return SizesT(end);
    }


    // ==================================== Dump =========================================== //


    void dump(std::ostream& out = std::cout) const
    {
        TextPageDumper dumper(out);
        generateDataEvents(&dumper);
    }

    void generateDataEvents(IBlockDataEventHandler* handler) const
    {
        handler->startStruct();
        handler->startGroup("FSE_ROW_ORDER_INPUT_BUFFER");

        handler->value("ALLOCATOR",     &Base::allocator_offset());
        handler->value("SIZE",          &size_);
        handler->value("MAX_SIZE",      &max_size_);

        handler->startGroup("DATA", size_);

        if (Blocks == 1)
        {
            ValueHelper<Value>::setup(handler, "DATA_ITEM", buffer_, size_ * Blocks, IBlockDataEventHandler::BYTE_ARRAY);
        }
        else {
            auto values = this->values();

            for (int32_t c = 0; c < size_; c++)
            {
                handler->value("DATA_ITEM", BlockValueProviderFactory::provider(Blocks, [&](int32_t idx) {
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
