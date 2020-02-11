
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

#include <memoria/core/types.hpp>
#include <memoria/core/types/list/append.hpp>
#include <memoria/core/tools/result.hpp>

#include <memoria/core/iovector/io_substream_base.hpp>

#include <type_traits>

namespace memoria {
namespace pdtbuf_ {

template <typename T, typename PkdStruct, psize_t Block>
class PDTDimension;

template <typename List, typename PkdStruct, psize_t StartIdx>
struct DimensionsListBuilder;

template <typename T, typename PkdStruct, typename... Types, psize_t Idx>
struct DimensionsListBuilder<TL<T, Types...>, PkdStruct, Idx> {
    using Dimension = PDTDimension<T, PkdStruct, Idx>;

    using Type = MergeLists<
        Dimension,
        DimensionsListBuilder<TL<Types...>, PkdStruct, Dimension::Width + Idx>
    >;
};

template <typename PkdStruct, psize_t Idx>
struct DimensionsListBuilder<TL<>, PkdStruct, Idx> {
    using Type = MergeLists<>;
};


template <typename PkdStructSO>
class PkdBufViewAccessor {
    PkdStructSO buf_;
public:
    using ViewType = typename PkdStructSO::ViewType;

    PkdBufViewAccessor(PkdStructSO buf):
        buf_(buf)
    {}

    ViewType get(psize_t idx) const {
        return buf_.access(idx);
    }

    bool operator==(const PkdBufViewAccessor& other) const {
        return buf_.data() == other.buf_.data();
    }
};


template <typename DataType, typename ArraySO, bool Is1DFixedSize = DTTIs1DFixedSize<DataType>>
class PackedDataTypeBufferIO;

template <typename DataType, typename ArraySO>
class PackedDataTypeBufferIO<
        DataType,
        ArraySO,
        false
>: public io::IO1DArraySubstreamView<DataType> {

    using Base = io::IO1DArraySubstreamView<DataType>;

    using typename Base::ViewType;

    ArraySO array_{};

public:
    void configure(ArraySO array)
    {
        array_ = array;
    }

    size_t size() const {
        return array_.size();
    }

    void read_to(size_t row, size_t size, ArenaBuffer<ViewType>& buffer) const
    {
        auto ii = array_.begin();
        ii += row;

        for (size_t c = 0; c < size; c++, ++ii) {
            buffer.append_value(*ii);
        }
    }

    void read_to(size_t row, size_t size, DataTypeBuffer<DataType>& buffer) const
    {
        auto ii = array_.begin();
        ii += row;

        for (size_t c = 0; c < size; c++, ++ii) {
            buffer.append(*ii);
        }
    }

    void read_to(size_t row, size_t size, Span<ViewType> buffer) const
    {
        auto ii = array_.begin();
        ii += row;

        for (size_t c = 0; c < size; c++, ++ii) {
            buffer[c] = (*ii);
        }
    }

    virtual Span<const ViewType> span(size_t row, size_t size) const {
        MMA_THROW(UnsupportedOperationException());
    }

    ViewType get(size_t row) const {
        return array_.access(row);
    }

    U8String describe() const {
        return TypeNameFactory<Base>::name().to_u8();
    }
};




template <typename DataType, typename ArraySO>
class PackedDataTypeBufferIO<
        DataType,
        ArraySO,
        true
>: public io::IO1DArraySubstreamView<DataType> {

    using Base = io::IO1DArraySubstreamView<DataType>;

    using typename Base::ViewType;

    ArraySO array_{};

public:
    void configure(ArraySO array)
    {
        array_ = array;
    }

    size_t size() const {
        return array_.size();
    }

    void read_to(size_t row, size_t size, ArenaBuffer<ViewType>& buffer) const
    {
        auto ii = array_.begin();
        ii += row;

        for (size_t c = 0; c < size; c++, ++ii) {
            buffer.append_value(*ii);
        }
    }

    void read_to(size_t row, size_t size, DataTypeBuffer<DataType>& buffer) const
    {
        auto ii = array_.begin();
        ii += row;

        for (size_t c = 0; c < size; c++, ++ii) {
            buffer.append(*ii);
        }
    }

    void read_to(size_t row, size_t size, Span<ViewType> buffer) const
    {
        auto ii = array_.begin();
        ii += row;

        for (size_t c = 0; c < size; c++, ++ii) {
            buffer[c] = (*ii);
        }
    }

    virtual Span<const ViewType> span(size_t row, size_t size) const {
        return Span<const ViewType>(
            array_.data()->template dimension<0>().data() + row, size
        );
    }

    ViewType get(size_t row) const {
        return array_.access(row);
    }

    U8String describe() const {
        return TypeNameFactory<Base>::name().to_u8();
    }
};



template <typename List> class BufferSizeTypeSelector;

template <typename T, typename... Tail>
class BufferSizeTypeSelector<TL<const T*, Tail...>> {
    static constexpr PackedDataTypeSize TailDataTypeSize = BufferSizeTypeSelector<TL<Tail...>>::DataTypeSize;

public:
    static constexpr PackedDataTypeSize DataTypeSize =
            TailDataTypeSize == PackedDataTypeSize::FIXED ?
                PackedDataTypeSize::FIXED :
                PackedDataTypeSize::VARIABLE;
};

template <typename T, typename... Tail>
class BufferSizeTypeSelector<TL<Span<const T>, Tail...>> {
public:
    static constexpr PackedDataTypeSize DataTypeSize = PackedDataTypeSize::VARIABLE;
};

template <>
class BufferSizeTypeSelector<TL<>> {
public:
    static constexpr PackedDataTypeSize DataTypeSize = PackedDataTypeSize::FIXED;
};

}}
