
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
struct IOSubstreamAdapter<ICtrApiSubstream<DataType, io::ColumnWise1D, ValueCodec>, false>
{
    using ValueView = typename DataTypeTraits<DataType>::ViewType;
    using Codec     = ValueCodec<ValueView>;
    using AtomType  = typename Codec::BufferType;

    using SubstreamT = io::IOVLen1DArraySubstream<DataType>;


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


    static void read_to(const io::IOSubstream& substream, int32_t column, int32_t start, int32_t size, ArenaBuffer<ValueView>& buffer)
    {
        const auto& subs = io::substream_cast<SubstreamT>(substream);
        subs.read_to(start, size, buffer);
    }


    static void read_to(const io::IOSubstream& substream, int32_t column, int32_t start, int32_t size, ArenaBuffer<AtomType>& buffer)
    {
        const auto& subs = io::substream_cast<SubstreamT>(substream);
        const auto* data = subs.data_for(start);
        buffer.append_values(Span<const AtomType>(data, size));
    }

    template <typename ItemView>
    static void read_one(const io::IOSubstream& substream, int32_t column, int32_t idx, ItemView& item)
    {
        const auto& subs = io::substream_cast<SubstreamT>(substream);

        item = subs.get(idx);
    }

    void reset(io::IOSubstream& substream) {
        this->substream_ = &io::substream_cast<SubstreamT>(substream);
    }

    void append(ValueView view) {
        substream_->append_from(Span<const ValueView>(&view, 1));
    }


    template <typename Buffer>
    void append_buffer(const Buffer& buffer) {
        substream_->append_from(buffer.span());
    }

    size_t size() const {
        return substream_->size();
    }
};


}}
