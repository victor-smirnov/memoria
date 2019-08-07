
// Copyright 2019 Victor Smirnov
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

#include <memoria/v1/core/iovector/io_substream_array_vlen_base.hpp>

#include <memoria/v1/core/types/type2type.hpp>

#include <memoria/v1/core/exceptions/exceptions.hpp>
#include <memoria/v1/core/strings/format.hpp>

#include <memoria/v1/core/memory/malloc.hpp>
#include <memoria/v1/core/tools/bitmap.hpp>

#include <memoria/v1/core/tools/spliterator.hpp>

#include <memoria/v1/core/tools/bitmap.hpp>

#include <type_traits>
#include <cstring>

namespace memoria {
namespace v1 {
namespace io {


template <typename PkdStruct>
class IORowwiseVLenArraySubstreamViewImpl final: public IORowwiseVLenArraySubstream<typename PkdStruct::Value> {

    static constexpr int32_t BUFFER_SIZE = 256;

    PkdStruct* array_{};

    mutable UniquePtr<int32_t> lengths_buffer_;

public:
    IORowwiseVLenArraySubstreamViewImpl(): lengths_buffer_{nullptr, ::free}
    {

    }

    virtual ~IORowwiseVLenArraySubstreamViewImpl() noexcept {}

    void configure(PkdStruct* array) noexcept
    {
        array_ = array;
    }

    int32_t size() const {
        return array_->size();
    }

    int32_t columns() const
    {
        return PkdStruct::Blocks;
    }

    uint8_t* ensure(int32_t capacity) final
    {
        MMA1_THROW(UnsupportedOperationException());
    }

    virtual uint8_t* reserve(int32_t size, const int32_t* lengths) {
        MMA1_THROW(UnsupportedOperationException());
    }


    void reset()
    {
        MMA1_THROW(UnsupportedOperationException());
    }

    uint8_t* select(int32_t row) const
    {
        return array_->row_ptr(row);
    }

    uint8_t* select(int32_t row)
    {
        MMA1_THROW(UnsupportedOperationException());
    }

    void get_lengths_to(int32_t start, int32_t size, int32_t* array) const
    {
        array_->get_lengths_to(start, size, array);
    }

    void copy_to(IOSubstream& target, int32_t start, int32_t length) const
    {
        init_buffers();

        auto& tgt_substream = substream_cast<
                IORowwiseVLenArraySubstream<typename PkdStruct::Value>
        >(target);


        FixedSizeSpliterator<int32_t, BUFFER_SIZE> splits(length);

        while (splits.has_more())
        {
            int32_t at = start + splits.split_start();

            get_lengths_to(at, splits.split_size(), lengths_buffer_.get());

            const auto* src_buffer = select(at);
            std::ptrdiff_t buf_size = select(start + splits.split_end()) - src_buffer;

            auto* tgt_buffer = tgt_substream.reserve(splits.split_size(), lengths_buffer_.get());

            MemCpyBuffer(src_buffer, tgt_buffer, buf_size);

            splits.next_split();
        }
    }

    virtual void reindex() {}

private:
    void init_buffers() const
    {
        if (!lengths_buffer_)
        {
            lengths_buffer_ = allocate_system_zeroed<int32_t>(BUFFER_SIZE);
        }
    }
};


}}}
