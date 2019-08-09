
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

#include <memoria/v1/core/types.hpp>
#include <memoria/v1/core/exceptions/exceptions.hpp>
#include <memoria/v1/core/strings/format.hpp>
#include <boost/any.hpp>

namespace memoria {
namespace v1 {

struct TinyInt   {};
struct UTinyInt  {};

struct SmallInt  {};
struct USmallInt {};

struct Integer   {};
struct UInteger  {};

struct BigInt    {};
struct UBigInt   {};

struct Varchar   {};
struct Varbinary {};
struct Real      {};
struct Double    {};
struct Timestamp {};
struct TSWithTimeZone {};
struct Date      {};
struct Time      {};
struct TimeWithTimeZone {};


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


template <typename T>
struct Dynamic {};

template <typename Key, typename Value>
class Multimap1 {
    Key key_;
    Value value_;

public:
    Multimap1(Key key, Value value): key_(key), value_(value) {}

    const Key& key() const {return key_;}
    const Value& value() const {return value_;}
};

template <>
struct TypeHash<Varchar>: UInt64Value<9748271> {};

template <>
struct TypeHash<Varbinary>: UInt64Value<83248912347> {};

template <>
struct TypeHash<BigInt>: UInt64Value<8> {};


}}
