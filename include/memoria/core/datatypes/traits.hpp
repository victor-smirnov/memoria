
// Copyright 2019-2022 Victor Smirnov
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

#include <memoria/core/datatypes/core.hpp>
#include <memoria/core/datatypes/fxd_size_view.hpp>
#include <memoria/core/datatypes/dt_span.hpp>


#include <memoria/core/datatypes/type_signature.hpp>
#include <memoria/core/strings/string_buffer.hpp>
#include <memoria/core/tools/type_name.hpp>
#include <memoria/core/integer/accumulator_common.hpp>
#include <memoria/core/numbers/number_view.hpp>

#include <memoria/core/memory/shared_ptr.hpp>

namespace memoria {


template <typename FromType, typename ToType>
struct DatatypeConverter;

template <typename Type, typename Selector = EmptyType>
struct DatatypeComparator;

template <typename Type, typename Selector = EmptyType>
struct DatatypeEqualityComparator;

template <typename LeftType, typename RightType, typename Selector = EmptyType>
struct CrossDatatypeComparator;

template <typename LeftType, typename RightType, typename Selector = EmptyType>
struct CrossDatatypeEqualityComparator;

template <typename Type>
struct FromPlainStringConverter;

template <typename Type>
struct ToPlainStringConverter;


struct NumericDatatype;


template <typename T> struct PrimitiveDataTypeName;

#define MMA_DECLARE_PRIMITIVE_DATATYPE_NAME(TypeName, TypeStr)  \
template <>                                             \
struct PrimitiveDataTypeName<TypeName> {                \
    static void create_signature(SBuf& buf, TypeName) { \
        buf << MMA_TOSTRING(TypeStr);                   \
    }                                                   \
}

MMA_DECLARE_PRIMITIVE_DATATYPE_NAME(TinyInt, TinyInt);
MMA_DECLARE_PRIMITIVE_DATATYPE_NAME(UTinyInt, UTinyInt);
MMA_DECLARE_PRIMITIVE_DATATYPE_NAME(SmallInt, SmallInt);
MMA_DECLARE_PRIMITIVE_DATATYPE_NAME(USmallInt, USmallInt);
MMA_DECLARE_PRIMITIVE_DATATYPE_NAME(Integer, Integer);
MMA_DECLARE_PRIMITIVE_DATATYPE_NAME(UInteger, UInteger);
MMA_DECLARE_PRIMITIVE_DATATYPE_NAME(BigInt, BigInt);
MMA_DECLARE_PRIMITIVE_DATATYPE_NAME(UBigInt, UBigInt);
MMA_DECLARE_PRIMITIVE_DATATYPE_NAME(Real, Real);
MMA_DECLARE_PRIMITIVE_DATATYPE_NAME(Double, Double);
MMA_DECLARE_PRIMITIVE_DATATYPE_NAME(Timestamp, Timestamp);
MMA_DECLARE_PRIMITIVE_DATATYPE_NAME(TimestampWithTZ, TimestampWithTZ);
MMA_DECLARE_PRIMITIVE_DATATYPE_NAME(Time, Time);
MMA_DECLARE_PRIMITIVE_DATATYPE_NAME(TimeWithTZ, TimeWithTZ);
MMA_DECLARE_PRIMITIVE_DATATYPE_NAME(Date, Date);
MMA_DECLARE_PRIMITIVE_DATATYPE_NAME(uint8_t, UByte);
MMA_DECLARE_PRIMITIVE_DATATYPE_NAME(int64_t, Int64);
MMA_DECLARE_PRIMITIVE_DATATYPE_NAME(uint64_t, UInt64);
MMA_DECLARE_PRIMITIVE_DATATYPE_NAME(Boolean, Boolean);
MMA_DECLARE_PRIMITIVE_DATATYPE_NAME(bool, Boolean);


template <typename T> struct DataTypeTraits {
    static constexpr bool isDataType = false;
    static constexpr bool isSdnDeserializable = false;
};


template <typename T>
using DTTViewType = typename DataTypeTraits<T>::ViewType;

template <typename T>
using DTViewArg = typename DataTypeTraits<T>::ViewType;

template <typename T>
using DTView = typename DataTypeTraits<T>::View2Type;

template <typename T>
using DTTAtomType = typename DataTypeTraits<T>::AtomType;

template <typename T>
using DTTLDViewType = typename DataTypeTraits<T>::LDViewType;

template <typename T>
using DTTTypeDimensionsTuple = typename DataTypeTraits<T>::TypeDimensionsTuple;

template <typename T>
using DTTDataDimensionsTuple = typename DataTypeTraits<T>::DataDimensionsTuple;

template <typename T>
using DTTLDStorageType = typename DataTypeTraits<T>::LDStorageType;




template <typename T>
using DTSpan = typename DataTypeTraits<T>::OSpanT;

template <typename T>
using DTSpanStorage = typename DataTypeTraits<T>::OSpanStorageT;



template <typename ViewT> class DTConstSharedPtr;

template <typename ViewT> class DTFxdValueWrapper;

template <typename ViewT, typename PtrT> class DTViewSpan;
template <typename ViewT, typename PtrT> class DTConstViewSpan;


template <typename T>
constexpr bool DTTisDataType = DataTypeTraits<T>::isDataType;

namespace dtt_ {
    template <typename T, typename = void>
    struct DTTHasLDStorageTypeH: std::false_type {};

    template <typename T>
    struct DTTHasLDStorageTypeH<T, VoidT<typename DataTypeTraits<T>::LDStorageType>>: std::true_type {};
}

template <typename T>
constexpr bool DTTisLinkedDataType = dtt_::DTTHasLDStorageTypeH<T>::value;


namespace dtt_ {
    template <typename T, typename = void>
    struct DTTHasParametersH1: std::false_type {};

    template <typename T>
    struct DTTHasParametersH1<T, VoidT<typename DataTypeTraits<T>::Parameters>>: std::true_type {};

    template <typename T, bool HasParamsTypedecl = DTTHasParametersH1<T>::value>
    struct DTTHasParametersH2: std::integral_constant<bool, ListSize<typename DataTypeTraits<T>::Parameters>> {};

    template <typename T>
    struct DTTHasParametersH2<T, false>: std::false_type {};

    template <typename T, bool HasParamsTypedecl = DTTHasParametersH1<T>::value>
    struct DTTParametersH: HasType<typename DataTypeTraits<T>::Parameters>{};

    template <typename T>
    struct DTTParametersH<T, false>: HasType<TL<>> {};
}

struct LDFxSizeValueViewSelector {};

template <typename T>
constexpr bool DTTisParametrized = dtt_::DTTHasParametersH2<T>::value;

template <typename T>
using DTTParameters = typename dtt_::DTTParametersH<T>::Type;



namespace dtt_ {
    template <typename TypedimensionsList, typename DataDimenstionsList>
    struct DTTIsNDFixedSize: HasValue<bool, false> {};

    template <typename T>
    struct DTTIsNDFixedSize<std::tuple<>, std::tuple<const T*>>: HasValue<bool, true> {};
}

template <typename DataType>
constexpr bool DTTIsNDFixedSize = dtt_::DTTIsNDFixedSize<
    DTTTypeDimensionsTuple<DataType>,
    DTTDataDimensionsTuple<DataType>
>::Value;


template <typename T> struct DataTypeTraitsBase {
    static constexpr bool isDataType    = true;
    static constexpr bool isSdnDeserializable = false;
    static constexpr bool isArithmetic  = false;
    static constexpr bool isFixedSize   = false;

    using DatumSelector         = EmptyType;
    using DatumStorageSelector  = EmptyType;
};



struct FixedSizeDataTypeTag {};


template <typename T, typename DataType>
struct MinimalDataTypeTraits: DataTypeTraitsBase<DataType>
{
    using ViewType = T;

    static constexpr bool HasTypeConstructors = false;

    using ExtData = EmptyType;

    using TypeDimensionsList  = TL<>;
    using TypeDimensionsTuple = AsTuple<TypeDimensionsList>;

    using DataDimensionsList  = TL<const T*>;
    using DataDimensionsTuple = AsTuple<DataDimensionsList>;

    static void create_signature(SBuf& buf, DataType obj) {
        PrimitiveDataTypeName<DataType>::create_signature(buf, obj);
    }

    static void create_signature(SBuf& buf) {
        PrimitiveDataTypeName<DataType>::create_signature(buf, DataType());
    }
};

template <typename T, typename DataType>
struct ArithmeticMinimalDataTypeTraits: MinimalDataTypeTraits<T, DataType> {
    static constexpr bool isArithmetic = true;
};


template <typename T, typename DataType, typename LDST = T>
struct FixedSizeDataTypeTraits: DataTypeTraitsBase<DataType>
{
    using ViewType      = T;
    using LDViewType    = T;
    using LDStorageType = LDST;

    using View2Type = Own<FxdSizeView<T, ViewKind::BY_VALUE>, OwningKind::WRAPPING>;

    static constexpr bool isFixedSize = true;

    static constexpr bool HasTypeConstructors = false;

    static constexpr bool isSdnDeserializable = std::is_arithmetic<T>::value;

    using ExtData = EmptyType;

    using TypeDimensionsList  = TL<>;
    using TypeDimensionsTuple = AsTuple<TypeDimensionsList>;

    using DataDimensionsList  = TL<const T*>;
    using DataDimensionsTuple = AsTuple<DataDimensionsList>;

    using DatumSelector = FixedSizeDataTypeTag;

    using MakeLDViewSelector = LDFxSizeValueViewSelector;

    // FIXME Muse use true storage type here
    using SpanStorageT = ViewType;
    using OSpanT = OSpan<View2Type, SpanStorageT>;

    static TypeDimensionsTuple describe_type(const DataType& data_type) {
        return TypeDimensionsTuple{};
    }

    static DataDimensionsTuple describe_data(const ViewType* view) {
        return DataDimensionsTuple{view};
    }

    static TypeDimensionsTuple describe_type(ViewType view) {
        return TypeDimensionsTuple{};
    }


    static ViewType make_view(const DataDimensionsTuple& data)
    {
        return *std::get<0>(data);
    }

    static ViewType make_view(const TypeDimensionsTuple& type, const DataDimensionsTuple& data)
    {
        return *std::get<0>(data);
    }

    static void create_signature(SBuf& buf, DataType obj) {
        PrimitiveDataTypeName<DataType>::create_signature(buf, obj);
    }

    static void create_signature(SBuf& buf) {
        PrimitiveDataTypeName<DataType>::create_signature(buf, DataType());
    }
};

template <typename T, typename DataType, typename LDST = T>
struct ArithmeticFixedSizeDataTypeTraits: FixedSizeDataTypeTraits<T, DataType, LDST> {
    static constexpr bool isArithmetic = true;

    using View2Type = Own<NumberView<T, ViewKind::BY_VALUE>, OwningKind::WRAPPING>;
};



template <typename T, typename DataType>
struct NonSdnFixedSizeDataTypeTraits: FixedSizeDataTypeTraits<T, DataType>
{
    static constexpr bool isSdnDeserializable = false;
};

template <typename T, typename DataType>
struct SdnFixedSizeDataTypeTraits: FixedSizeDataTypeTraits<T, DataType>
{
    static constexpr bool isSdnDeserializable = true;
};


template <typename DataType, typename Buffer>
class FixedSizeSparseObjectBuilder {
    Buffer* buffer_;

    using ViewType = DTTViewType<DataType>;

    ViewType value_;

public:
    FixedSizeSparseObjectBuilder(Buffer* buffer):
        buffer_(buffer),
        value_()
    {}

    FixedSizeSparseObjectBuilder(FixedSizeSparseObjectBuilder&&) = delete;
    FixedSizeSparseObjectBuilder(const FixedSizeSparseObjectBuilder&) = delete;

    const ViewType& view() const {
        return value_;
    }

    const ViewType& value() const {
        return value_;
    }

    ViewType& value() {
        return value_;
    }


    void build()
    {
        buffer_->append(value_);
        value_ = ViewType{};
    }

    bool is_empty() const {
        return true;
    }
};


template <>
struct DataTypeTraits<TinyInt>: ArithmeticFixedSizeDataTypeTraits<int8_t, TinyInt> {};

template <>
struct DataTypeTraits<UTinyInt>: ArithmeticFixedSizeDataTypeTraits<uint8_t, UTinyInt> {};


template <>
struct DataTypeTraits<SmallInt>: ArithmeticFixedSizeDataTypeTraits<int16_t, SmallInt> {};

template <>
struct DataTypeTraits<USmallInt>: ArithmeticFixedSizeDataTypeTraits<uint16_t, USmallInt> {};


template <>
struct DataTypeTraits<Integer>: ArithmeticFixedSizeDataTypeTraits<int32_t, Integer> {};

template <>
struct DataTypeTraits<UInteger>: ArithmeticFixedSizeDataTypeTraits<uint32_t, UInteger> {};

template <>
struct DataTypeTraits<BigInt>: ArithmeticFixedSizeDataTypeTraits<int64_t, BigInt> {};

template <>
struct DataTypeTraits<UBigInt>: ArithmeticFixedSizeDataTypeTraits<uint64_t, UBigInt> {};

template <>
struct DataTypeTraits<Real>: ArithmeticFixedSizeDataTypeTraits<float, Real> {};

template <>
struct DataTypeTraits<Double>: ArithmeticFixedSizeDataTypeTraits<double, Double> {};

template <>
struct DataTypeTraits<Timestamp>: NonSdnFixedSizeDataTypeTraits<int64_t, Timestamp> {};

template <>
struct DataTypeTraits<TimestampWithTZ>: NonSdnFixedSizeDataTypeTraits<int64_t, TimestampWithTZ> {};

template <>
struct DataTypeTraits<Time>: NonSdnFixedSizeDataTypeTraits<int32_t, Time> {};

template <>
struct DataTypeTraits<TimeWithTZ>: NonSdnFixedSizeDataTypeTraits<int64_t, TimeWithTZ> {};

template <>
struct DataTypeTraits<Date>: NonSdnFixedSizeDataTypeTraits<int64_t, Date> {};

template <>
struct DataTypeTraits<Boolean>: FixedSizeDataTypeTraits<bool, Boolean, uint8_t> {};


template <>
struct DataTypeTraits<int32_t>: ArithmeticFixedSizeDataTypeTraits<int32_t, Integer> {};

template <>
struct DataTypeTraits<int64_t>: ArithmeticFixedSizeDataTypeTraits<int64_t, int64_t> {};

template <>
struct DataTypeTraits<uint64_t>: ArithmeticFixedSizeDataTypeTraits<uint64_t, uint64_t> {};


#ifdef MMA_HAS_INT128
template <>
struct DataTypeTraits<UInt128T>: ArithmeticFixedSizeDataTypeTraits<UInt128T, UInt128T> {};

template <>
struct DataTypeTraits<Int128T>: ArithmeticFixedSizeDataTypeTraits<Int128T, Int128T> {};
#endif


template <>
struct DataTypeTraits<UAcc64T>: ArithmeticFixedSizeDataTypeTraits<UAcc64T, UAcc64T> {};

template <>
struct DataTypeTraits<UAcc128T>: ArithmeticFixedSizeDataTypeTraits<UAcc128T, UAcc128T> {};

template <>
struct DataTypeTraits<UAcc192T>: ArithmeticFixedSizeDataTypeTraits<UAcc192T, UAcc192T> {};

template <>
struct DataTypeTraits<UAcc256T>: ArithmeticFixedSizeDataTypeTraits<UAcc256T, UAcc256T> {};

template <>
struct DataTypeTraits<Decimal>: DataTypeTraitsBase<Decimal>
{
    using Parameters = TL<>;

    static constexpr bool HasTypeConstructors = true;
    static constexpr bool Arithmetic = true;


    using ViewType = EmptyType; // Probably, hasn't been defined yet

    static void create_signature(SBuf& buf, const Decimal& obj)
    {
        buf << "Decimal";

        if (obj.is_default()) {
            buf << "()";
        }
        else {
            buf << "(" << obj.precision() << ", " << obj.scale() << ")";
        }
    }

    static void create_signature(SBuf& buf)
    {
        buf << "Decimal";
    }
};


template <>
struct DataTypeTraits<BigDecimal>: DataTypeTraitsBase<BigDecimal>
{
    using Parameters = TL<>;

    static constexpr bool isDataType          = true;
    static constexpr bool HasTypeConstructors = true;
    static constexpr bool Arithmetic = true;


    using ViewType = EmptyType; // Probably, hasn't been defined yet


    static void create_signature(SBuf& buf, const Decimal& obj)
    {
        buf << "BigDecimal";

        if (obj.is_default())
        {
            buf << "()";
        }
        else {
            buf << "(" << obj.precision() << ", " << obj.scale() << ")";
        }
    }

    static void create_signature(SBuf& buf) {
        buf << "BigDecimal";
    }
};



template <>
struct DataTypeTraits<CoreApiProfileDT>: DataTypeTraitsBase<CoreApiProfileDT>
{
    using Parameters = TL<>;

    static constexpr bool HasTypeConstructors = false;

    using ViewType = CoreApiProfile; // Probably, hasn't been defined yet

    static void create_signature(SBuf& buf, const CoreApiProfileDT& obj) {
        create_signature(buf);
    }

    static void create_signature(SBuf& buf) {
        buf << "CoreApiProfileDT";
    }
};



namespace arena {

template <typename DT, typename SelectorTag = typename DataTypeTraits<DT>::DatumSelector>
class ArenaDataTypeContainer;

}


template<typename T>
TypeSignature make_datatype_signature(T obj)
{
    SBuf buf;
    DataTypeTraits<T>::create_signature(buf, obj);
    return TypeSignature(buf.str());
}

template<typename T>
TypeSignature make_datatype_signature()
{
    SBuf buf;
    DataTypeTraits<T>::create_signature(buf);
    return TypeSignature(buf.str());
}

template<typename T>
U8String make_datatype_signature_string()
{
    SBuf buf;
    DataTypeTraits<T>::create_signature(buf);
    return buf.str();
}


template <typename Type>
struct DatatypeComparator<Type, NumericDatatype> {
    static int32_t compare(const DTTViewType<Type>& left, const DTTViewType<Type>& right) noexcept {
        return left - right;
    }
};

template <typename Type>
struct DatatypeEqualityComparator<Type, NumericDatatype> {
    static bool equals(const DTTViewType<Type>& left, const DTTViewType<Type>& right) noexcept {
        return right == left;
    }
};

template <typename Selector>
struct DatatypeEqualityComparator<Boolean, Selector> {
    static bool equals(const DTTViewType<Boolean>& left, const DTTViewType<Boolean>& right) noexcept {
        return right == left;
    }
};


template <typename LeftType, typename RightType>
struct CrossDatatypeComparator<LeftType, RightType, NumericDatatype> {
    static int32_t compare(const DTTViewType<LeftType>& left, const DTTViewType<RightType>& right) noexcept {
        return left - right;
    }
};

template <typename LeftType, typename RightType>
struct CrossDatatypeEqualityComparator<LeftType, RightType, NumericDatatype> {
    static bool equals(const DTTViewType<LeftType>& left, const DTTViewType<RightType>& right) noexcept {
        return right == left;
    }
};



}
