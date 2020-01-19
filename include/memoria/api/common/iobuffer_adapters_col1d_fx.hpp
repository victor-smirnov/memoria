
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

#include <memoria/api/common/ctr_api_btss.hpp>

#include <memoria/core/datatypes/traits.hpp>
#include <memoria/core/datatypes/encoding_traits.hpp>
#include <memoria/core/datatypes/io_vector_traits.hpp>

#include <memoria/core/iovector/io_vector.hpp>

#include <memoria/core/tools/bitmap.hpp>

namespace memoria {


template <typename DataType, template <typename CxxType> class ValueCodec>
struct IOSubstreamAdapter<ICtrApiSubstream<DataType, io::ColumnWise1D, ValueCodec>, true>
{
    using ViewType = typename DataTypeTraits<DataType>::ViewType;
    using AtomType  = ViewType;

    using SubstreamT = io::IO1DArraySubstreamView<DataType>;

    size_t size_{};
    SubstreamT* substream_{};
    int32_t column_;

    IOSubstreamAdapter(int32_t column):
        column_(column)
    {}


    static void read_to(const io::IOSubstream& substream, int32_t column, int32_t start, int size, Span<const ViewType>& buffer)
    {
        const auto& subs = io::substream_cast<SubstreamT>(substream);
        buffer = subs.span(start, size);
    }

    static void read_to(const io::IOSubstream& substream, int32_t column, int32_t start, int32_t size, ArenaBuffer<ViewType>& buffer)
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
        size_ = 0;
        this->substream_ = &io::substream_cast<SubstreamT>(substream);
    }



    size_t size() const {
        return size_;
    }

};


}
