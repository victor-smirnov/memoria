
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

#include <memoria/v1/api/common/ctr_api_btss.hpp>

#include <memoria/v1/api/datatypes/traits.hpp>
#include <memoria/v1/api/datatypes/encoding_traits.hpp>
#include <memoria/v1/api/datatypes/io_vector_traits.hpp>

#include <memoria/v1/core/iovector/io_vector.hpp>

#include <memoria/v1/core/tools/bitmap.hpp>

namespace memoria {
namespace v1 {





template <typename DataType, template <typename CxxType> class ValueCodec>
struct IOSubstreamAdapter<ICtrApiSubstream<DataType, io::ColumnWise, ValueCodec>, false>
{
    using ValueView = typename DataTypeTraits<DataType>::ViewType;
    using Codec     = ValueCodec<ValueView>;
    using AtomType  = typename Codec::BufferType;

    using SubstreamT = typename io::IOSubstreamInterfaceTF<
        ValueView,
        true, // ColumnWise
        false // VLen
    >::Type;

    io::DefaultIOBuffer<int32_t> lengths_;
    size_t size_{};
    SubstreamT* substream_{};
    int32_t column_;

    IOSubstreamAdapter(int32_t column):
        column_(column)
    {}

    static const auto* select(const io::IOSubstream& substream, int32_t column, int32_t row)
    {
        const auto& subs = io::substream_cast<SubstreamT>(substream);
        return subs.select(column, row);
    }


    static void read_to(const io::IOSubstream& substream, int32_t column, int32_t start, int32_t size, io::DefaultIOBuffer<ValueView>& buffer)
    {
        const auto& subs = io::substream_cast<SubstreamT>(substream);
        io::ConstVLenArrayColumnMetadata descr = subs.select_and_describe(column, start);

        if (start + size <= descr.size)
        {
            Codec codec;
            size_t pos = 0;
            for (int32_t c = 0; c < size; c++)
            {
                ValueView vv;
                size_t len = codec.decode(descr.data_buffer, vv, pos);

                buffer.emplace_back(std::move(vv));

                pos += len;
            }
        }
        else {
            MMA1_THROW(RuntimeException())
                    << fmt::format_ex(
                           u"Substream range check: start = {}, size = {}, size = {}",
                           start, size, descr.size
                       );
        }
    }


    static void read_to(const io::IOSubstream& substream, int32_t column, int32_t start, int32_t size, io::DefaultIOBuffer<AtomType>& buffer)
    {
        const auto& subs = io::substream_cast<SubstreamT>(substream);
        io::ConstVLenArrayColumnMetadata descr = subs.select_and_describe(column, start);
        buffer.append_values(Span<const AtomType>(descr.data_buffer, size));
    }

    template <typename ItemView>
    static void read_one(const io::IOSubstream& substream, int32_t column, int32_t idx, ItemView& item)
    {
        const auto& subs = io::substream_cast<SubstreamT>(substream);
        io::ConstVLenArrayColumnMetadata descr = subs.select_and_describe(column, idx);

        if (idx + 1 <= descr.size)
        {
            Codec codec;
            codec.decode(descr.data_buffer, item, 0);
        }
        else {
            MMA1_THROW(RuntimeException())
                    << fmt::format_ex(
                           u"Substream range check: start = {}, size = {}, size = {}",
                           idx, 1, descr.size
                       );
        }
    }

    void reset(io::IOSubstream& substream) {
        size_ = 0;
        this->substream_ = &io::substream_cast<SubstreamT>(substream);
    }

    void append(ValueView view) {
        append_buffer(Span<const ValueView>(&view, 1), 0, 1);
    }


    template <typename Buffer>
    void append_buffer(const Buffer& buffer) {
        append_buffer(buffer, 0, buffer.size());
    }


    template <typename Buffer>
    void append_buffer(const Buffer& buffer, size_t start, size_t size)
    {
        size_ += size;

        Codec codec;

        lengths_.clear();
        lengths_.ensure(size);

        for (size_t c = 0; c < size; c++)
        {
            lengths_.append_value(codec.length(buffer[c]));
        }

        auto* ptr = substream_->reserve(column_, size, lengths_.tail_ptr());

        size_t pos = 0;
        for (size_t c = 0; c < size; c++)
        {
            size_t len = codec.encode(ptr, buffer[start + c], pos);
            pos += len;
        }
    }
};


}}
