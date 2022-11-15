
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
#include <memoria/core/exceptions/exceptions.hpp>
#include <memoria/core/strings/format.hpp>
#include <memoria/core/reflection/typehash.hpp>

#include <boost/any.hpp>

namespace memoria {

struct TinyInt   {};
struct UTinyInt  {};

struct SmallInt  {};
struct USmallInt {};

struct Integer   {};
struct UInteger  {};

struct BigInt    {};
struct UBigInt   {};

struct Real      {};
struct Double    {};

struct Varchar   {};
struct Varbinary {};
struct Timestamp {};
struct TimestampWithTZ {};
struct Date      {};
struct Time      {};
struct TimeWithTZ {};
struct Boolean   {};

// TODO: May be Hermes Datatypes, but
// should not have unprefixed aliases
struct UID64CowBlockID {};
struct UID256CowBlockID {};

template <typename ProfileDT> struct ProfileFromDataTypeTF;
template <typename Profile> struct DataTypeFromProfileTF;

struct CoreApiProfileDT {};

template <>
struct ProfileFromDataTypeTF<CoreApiProfileDT>: HasType<CoreApiProfile> {};

template <>
struct DataTypeFromProfileTF<CoreApiProfile>: HasType<CoreApiProfileDT> {};

template <typename DT>
using ProfileFromDataType = typename ProfileFromDataTypeTF<DT>::Type;

template <typename DT>
using DataTypeFromProfile = typename DataTypeFromProfileTF<DT>::Type;


class Decimal {
    bool default_;
    short precision_;
    short scale_;
public:
    constexpr Decimal(short precision, short scale):
        default_(false), precision_(precision), scale_(scale)
    {}

    constexpr Decimal():
        default_(true), precision_(38), scale_(0)
    {}

    constexpr Decimal(short precision):
        default_(false), precision_(precision), scale_(0)
    {}

    short precision() const {return precision_;}
    short scale() const {return scale_;}

    bool is_default() const {return default_;}
};

class BigDecimal {
    bool default_;
    int32_t precision_;
    int32_t scale_;

public:
    constexpr BigDecimal(int32_t precision, int32_t scale):
        default_(false), precision_(precision), scale_(scale)
    {}

    constexpr BigDecimal():
        default_(true), precision_(38), scale_(0)
    {}

    constexpr BigDecimal(int32_t precision):
        default_(false), precision_(precision), scale_(0)
    {}

    int32_t precision() const {return precision_;}
    int32_t scale() const {return scale_;}
    bool is_default() const {return default_;}
};

enum class DTOrdering {
    SUM, MAX, UNORDERED
};

template <typename DataType, typename Buffer> class SparseObjectBuilder;


template <>
struct TypeHash<TinyInt>:  UInt64Value<20>  {};

template <>
struct TypeHash<UTinyInt>: UInt64Value<21>  {};

template <>
struct TypeHash<SmallInt>: UInt64Value<22>  {};

template <>
struct TypeHash<USmallInt>: UInt64Value<24> {};

template <>
struct TypeHash<Integer>:  UInt64Value<23>  {};

template <>
struct TypeHash<UInteger>: UInt64Value<25>  {};

template <>
struct TypeHash<BigInt>:   UInt64Value<26>  {};

template <>
struct TypeHash<UBigInt>:  UInt64Value<27>  {};

template <>
struct TypeHash<Varchar>:  UInt64Value<28>  {};

template <>
struct TypeHash<Varbinary>: UInt64Value<29> {};

template <>
struct TypeHash<Real>:     UInt64Value<30>  {};

template <>
struct TypeHash<Double>: UInt64Value<31>    {};

template <>
struct TypeHash<Timestamp>: UInt64Value<32> {};

template <>
struct TypeHash<TimestampWithTZ>: UInt64Value<33> {};

template <>
struct TypeHash<Date>: UInt64Value<34>      {};

template <>
struct TypeHash<Time>: UInt64Value<35>      {};

template <>
struct TypeHash<TimeWithTZ>: UInt64Value<36> {};

template <>
struct TypeHash<Boolean>: UInt64Value<37>   {};

template <>
struct TypeHash<BigDecimal>: UInt64Value<38> {};

template <>
struct TypeHash<Decimal>: UInt64Value<39> {};



template <>
struct TypeHash<CoreApiProfileDT>: UInt64Value<75796829474235345ull> {};

using AllHermesDatatypes = TL<
    Varchar, BigInt, UBigInt, Boolean, Double, Real, TinyInt, UTinyInt, SmallInt, USmallInt, Integer, UInteger
>;

using IntegerNumericDatatypes = TL<
    TinyInt, SmallInt, Integer, BigInt,
    UTinyInt, USmallInt, UInteger, UBigInt
>;

using RealNumericDatatypes = TL<
    Real, Double
>;

using AllNumericDatatypes = MergeLists<
    IntegerNumericDatatypes, RealNumericDatatypes
>;



}
