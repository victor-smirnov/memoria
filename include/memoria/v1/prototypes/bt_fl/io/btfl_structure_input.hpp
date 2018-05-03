
// Copyright 2016 Victor Smirnov
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

#include <memoria/v1/core/types.hpp>
#include <memoria/v1/core/memory/malloc.hpp>

#include <memoria/v1/core/packed/tools/packed_dispatcher.hpp>
#include <memoria/v1/core/packed/sseq/packed_rle_searchable_seq.hpp>

#include <memoria/v1/prototypes/bt/layouts/bt_input.hpp>
#include <memoria/v1/prototypes/bt_fl/btfl_tools.hpp>

#include <memoria/v1/prototypes/bt_fl/io/btfl_input_tools.hpp>

#include <memory>

namespace memoria {
namespace v1 {
namespace btfl {
namespace io {



template <typename DataBuffer, typename CtrSizeT>
struct StructureStreamInputBuffer {
    using Buffer    = DataBuffer;
protected:

    using BufferT       = typename DataBuffer::PtrType;
    using BufferSizes   = typename BufferT::BufferSizesT;

    DataBuffer  buffer_;
    BufferT*    buffer_ptr_;

    int32_t last_symbol_    = -1;
    uint64_t run_length_ = 0;




public:
    StructureStreamInputBuffer(){}

    void init(DataBuffer&& buffer)
    {
        buffer_     = std::move(buffer);
        buffer_ptr_ = buffer_.get();
    }

    void init(int32_t capacity)
    {
        init(create_input_buffer(256));
    }


    DataBuffer& buffer() {return buffer_;}
    const DataBuffer& buffer() const {return buffer_;}


    void reset()
    {
        if (!buffer_.is_null())
        {
            buffer_->reset();
        }

        last_symbol_ = -1;
        run_length_  = 0;
    }

    void finish()
    {
        if (last_symbol_ >= 0)
        {
            flush_run();
            last_symbol_ = -1;
        }

        buffer_->reindex();
    }

    void append_run(int32_t symbol, uint64_t run_length)
    {
        if (symbol == last_symbol_ || last_symbol_ < 0)
        {
            last_symbol_ = symbol;
            run_length_ += run_length;
        }
        else
        {
            flush_run();

            last_symbol_ = symbol;
            run_length_  = run_length;
        }
    }


    BufferSizes data_capacity() const
    {
        return buffer_->data_capacity();
    }

    auto* create_input_buffer(const BufferSizes& buffer_sizes)
    {
        int32_t block_size  = BufferT::block_size(buffer_sizes);
        BufferT* buffer     = allocate_system<BufferT>(block_size).release();
        if (buffer)
        {
            buffer->setTopLevelAllocator();
            OOM_THROW_IF_FAILED(buffer->init(block_size, buffer_sizes), MMA1_SRC);
            return buffer;
        }
        else {
            MMA1_THROW(OOMException());
        }
    }

    auto* create_input_buffer(int32_t buffer_size)
    {
        int32_t block_size  = BufferT::block_size(buffer_size) + 500;
        BufferT* buffer     = allocate_system<BufferT>(block_size).release();
        if (buffer)
        {
            buffer->setTopLevelAllocator();
            OOM_THROW_IF_FAILED(buffer->init(block_size, buffer_size), MMA1_SRC);
            return buffer;
        }
        else {
            MMA1_THROW(OOMException());
        }
    }

private:




    void flush_run()
    {
        if (run_length_ > 0 && !buffer_->emplace_back_symbols_run(last_symbol_, run_length_))
        {
            enlarge();

            if (!buffer_->emplace_back_symbols_run(last_symbol_, run_length_))
            {
                MMA1_THROW(Exception()) << WhatCInfo("Symbols run entry is too large for RLE Sequence");
            }
        }
    }


protected:

    void enlarge()
    {
        BufferSizes current_capacity    = data_capacity();
        BufferSizes new_capacity        = current_capacity;
        VectorAdd(new_capacity, new_capacity);

        auto new_buffer = create_input_buffer(new_capacity);

        OOM_THROW_IF_FAILED(buffer_.get()->copyTo(new_buffer), MMA1_SRC);

        init(DataBuffer(new_buffer));
    }
};







}}}}
