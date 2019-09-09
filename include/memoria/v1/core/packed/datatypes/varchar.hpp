
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

#include <memoria/v1/core/packed/datatypes/accessors_common.hpp>
#include <memoria/v1/core/strings/u8_string.hpp>
#include <memoria/v1/core/tools/span.hpp>

#include <memoria/v1/core/tools/bitmap.hpp>

namespace memoria {
namespace v1 {


template <typename ArraySO, typename SelectorTag>
struct PkdDataTypeAccessor<Varchar, PkdVLEArrayTag, ArraySO, SelectorTag> {

    using DataType = Varchar;
    using ViewType = DataTypeTraits<Varchar>::ViewType;
    using DataSizeType = typename ArraySO::DataSizeType;
    using DataAtomType = typename ArraySO::DataAtomType;

    static constexpr psize_t Columns = ArraySO::Columns;

private:
    const DataSizeType* offsets_;
    const DataAtomType* data_;

public:
    PkdDataTypeAccessor(const ArraySO& array, psize_t column):
        offsets_(array.data()->offsets(column)),
        data_(array.data()->data(column))
    {}

    bool operator==(const PkdDataTypeAccessor& other) const {
        return offsets_ == other.offsets_;
    }

    ViewType get(psize_t row) const
    {
        psize_t length = offsets_[row + 1] - offsets_[row];
        const DataAtomType* data = data_ + offsets_[row];

        return ViewType(data, length);
    }

    static ViewType get(const ArraySO& array, psize_t column, psize_t row)
    {
        const DataSizeType* offsets = array.data()->offsets(column);
        const DataAtomType* data = array.data()->data(column);

        psize_t length = offsets[row + 1] - offsets[row];

        return ViewType(data + offsets[row], length);
    }



    static size_t length(const ViewType& element) {
        return element.length();
    }

    static const auto* data(const ViewType& element) {
        return element.data();
    }

    static OpStatus rowwise_insert(ArraySO& array, psize_t row_at, Span<const Span<const ViewType>> elements)
    {
        psize_t size = elements.length();
        return insert(array, row_at, size, [&] (psize_t col, psize_t row) -> const ViewType& {
            return elements[row][col];
        });
    }

    static OpStatus columnwise_insert(ArraySO& array, psize_t row_at, Span<const Span<const ViewType>> elements)
    {
        psize_t size = elements[0].length();
        return insert(array, row_at, size, [&] (psize_t col, psize_t row) -> const ViewType& {
            return elements[col][row];
        });
    }

    template <typename AccessorFn>
    static OpStatus insert(ArraySO& array, psize_t row_at, psize_t size, AccessorFn&& elements)
    {
        psize_t data_lengths[Columns] = {};

        for (psize_t column = 0; column < Columns; column++)
        {
            for (size_t row = 0; row < size; row++)
            {
                data_lengths[column] += length(elements(column, row));
            }
        }

        if (isFail(array.insertSpace(row_at, size, data_lengths))) {
            return OpStatus::FAIL;
        }

        for (psize_t column = 0; column < Columns; column++)
        {
            auto array_offsets = array.data()->offsets(column);
            auto array_data    = array.data()->data(column);

            size_t pos = array_offsets[row_at];
            for (psize_t row = 0; row < size; row++)
            {
                auto element = elements(column, row);

                size_t len = DataTypeTraits<DataType>::length(element);
                const auto* dat = DataTypeTraits<DataType>::data(element);

                MemCpyBuffer(dat, array_data + pos, len);
                array_offsets[row_at + row] = pos;
                pos += len;
            }
        }

        return OpStatus::OK;
    }
};


}}
