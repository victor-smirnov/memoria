
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
struct IOSubstreamAdapter<ICtrApiSubstream<DataType, io::RowWise, ValueCodec>, false>
{
    using ValueView = typename DataTypeTraits<DataType>::ViewType;
    using Codec = ValueCodec<ValueView>;

    using SubstreamT = typename io::IOSubstreamInterfaceTF<
        ValueView,
        false, // RowWise
        false // Vlen
    >::Type;

    io::DefaultIOBuffer<int32_t> lengths_;

    template <typename Buffer>
    static void read_to(const io::IOSubstream& substream, int32_t column, int32_t start, int size, Buffer& buffer)
    {
        const auto& subs = io::substream_cast<SubstreamT>(substream);
        if (subs.columns() == 1)
        {
            if (start + size <= subs.size())
            {
                auto data_buffer = subs.select(start);

                Codec codec;
                size_t pos = 0;
                for (int32_t c = 0; c < size; c++)
                {
                    ValueView vv;
                    size_t len = codec.decode(data_buffer, vv, pos);

                    buffer.emplace_back(std::move(vv));

                    pos += len;
                }
            }
            else {
                MMA1_THROW(RuntimeException())
                        << fmt::format_ex(
                               u"Substream range check: start = {}, size = {}, size = {}",
                               start, size, subs.size()
                           );
            }
        }
        else {
            MMA1_THROW(RuntimeException())
                << WhatCInfo("Multicolumn RowWise substreams are not yet supported");
        }
    }

    template <typename Buffer>
    void write_from(io::IOSubstream& substream, int32_t column, Buffer& buffer, size_t start, size_t size)
    {
        const auto& subs = io::substream_cast<SubstreamT>(substream);

        if (subs.columns() == 1)
        {
            Codec codec;

            lengths_.clear();
            lengths_.ensure(size);

            for (size_t c = 0; c < size; c++)
            {
                lengths_.append_value(codec.length(buffer[c]));
            }

            auto* ptr = subs.reserve(size, lengths_.tail_ptr());

            size_t pos = 0;
            for (size_t c = 0; c < size; c++)
            {
                size_t len = codec.encode(ptr, buffer[start + c], pos);
                pos += len;
            }
        }
        else {
            MMA1_THROW(RuntimeException())
                << WhatCInfo("Multicolumn RowWise substreams are not yet supported");
        }
    }
};

}}
