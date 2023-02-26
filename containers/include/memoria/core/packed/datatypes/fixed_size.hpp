
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

#include <memoria/core/packed/datatypes/accessors_common.hpp>
#include <memoria/core/strings/u8_string.hpp>
#include <memoria/core/tools/span.hpp>

#include <memoria/core/tools/bitmap.hpp>

namespace memoria {

template <typename DataType, typename PkdStructTag, typename ArraySO>
struct PkdDataTypeAccessor<DataType, PkdStructTag, ArraySO, FixedSizeDataTypeTag> {

    using ViewType  = typename DataTypeTraits<DataType>::ViewType;

    static constexpr size_t Columns = ArraySO::Columns;

private:
    const ViewType* data_;

public:
    PkdDataTypeAccessor(const ArraySO& array, size_t column):
        data_(array.values(column).data())
    {}

    bool operator==(const PkdDataTypeAccessor& other) const {
        return data_ == other.data_;
    }

    const ViewType& get(size_t row) const
    {
        return *(data_ + row);
    }

    static const ViewType& get(const ArraySO& array, psize_t column, size_t row)
    {
        const ViewType* data = array.values(column).data();
        return *(data + row);
    }
};


}
