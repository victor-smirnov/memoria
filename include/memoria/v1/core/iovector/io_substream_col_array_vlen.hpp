
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
class IOColumnwiseVLenArraySubstreamImpl final: public IOColumnwiseVLenArraySubstream<Value> {

    static constexpr int32_t BUFFER_SIZE = 256;

    VLenArrayColumnMetadata columns_[Columns]{};
    std::vector<int32_t> offsets_[Columns]{};

    mutable UniquePtr<int32_t> lengths_buffer_;

public:
    IOColumnwiseVLenArraySubstreamImpl(): IOColumnwiseVLenArraySubstreamImpl(64)
    {}

    IOColumnwiseVLenArraySubstreamImpl(int32_t initial_capacity): lengths_buffer_{nullptr, ::free}
    {
        for (int32_t c = 0; c < Columns; c++)
        {
            columns_[c].data_buffer  = allocate_system<uint8_t>(initial_capacity).release();
            columns_[c].size         = 0;
            columns_[c].data_size    = 0;
            columns_[c].data_buffer_size = initial_capacity;
        }
    }

    virtual ~IOColumnwiseVLenArraySubstreamImpl() noexcept
    {
        for (int32_t c = 0; c < Columns; c++)
        {
            free_system(columns_[c].data_buffer);
        }
    }

    ConstVLenArrayColumnMetadata describe(int32_t column) const
    {
        auto& col = columns_[column];
        return ConstVLenArrayColumnMetadata{
            col.data_buffer,
            col.data_size,
            col.data_buffer_size,
            col.size
        };
    }

    VLenArrayColumnMetadata describe(int32_t column)
    {
        return columns_[column];
    }

    int32_t columns() const
    {
        return Columns;
    }

    const std::type_info& content_type() const
    {
        return typeid(Value);
    }

    uint8_t* ensure(int32_t column, int32_t required) final
    {
        auto& col = columns_[column];

        if (col.data_size + required > col.data_buffer_size)
        {
            int32_t db_size = col.data_buffer_size;

            while (col.data_size + required > db_size)
            {
                db_size *= 2;
            }

            uint8_t* new_data_buffer = allocate_system<uint8_t>(db_size).release();
            std::memcpy(new_data_buffer, col.data_buffer, col.data_buffer_size);

            free_system(col.data_buffer);

            col.data_buffer      = new_data_buffer;
            col.data_buffer_size = db_size;
        }

        return col.data_buffer;
    }

    virtual uint8_t* reserve(int32_t column, int32_t size, const int32_t* lengths)
    {
        auto& col = columns_[column];

        int32_t offset0 = col.data_size;

        for (int32_t c = 0; c < size; c++) {
            offsets_[column].push_back(offset0);
            offset0 += lengths[c];
        }

        int32_t inserted_length = offset0 - col.data_size;

        int32_t last_data_size = col.data_size;

        if (MMA1_UNLIKELY(col.data_size + inserted_length > col.data_buffer_size)) {
            ensure(column, inserted_length);
        }

        col.size += size;
        col.data_size += inserted_length;

        return col.data_buffer + last_data_size;
    }


    void reset()
    {
        for (int32_t c = 0; c < Columns; c++)
        {
            columns_[c].size = 0;
            columns_[c].data_size = 0;
            offsets_[c].clear();
        }
    }

    uint8_t* select(int32_t column, int32_t idx)
    {
        return columns_[column].data_buffer + offsets_[column][idx];
    }

    const uint8_t* select(int32_t column, int32_t idx) const
    {
        return columns_[column].data_buffer + offsets_[column][idx];
    }


    VLenArrayColumnMetadata select_and_describe(int32_t column, int32_t idx)
    {
        auto& col = columns_[column];

        int32_t offs = offsets_[column][idx];
        return VLenArrayColumnMetadata{col.data_buffer + offs, col.data_size - offs, col.data_buffer_size - offs, col.size};
    }

    ConstVLenArrayColumnMetadata select_and_describe(int32_t column, int32_t idx) const
    {
        auto& col = columns_[column];

        int32_t offs = offsets_[column][idx];
        return ConstVLenArrayColumnMetadata{col.data_buffer + offs, col.data_size - offs, col.data_buffer_size - offs, col.size};
    }

    virtual void get_lengths_to(int32_t column, int32_t start, int32_t size, int32_t* array) const
    {
        const auto& offsets = offsets_[column];

        int32_t idx = start;
        for (int32_t c = 0; c < size; c++, idx++) {
            array[c] = offsets[idx + 1] - offsets[idx];
        }
    }

    virtual void copy_to(IOSubstream& target, int32_t start, int32_t length) const
    {
        init_buffers();

        auto& tgt_substream = substream_cast<
            IOColumnwiseVLenArraySubstream<Value>
        >(target);

        FixedSizeSpliterator<int32_t, BUFFER_SIZE> splits(length);

        while (splits.has_more())
        {
            int32_t at = start + splits.split_start();

            for (int32_t col = 0; col < Columns; col++)
            {
                get_lengths_to(col, at, splits.split_size(), lengths_buffer_.get());

                const auto* src_buffer  = select(col, at);
                std::ptrdiff_t buf_size = select(col, start + splits.split_end()) - src_buffer;

                auto* tgt_buffer = tgt_substream.reserve(col, splits.split_size(), lengths_buffer_.get());

                MemCpyBuffer(src_buffer, tgt_buffer, buf_size);
            }

            splits.next_split();
        }
    }


    virtual void reindex()
    {
        for (int32_t column = 0; column < Columns; column++)
        {
            offsets_[column].push_back(columns_[column].data_size);
        }
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
