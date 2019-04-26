
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


#include <type_traits>
#include <cstring>

namespace memoria {
namespace v1 {
namespace io {

template <typename Value, int32_t Columns>
class IORowwiseVLenArraySubstreamImpl final: public IORowwiseVLenArraySubstream<Value> {

    uint8_t* data_buffer_{};
    int32_t data_size_{};
    int32_t data_buffer_size_{};
    int32_t size_{};

    std::vector<int32_t> row_offsets_{};

    static constexpr int32_t BUFFER_SIZE = 256;

    mutable UniquePtr<int32_t> lengths_buffer_;

public:
    IORowwiseVLenArraySubstreamImpl(): IORowwiseVLenArraySubstreamImpl(64)
    {}

    IORowwiseVLenArraySubstreamImpl(int32_t initial_capacity): lengths_buffer_(nullptr, ::free)
    {
        data_buffer_ = allocate_system<uint8_t>(initial_capacity).release();
        data_buffer_size_ = initial_capacity;
    }

    virtual ~IORowwiseVLenArraySubstreamImpl() noexcept
    {
        free_system(data_buffer_);
    }


    int32_t columns() const
    {
        return Columns;
    }

    uint8_t* ensure(int32_t capacity) final
    {
        if (data_size_ + capacity > data_buffer_size_)
        {
            int32_t db_size = data_buffer_size_;

            while (data_size_ + capacity > db_size)
            {
                db_size *= 2;
            }

            uint8_t* new_data_buffer = allocate_system<uint8_t>(db_size).release();
            std::memcpy(new_data_buffer, data_buffer_, data_buffer_size_);

            free_system(data_buffer_);

            data_buffer_      = new_data_buffer;
            data_buffer_size_ = db_size;
        }

        return data_buffer_;
    }

    virtual uint8_t* reserve(int32_t size, const int32_t* lengths)
    {
        int32_t offset0 = data_size_;

        for (int32_t c = 0; c < size; c++) {
            row_offsets_.push_back(offset0);
            offset0 += lengths[c];
        }

        int32_t inserted_length = offset0 - data_size_;

        int32_t last_data_size = data_size_;

        if (MMA1_UNLIKELY(data_size_ + inserted_length > data_buffer_size_)) {
            ensure(inserted_length);
        }

        size_ += size;
        data_size_ += inserted_length;

        return data_buffer_ + last_data_size;
    }


    void reset()
    {
        size_ = 0;
        data_size_ = 0;

        row_offsets_.clear();
    }

    const uint8_t* select(int32_t row) const
    {
        return data_buffer_ + row_offsets_[row];
    }

    uint8_t* select(int32_t row)
    {
        return data_buffer_ + row_offsets_[row];
    }

    virtual void get_lengths_to(int32_t start, int32_t size, int32_t* array) const
    {
        int32_t idx = start;
        for (int32_t c = 0; c < size; c++, idx++) {
            array[c] = row_offsets_[idx + 1] - row_offsets_[idx];
        }
    }



    virtual void copy_to(IOSubstream& target, int32_t start, int32_t length) const
    {
        init_buffers();

        auto& tgt_substream = substream_cast<IORowwiseVLenArraySubstream<Value>>(target);

        FixedSizeSpliterator<int32_t, BUFFER_SIZE> splits(length);

        while (splits.has_more())
        {
            int32_t at = start + splits.split_start();

            get_lengths_to(at, splits.split_size(), lengths_buffer_.get());

            const auto* src_buffer  = select(at);
            std::ptrdiff_t buf_size = select(start + splits.split_end()) - src_buffer;

            auto* tgt_buffer = tgt_substream.reserve(splits.split_size(), lengths_buffer_.get());

            MemCpyBuffer(src_buffer, tgt_buffer, buf_size);

            splits.next_split();
        }
    }

    virtual void reindex()
    {
        row_offsets_.push_back(data_size_);
    }

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
