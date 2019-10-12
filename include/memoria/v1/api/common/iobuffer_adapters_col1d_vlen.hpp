
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

#include <memoria/v1/api/datatypes/buffer/buffer_generic.hpp>

namespace memoria {
namespace v1 {


template <typename DataType, template <typename CxxType> class ValueCodec>
struct IOSubstreamAdapter<ICtrApiSubstream<DataType, io::ColumnWise1D, ValueCodec>, false>
{
    using ViewType = typename DataTypeTraits<DataType>::ViewType;
    using AtomType  = typename DataTypeTraits<DataType>::AtomType;

    using SubstreamT = io::IO1DArraySubstreamView<DataType>;


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


    static void read_to(
            const io::IOSubstream& substream,
            int32_t column,
            int32_t start,
            int32_t size,
            ArenaBuffer<ViewType>& buffer
    )
    {
        const auto& subs = io::substream_cast<SubstreamT>(substream);
        subs.read_to(start, size, buffer);
    }


    static void read_to(
            const io::IOSubstream& substream,
            int32_t column,
            int32_t start,
            int32_t size,
            ArenaBuffer<AtomType>& buffer
    )
    {
        const auto& subs = io::substream_cast<SubstreamT>(substream);
        subs.read_to(start, size, buffer);
    }

    static void read_to(
            const io::IOSubstream& substream,
            int32_t column,
            int32_t start,
            int32_t size,
            DataTypeBuffer<DataType>& buffer
    )
    {
        const auto& subs = io::substream_cast<SubstreamT>(substream);
        subs.read_to(start, size, buffer);
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

    void append(ViewType view) {
        substream_->append_from(Span<const ViewType>(&view, 1));
    }

    void append_buffer(Span<const ViewType> buffer) {
        substream_->append_from(buffer);
    }

    template <typename ValueType>
    void append_buffer(Span<const ValueType> buffer) {
        for (auto& value: buffer) {
            append(value);
        }
    }

    template <typename ValueType>
    void append_buffer(const std::vector<ValueType> buffer) {
        for (auto& value: buffer) {
            append(value);
        }
    }

    template <typename ValueType>
    void append(Span<const ValueType> buffer) {
        for (auto& value: buffer) {
            append(value);
        }
    }

    void append(Span<const ViewType> buffer) {
        substream_->append_from(buffer);
    }

    size_t size() const {
        return substream_->size();
    }
};


}}
