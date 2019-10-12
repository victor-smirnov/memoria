
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

#include <memoria/v1/api/common/ctr_output_btfl_entries.hpp>
#include <memoria/v1/api/common/ctr_output_btfl_run.hpp>

#include <memoria/v1/core/exceptions/exceptions.hpp>
#include <memoria/v1/core/tools/static_array.hpp>
#include <memoria/v1/core/iovector/io_symbol_sequence.hpp>

#include <memory>
#include <tuple>
#include <functional>
#include <vector>

namespace memoria {
namespace v1 {

namespace _ {

    template <
        typename DataType,
        typename ViewType = typename DataTypeTraits<DataType>::ViewType,
        bool IsFixedSize = DataTypeTraits<DataType>::isFixedSize
    >
    class MMapSubstreamAdapter;

    template <typename DataType, typename ViewType>
    class MMapSubstreamAdapter<DataType, ViewType, true> {
        Span<const ViewType> array_;

    public:
        size_t size() const {return array_.size();}

        auto& array() {return array_;}
        const auto& array() const {return array_;}
        Span<const ViewType> span() const {return array_;}
        void clear() {
            array_ = Span<const ViewType>();
        }
    };


    template <typename DataType, typename ViewType>
    class MMapSubstreamAdapter<DataType, ViewType, false> {
        ArenaBuffer<ViewType> array_;

    public:
        size_t size() const {return array_.size();}

        auto& array() {return array_;}
        const auto& array() const {return array_;}
        Span<const ViewType> span() const {return array_.span();}
        void clear() {
            array_.clear();
        }
    };




    template <
        typename DataType,
        typename ViewType = typename DataTypeTraits<DataType>::ViewType,
        bool IsFixedSize = DataTypeTraits<DataType>::isFixedSize
    >
    class MMapValueGroupsAdapter;


    template <typename DataType, typename ViewType>
    class MMapValueGroupsAdapter<DataType, ViewType, true> {

        ArenaBuffer<Span<const ViewType>> array_;

    public:
        auto& array() {return array_;}
        const auto& array() const {return array_;}
        Span<const Span<const ViewType>> span() const {return array_.span();}
        void clear() {
            array_.clear();
        }

        template <typename SubstreamAdapter, typename Parser>
        auto populate(const io::IOSubstream& substream, const Parser& parser, int32_t values_offset, size_t start, size_t end)
        {
            const io::IO1DArraySubstreamView<DataType>& subs =
                    io::substream_cast<io::IO1DArraySubstreamView<DataType>>(substream);

            auto span = subs.span(values_offset, subs.size() - values_offset);

            const ViewType* values = span.data();

            size_t keys_num{};
            size_t values_end = values_offset;

            for (size_t c = start; c < end;)
            {
                auto& run = parser.buffer()[c];
                if (MMA1_LIKELY(run.symbol == 0))
                {
                    keys_num++;
                    uint64_t keys_run_length = run.length;
                    if (MMA1_LIKELY(keys_run_length == 1)) // typical case
                    {
                        auto& next_run = parser.buffer()[c + 1];

                        if (MMA1_LIKELY(next_run.symbol == 1))
                        {
                            uint64_t values_run_length = next_run.length;
                            array_.append_value(absl::Span<const ViewType>(values, values_run_length));

                            values += values_run_length;
                            values_end += values_run_length;
                            c += 2;
                        }
                        else {
                            // zero-length value
                            array_.append_value(absl::Span<const ViewType>());
                            c += 1;
                        }
                    }
                    else
                    {
                        // A series of keys with zero-length values
                        for (uint64_t s = 0; s < keys_run_length; s++)
                        {
                            array_.append_value(absl::Span<const ViewType>());
                        }

                        c += 1;
                    }
                }
                else {
                    MMA1_THROW(RuntimeException()) << WhatCInfo("Unexpected NON-KEY symbol in streams structure");
                }
            }

            return std::make_tuple(values_end, keys_num);
        }
    };


    template <typename DataType, typename ViewType>
    class MMapValueGroupsAdapter<DataType, ViewType, false>
    {
        ArenaBuffer<Span<const ViewType>> array_;
        ArenaBuffer<ViewType> values_;

    public:
        auto& array() {return array_;}
        const auto& array() const {return array_;}
        auto& values() {return values_;}
        const auto& values() const {return values_;}

        Span<const Span<const ViewType>> span() const {return array_.span();}
        void clear() {
            values_.clear();
            array_.clear();
        }

        template <typename SubstreamAdapter, typename Parser>
        auto populate(const io::IOSubstream& substream, const Parser& parser, int32_t values_offset, size_t start, size_t end)
        {
            uint64_t values_size{};
            for (size_t c = start; c < end;)
            {
                auto& run = parser.buffer()[c];
                if (MMA1_LIKELY(run.symbol == 0))
                {
                    uint64_t keys_run_length = run.length;
                    if (MMA1_LIKELY(keys_run_length == 1)) // typical case
                    {
                        auto& next_run = parser.buffer()[c + 1];

                        if (MMA1_LIKELY(next_run.symbol == 1))
                        {
                            uint64_t values_run_length = next_run.length;
                            values_size += values_run_length;
                            c += 2;
                        }
                        else {
                            // zero-length value
                            c += 1;
                        }
                    }
                    else
                    {
                        // A series of keys with zero-length values
                        c += 1;
                    }
                }
                else {
                    MMA1_THROW(RuntimeException()) << WhatCInfo("Unexpected NON-KEY symbol in streams structure");
                }
            }

            SubstreamAdapter::read_to(substream, 0, values_offset, values_size, values_);

            size_t keys_num{};
            size_t local_offset{};
            for (size_t c = start; c < end;)
            {
                auto& run = parser.buffer()[c];
                if (MMA1_LIKELY(run.symbol == 0))
                {
                    keys_num++;
                    uint64_t keys_run_length = run.length;
                    if (MMA1_LIKELY(keys_run_length == 1)) // typical case
                    {
                        auto& next_run = parser.buffer()[c + 1];

                        if (MMA1_LIKELY(next_run.symbol == 1))
                        {
                            uint64_t values_run_length = next_run.length;
                            array_.append_value(Span<const ViewType>(
                                values_.data() + local_offset, values_run_length
                            ));

                            local_offset += values_run_length;
                            c += 2;
                        }
                        else {
                            // zero-length value
                            array_.append_value(absl::Span<const ViewType>());
                            c += 1;
                        }
                    }
                    else
                    {
                        // A series of keys with zero-length values
                        for (uint64_t s = 0; s < keys_run_length; s++)
                        {
                            array_.append_value(absl::Span<const ViewType>());
                        }

                        c += 1;
                    }
                }
                else {
                    MMA1_THROW(RuntimeException()) << WhatCInfo("Unexpected NON-KEY symbol in streams structure");
                }
            }

            return std::make_tuple(values_offset + values_size, keys_num);
        }
    };



    template <
        typename DataType,
        typename AtomType,
        bool IsFixedSize = DataTypeTraits<DataType>::isFixedSize
    >
    class MMapValuesBufferAdapter;

    template <typename DataType, typename AtomType>
    class MMapValuesBufferAdapter<DataType, AtomType, true> {

        ArenaBuffer<AtomType> array_;

    public:
        Span<const AtomType> span() const {return array_.span();}
        void clear() {
            array_.clear();
        }

        template <typename SubstreamAdapter>
        void append(const io::IOSubstream& substream, int32_t column, size_t start, size_t end)
        {
            SubstreamAdapter::read_to(substream, column, start, end, array_);
        }
    };


    template <typename DataType, typename AtomType>
    class MMapValuesBufferAdapter<DataType, AtomType, false> {

        using ViewType = typename DataTypeTraits<DataType>::ViewType;

        DataTypeBuffer<DataType> buffer_;

    public:

        Span<const ViewType> span() const {return buffer_.span();}

        void clear() {
            buffer_.clear();
        }

        template <typename SubstreamAdapter>
        void append(const io::IOSubstream& substream, int32_t column, size_t start, size_t end)
        {
            SubstreamAdapter::read_to(substream, column, start, end, buffer_);
        }
    };
}

}}
