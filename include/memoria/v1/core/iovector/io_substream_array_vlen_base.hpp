
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

#include <memoria/v1/core/iovector/io_substream_base.hpp>

#include <memoria/v1/core/tools/type_name.hpp>

#include <memoria/v1/core/tools/arena_buffer.hpp>

#include <memoria/v1/api/datatypes/traits.hpp>

#include <typeinfo>

namespace memoria {
namespace v1 {
namespace io {


template<typename Value>
struct IORowwiseVLenArraySubstream: IOSubstream {
    virtual int32_t columns() const                     = 0;
    virtual int32_t size() const                        = 0;

    virtual const uint8_t* select(int32_t row) const    = 0;
    virtual uint8_t* select(int32_t row)                = 0;

    virtual void get_lengths_to(int32_t start, int32_t size, int32_t* array) const = 0;

    virtual uint8_t* reserve(int32_t rows, const int32_t* lengths) = 0;    
    virtual uint8_t* ensure(int32_t capacity) = 0;

    virtual U8String describe() const {
        return TypeNameFactory<IORowwiseVLenArraySubstream<Value>>::name().to_u8();
    }

    virtual const std::type_info& substream_type() const {
        return typeid(IORowwiseVLenArraySubstream<Value>);
    }
};



struct VLenArrayColumnMetadata {
    uint8_t* data_buffer;
    int32_t data_size;
    int32_t data_buffer_size;
    int32_t size;
};

struct ConstVLenArrayColumnMetadata {
    const uint8_t* data_buffer;
    int32_t data_size;
    int32_t data_buffer_size;
    int32_t size;
};

template <typename Value>
struct IOColumnwiseVLenArraySubstream: IOSubstream {
    virtual ConstVLenArrayColumnMetadata describe(int32_t column) const    = 0;
    virtual VLenArrayColumnMetadata describe(int32_t column)               = 0;


    virtual int32_t columns() const                                   = 0;
    virtual const uint8_t* select(int32_t column, int32_t idx) const  = 0;
    virtual uint8_t* select(int32_t column, int32_t idx)              = 0;

    virtual void get_lengths_to(int32_t column, int32_t start, int32_t size, int32_t* array) const = 0;

    virtual VLenArrayColumnMetadata select_and_describe(int32_t column, int32_t idx)              = 0;
    virtual ConstVLenArrayColumnMetadata select_and_describe(int32_t column, int32_t idx) const   = 0;

    virtual uint8_t* reserve(int32_t column, int32_t size, const int32_t* lengths) = 0;

    virtual uint8_t* ensure(int32_t column, int32_t capacity) = 0;

    virtual U8String describe() const {
        return TypeNameFactory<IOColumnwiseVLenArraySubstream<Value>>::name().to_u8();
    }


    virtual const std::type_info& substream_type() const {
        return typeid(IOColumnwiseVLenArraySubstream<Value>);
    }

    virtual const std::type_info& content_type() const {
        return typeid(Value);
    }
};

template <typename DataType>
struct IOVLen1DArraySubstream: IOSubstream {

    using ViewType  = typename DataTypeTraits<DataType>::ViewType;
    using ValueType = typename DataTypeTraits<DataType>::ValueType;
    using AtomType  = typename DataTypeTraits<DataType>::AtomType;
    using DataSizeType = psize_t;


    using Buffer = ArenaBuffer<ViewType>;

    virtual psize_t size() const = 0;

    virtual psize_t length(psize_t row, psize_t size) const = 0;

    virtual void read_to(psize_t row, psize_t size, Buffer& buffer) const = 0;
    virtual void append_from(Span<const ValueType> buffer) = 0;
    virtual void append_from(Span<const ViewType> buffer) = 0;

    virtual DataSizeType* offsets(psize_t row) = 0;
    virtual const DataSizeType* offsets(psize_t row) const = 0;

    virtual AtomType* data(psize_t pos) = 0;
    virtual const AtomType* data(psize_t pos) const = 0;

    virtual const AtomType* data_for(psize_t row) const = 0;

    virtual ViewType get(psize_t row) const = 0;

    virtual const std::type_info& substream_type() const {
        return typeid(IOVLen1DArraySubstream<DataType>);
    }

};



template <typename DataType>
class IOVLen1DArraySubstreamImpl: public IOVLen1DArraySubstream<DataType> {
    using Base = IOVLen1DArraySubstream<DataType>;
public:
    using typename Base::ViewType;
    using typename Base::ValueType;
    using typename Base::AtomType;
    using typename Base::DataSizeType;
    using typename Base::Buffer;

private:

    using Adapter = DataTypeTraits<DataType>;

    ArenaBuffer<psize_t> offsets_;
    ArenaBuffer<AtomType> data_;

public:
    IOVLen1DArraySubstreamImpl() {
        offsets_.emplace_back(0);
    }

    virtual psize_t size() const {
        return offsets_.size() - 1;
    }

    virtual psize_t length(psize_t row, psize_t size) const
    {
        return offsets_[row + size] - offsets_[row];
    }

    virtual void read_to(psize_t row, psize_t size, Buffer& buffer) const
    {
        for (size_t c = row; c < row + size; c++)
        {
            const AtomType* data = data_.data() + offsets_[c];
            psize_t length = offsets_[c + 1] - offsets_[c];

            ViewType view = Adapter::make_view(data, length);
            buffer.append_value(view);
        }
    }

    virtual void append_from(Span<const ViewType> buffer)
    {
        for (auto& value: buffer)
        {
            psize_t length = Adapter::length(value);
            const AtomType* data = Adapter::data(value);
            psize_t top = offsets_.head();

            offsets_.append_value(top + length);
            data_.append_values(data, length);
        }
    }

    virtual void append_from(Span<const ValueType> buffer)
    {
        for (auto& value: buffer)
        {
            psize_t length = Adapter::length(value);
            const AtomType* data = Adapter::data(value);
            psize_t top = offsets_.head();

            offsets_.append_value(top + length);
            data_.append_values(data, length);
        }
    }

    virtual DataSizeType* offsets(psize_t row) {
        return offsets_.data() + row;
    }

    virtual const DataSizeType* offsets(psize_t row) const {
        return offsets_.data() + row;
    }

    virtual AtomType* data(psize_t pos) {
        return data_.data() + pos;
    }

    virtual const AtomType* data(psize_t pos) const {
        return data_.data() + pos;
    }

    virtual const AtomType* data_for(psize_t row) const {
        return data_.data() + offsets_[row];
    }

    virtual ViewType get(psize_t row) const
    {
        const AtomType* data = data_.data() + offsets_[row];
        psize_t length = offsets_[row + 1] - offsets_[row];

        return Adapter::make_view(data, length);
    }

    virtual void reset() {
        offsets_.clear();
        offsets_.append_value(0);
        data_.clear();
    }

    virtual void reindex() {}

    virtual U8String describe() const {
        return TypeNameFactory<IOVLen1DArraySubstream<DataType>>::name().to_u8();
    }
};


template <typename DataType, typename ArraySO>
class IOVLen1DArraySubstreamViewImpl: public IOVLen1DArraySubstream<DataType> {
    using Base = IOVLen1DArraySubstream<DataType>;
public:
    using typename Base::ViewType;
    using typename Base::ValueType;
    using typename Base::AtomType;
    using typename Base::DataSizeType;
    using typename Base::Buffer;

    using Accessor = typename ArraySO::Accessor;

private:

    using Adapter = DataTypeTraits<DataType>;

    ArraySO array_;

public:
    IOVLen1DArraySubstreamViewImpl() {}

    void configure(ArraySO array)
    {
        array_ = array;
    }

    virtual psize_t size() const {
        return array_.size();
    }

    virtual psize_t length(psize_t row, psize_t size) const
    {
        return array_.length(0, row, size);
    }

    virtual void read_to(psize_t row, psize_t size, Buffer& buffer) const
    {
        Accessor accessor(array_, 0);

        for (size_t c = row; c < row + size; c++)
        {
            buffer.append_value(accessor.get(c));
        }
    }

    virtual void append_from(Span<const ViewType> buffer)
    {
        MMA1_THROW(UnsupportedOperationException());
    }

    virtual void append_from(Span<const ValueType> buffer)
    {
        MMA1_THROW(UnsupportedOperationException());
    }

    virtual DataSizeType* offsets(psize_t row) {
        MMA1_THROW(UnsupportedOperationException());
    }

    virtual const DataSizeType* offsets(psize_t row) const {
        return array_.data()->offsets(0) + row;
    }

    virtual AtomType* data(psize_t pos) {
        MMA1_THROW(UnsupportedOperationException());
    }

    virtual const AtomType* data(psize_t pos) const {
        return array_.data()->data(0) + pos;
    }

    virtual const AtomType* data_for(psize_t row) const {
        return array_.data_for(0, row);
    }

    virtual ViewType get(psize_t row) const
    {
        Accessor accessor(array_, 0);
        return accessor.get(row);
    }


    virtual void reset() {}

    virtual void reindex() {}

    virtual U8String describe() const {
        return TypeNameFactory<IOVLen1DArraySubstream<DataType>>::name().to_u8();
    }
};


}}}
