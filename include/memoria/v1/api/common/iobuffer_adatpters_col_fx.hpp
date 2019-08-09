
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
struct IOSubstreamAdapter<ICtrApiSubstream<DataType, io::ColumnWise, ValueCodec>, true>
{
    using ValueView = typename DataTypeTraits<DataType>::ViewType;
    using ValueType = typename DataTypeTraits<DataType>::ValueType;

    using SubstreamT = typename io::IOSubstreamInterfaceTF<
        ValueView,
        true, // ColumnWise
        true // FixedSize
    >::Type;


    template <typename BufferView>
    static void read_to(const io::IOSubstream& substream, int32_t column, int32_t start, int size, BufferView& buffer)
    {
        const auto& subs = io::substream_cast<SubstreamT>(substream);
        io::FixedSizeArrayColumnMetadata<const ValueView> descr = subs.describe(column);

        if (start + size <= descr.size)
        {
            buffer = BufferView(descr.data_buffer + start, size);
        }
        else {
            MMA1_THROW(RuntimeException())
                    << fmt::format_ex(
                           u"Substream range check: start = {}, size = {}, size = {}",
                           start, size, descr.size
                       );
        }
    }

    void write_from(io::IOSubstream& substream, int32_t column, Span<const ValueType> buffer, size_t start, size_t size)
    {
        const auto& subs = io::substream_cast<SubstreamT>(substream);

        auto* ptr = subs.reserve(column, size);

        MemCpyBuffer(buffer.data() + start, ptr, size);
    }

};


}}
