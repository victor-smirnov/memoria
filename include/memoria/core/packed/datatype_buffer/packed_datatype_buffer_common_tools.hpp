
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

template <
        typename T,
        typename PkdStruct,
        size_t Dimension,
        size_t BlockStart,
        size_t TotalWidth,
        size_t DimensionsStart
>
class PDTDimension;

template <typename List, typename PkdStruct, size_t TotalWidth, size_t DimensionsStart, size_t StartBlkIdx = 0, size_t StartDimIdx = 0>
struct DimensionsListBuilder;

template <
        typename T,
        typename PkdStruct,
        typename... Types,

        size_t TotalWidth,
        size_t DimensionsStart,
        size_t BlkIdx,
        size_t DimIdx
>
struct DimensionsListBuilder<TL<T, Types...>, PkdStruct, TotalWidth, DimensionsStart, BlkIdx, DimIdx> {
    using Dimension = PDTDimension<
        T,
        PkdStruct,
        DimIdx,
        BlkIdx,
        TotalWidth,
        DimensionsStart
    >;

    using Type = MergeLists<
        Dimension,
        typename DimensionsListBuilder<
            TL<Types...>,
            PkdStruct,

            TotalWidth,
            DimensionsStart,
            Dimension::Width + BlkIdx,
            DimIdx + 1
        >::Type
    >;
};

template <typename PkdStruct, size_t Dimension, size_t TotalWidth, size_t DimensionsStart, size_t Idx>
struct DimensionsListBuilder<TL<>, PkdStruct, Dimension, TotalWidth, DimensionsStart, Idx> {
    using Type = MergeLists<>;
};


template <typename List, typename PkdStruct, size_t StartIdx = 0, size_t Blocks = ListSize<List>>
struct DimensionsListWidthBuilder;

template <typename T, typename PkdStruct, typename... Types, size_t Idx, size_t Blocks>
struct DimensionsListWidthBuilder<TL<T, Types...>, PkdStruct, Idx, Blocks> {
    using Dimension = PDTDimension<T, PkdStruct, Idx, Idx, Blocks, Blocks>;

    static constexpr size_t Value =
            Dimension::Width
            + DimensionsListWidthBuilder<TL<Types...>, PkdStruct, Dimension::Width + Idx, Blocks>::Value;
};

template <typename PkdStruct, size_t Idx, size_t Blocks>
struct DimensionsListWidthBuilder<TL<>, PkdStruct, Idx, Blocks> {
    static constexpr size_t Value = 0;
};



template <typename PkdStructSO>
class PkdBufViewAccessor {
    PkdStructSO buf_;
    size_t column_;
public:
    using ViewType = typename PkdStructSO::ViewType;

    PkdBufViewAccessor(PkdStructSO buf, size_t column):
        buf_(buf),
        column_(column)
    {}

    ViewType get(psize_t idx) const {
        return buf_.access(column_, idx);
    }

    bool operator==(const PkdBufViewAccessor& other) const {
        return buf_.data() == other.buf_.data();
    }
};


template <typename DataType, typename ArraySO, bool Is1DFixedSize = DTTIsNDFixedSize<DataType>>
class PackedDataType1DBufferIO;

template <typename DataType, typename ArraySO>
class PackedDataType1DBufferIO<
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
class PackedDataType1DBufferIO<
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
            array_.data()->template dimension<0>(0).data() + row, size
        );
    }

    ViewType get(size_t row) const {
        return array_.access(row);
    }

    U8String describe() const {
        return TypeNameFactory<Base>::name().to_u8();
    }
};





template <typename DataType, typename ArraySO, bool Is1DFixedSize = DTTIsNDFixedSize<DataType>>
class PackedDataTypeNDBufferIO;

template <typename DataType, typename ArraySO>
class PackedDataTypeNDBufferIO<
        DataType,
        ArraySO,
        false
>: public io::IONDArraySubstreamView<DataType> {

    using Base = io::IONDArraySubstreamView<DataType>;

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

    size_t columns() const {
        return array_.columns();
    }

    void read_to(size_t column, size_t row, size_t size, ArenaBuffer<ViewType>& buffer) const
    {
        auto ii = array_.begin(column);
        ii += row;

        for (size_t c = 0; c < size; c++, ++ii) {
            buffer.append_value(*ii);
        }
    }

    void read_to(size_t column, size_t row, size_t size, DataTypeBuffer<DataType>& buffer) const
    {
        auto ii = array_.begin(column);
        ii += row;

        for (size_t c = 0; c < size; c++, ++ii) {
            buffer.append(*ii);
        }
    }

    void read_to(size_t column, size_t row, size_t size, Span<ViewType> buffer) const
    {
        auto ii = array_.begin(column);
        ii += row;

        for (size_t c = 0; c < size; c++, ++ii) {
            buffer[c] = (*ii);
        }
    }

    virtual Span<const ViewType> span(size_t column, size_t row, size_t size) const {
        MMA_THROW(UnsupportedOperationException());
    }

    ViewType get(size_t column, size_t row) const {
        return array_.access(column, row);
    }

    U8String describe() const {
        return TypeNameFactory<Base>::name().to_u8();
    }
};




template <typename DataType, typename ArraySO>
class PackedDataTypeNDBufferIO<
        DataType,
        ArraySO,
        true
>: public io::IONDArraySubstreamView<DataType> {

    using Base = io::IONDArraySubstreamView<DataType>;

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

    size_t columns() const {
        return array_.columns();
    }

    void read_to(size_t column, size_t row, size_t size, ArenaBuffer<ViewType>& buffer) const
    {
        auto ii = array_.begin(column);
        ii += row;

        for (size_t c = 0; c < size; c++, ++ii) {
            buffer.append_value(*ii);
        }
    }

    void read_to(size_t column, size_t row, size_t size, DataTypeBuffer<DataType>& buffer) const
    {
        auto ii = array_.begin(column);
        ii += row;

        for (size_t c = 0; c < size; c++, ++ii) {
            buffer.append(*ii);
        }
    }

    void read_to(size_t column, size_t row, size_t size, Span<ViewType> buffer) const
    {
        auto ii = array_.begin(column);
        ii += row;

        for (size_t c = 0; c < size; c++, ++ii) {
            buffer[c] = (*ii);
        }
    }

    virtual Span<const ViewType> span(size_t column, size_t row, size_t size) const {
        return Span<const ViewType>(
            array_.data()->template dimension<0>(column).data() + row, size
        );
    }

    ViewType get(size_t column, size_t row) const {
        return array_.access(column, row);
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


template <typename DataType, typename BlockValueProviderFactory, size_t Columns>
struct DTBufferPrintHelper {

    template <typename PkdStructSo, typename Handler>
    static void handle(const PkdStructSo* buffer, Handler* handler) {
        const auto& meta = buffer->data()->metadata();
        for (size_t c = 0; c < meta.size(); c++)
        {
            handler->value("VALUES", BlockValueProviderFactory::provider(Columns, [&](size_t column) {
                return buffer->access(column, c);
            }));
        }
    }
};

template <typename T>
struct DTBufferPrintHelper<UTinyInt, T, 1> {

    template <typename PkdStructSo, typename Handler>
    static void handle(const PkdStructSo* buffer, Handler* handler) {
        const auto& meta = buffer->data()->metadata();

        handler->as_uint8_array("VALUES", meta.size(), [&](size_t idx) -> uint8_t {
            return buffer->access(idx);
        });
    }
};


}}
