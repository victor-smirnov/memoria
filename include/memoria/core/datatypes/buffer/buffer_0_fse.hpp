
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


#include <memoria/core/datatypes/buffer/buffer_common.hpp>


namespace memoria {

template <typename DataTypeT, typename ValueType>
class DataTypeBuffer<
        DataTypeT,
        SparseObjectAdapterDescriptor<
            true,
            TL<>,
            TL<const ValueType*>
        >
>: public io::IOSubstream
{
    using TypeDimensionsTuple = std::tuple<>;
    using DataDimensionsTuple = AsTuple<TL<const ValueType*>>;
    using Builder = FixedSizeSparseObjectBuilder<DataTypeT, DataTypeBuffer>;

    ArenaBuffer<ValueType> arena_;

    Builder builder_;

public:
    using DataType = DataTypeT;

    DataTypeBuffer(DataType data_type = DataType()): builder_(this) {}
    DataTypeBuffer(const TypeDimensionsTuple& type_data): builder_(this) {}

    virtual void reindex() {}

    virtual U8String describe() const {
        return U8String("DataTypeBuffer<") + TypeNameFactory<DataType>::name().to_u8() + ">";
    }

    virtual const std::type_info& substream_type() const {
        return typeid(DataTypeBuffer);
    }

    Builder& builder() {return builder_;}
    const Builder& builder() const {return builder_;}

    void clear() {
        arena_.clear();
    }

    void reset() {
        arena_.reset();
    }

    size_t size() const {
        return arena_.size();
    }

    bool emplace_back(const ValueType& value) {
        return arena_.emplace_back(value);
    }

    bool append(const ValueType& value) {
        return arena_.emplace_back(value);
    }

    bool append(Span<const ValueType> value) {
        return arena_.append_values(value);
    }

    const ValueType& operator[](size_t idx) const {
        return arena_[idx];
    }

    Span<const ValueType> span() const {
        return arena_.span();
    }

    Span<const ValueType> span(size_t from) const
    {
        return arena_.span(from);
    }

    Span<const ValueType> span(size_t from, size_t length) const
    {
        return arena_.span(from, length);
    }

    template <int32_t Dimension>
    size_t data_length(size_t start, size_t size) const {
        return size * sizeof(ValueType);
    }

    template <int32_t Dimension>
    const ValueType* data(size_t start) const {
        return arena_.data() + start;
    }

    std::tuple<size_t> data_lengths(size_t start, psize_t size) const
    {
        return std::tuple<size_t>(size);
    }
};

}
