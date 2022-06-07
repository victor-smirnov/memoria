
// Copyright 2011-2022 Victor Smirnov
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

#include <memoria/core/config.hpp>

#include <boost/exception/all.hpp>
#include <boost/stacktrace.hpp>

#include <string>
#include <sstream>
#include <cassert>
#include <cstdint>
#include <exception>

#include <unicode/utypes.h>

#include <type_traits>
#include <tuple>

namespace memoria {

static constexpr int DEFAULT_BLOCK_SIZE                 = 8192;
static constexpr int PackedAllocationAlignment          = 8;

using psize_t = uint32_t;
enum class PackedDataTypeSize {FIXED, VARIABLE};

// Require Gcc or Clang for now.
#ifdef MMA_HAS_INT128
using UInt128T = __uint128_t;
using Int128T  = __int128_t;
#endif

template <typename T, T V> struct ConstValue {
    static constexpr T Value = V;

    constexpr operator T() const noexcept {return Value;}
    constexpr T operator()() const noexcept {return Value;}
};

template <uint32_t Value>
using UInt32Value = ConstValue<uint32_t, Value>;

template <uint64_t Value>
using UInt64Value = ConstValue<uint64_t, Value>;

template <int32_t Value>
using IntValue = ConstValue<int32_t, Value>;


template <bool Value>
using BoolValue = ConstValue<bool, Value>;


template <typename...> using VoidT = void;


template <typename T> struct TypeHash; // must define Value constant

template <typename T>
constexpr uint64_t TypeHashV = TypeHash<T>::Value;

template <typename ... Types>
struct TypeList {
    constexpr TypeList() = default;
};

template <typename ... Types>
using TL = TypeList<Types...>;


template <typename T, T ... Values>
struct ValueList {
    constexpr ValueList() = default;
};


template <int32_t... Values>
using IntList = ValueList<int32_t, Values...>;

template <size_t... Values>
using SizeTList = ValueList<size_t, Values...>;

template <uint32_t... Values>
using UInt32List = ValueList<uint32_t, Values...>;

template <uint64_t... Values>
using UInt64List = ValueList<uint64_t, Values...>;

template <typename> struct TypeHash;


/*
 * Container type names & profiles
 */

struct BT {};

struct Composite    {};

template <typename ChildType = void>
class CoreApiProfileT {};

using CoreApiProfile = CoreApiProfileT<>;

struct WT {};

// Placeholder type to be used in place of Block IDs
struct IDType {};
/*
 * End of container type names and profiles
 */




struct NullType {};
struct EmptyType {};

struct IncompleteType;
struct TypeIsNotDefined {};

template <typename Name>
struct TypeNotFound;
struct TypeIsNotDefined;

struct SerializationData {
    char* buf;
    size_t total;

    SerializationData(): buf(nullptr), total(0) {}
};

struct DeserializationData {
    const char* buf;
};

enum class WalkDirection {
    UP, DOWN
};

enum class WalkCmd {
    FIRST_LEAF, LAST_LEAF, THE_ONLY_LEAF, FIX_TARGET, NONE, PREFIXES, REFRESH
};


enum class SearchType {GT, GE};
enum class LeafDataLengthType {FIXED, VARIABLE};

extern int64_t DebugCounter;
extern int64_t DebugCounter1;
extern int64_t DebugCounter2;
extern int64_t DebugCounter3;

namespace detail {
    template <typename List>  struct AsTupleH;

    template <typename... Types>
    struct AsTupleH<TL<Types...>> {
        using Type = std::tuple<Types...>;
    };
}

template <typename List>
using AsTuple = typename detail::AsTupleH<List>::Type;


template <typename T>
struct HasType {
    using Type = T;
};

template <typename T, T V_>
struct HasValue {
    static constexpr T Value = V_;
};


namespace detail {
    template <typename T, bool Flag, typename... AdditionalTypes>
    struct FailIfT {
        static_assert(!Flag, "Template failed");
        using Type = T;
    };
}

template <bool Flag, typename T, typename... AdditionalTypes>
using FailIf = typename detail::FailIfT<T, Flag, AdditionalTypes...>::Type;

template <bool Flag, size_t V, typename... AdditionalTypes>
constexpr size_t FailIfV = detail::FailIfT<IntValue<V>, Flag, AdditionalTypes...>::Type::Value;


template <typename T> class ValueCodec;
template <typename T> struct FieldFactory;

template <typename T>
class ValuePtrT1 {
    const T* addr_;
    size_t length_;
public:
    ValuePtrT1(): addr_(), length_() {}
    ValuePtrT1(const T* addr, size_t length): addr_(addr), length_(length) {}

    const T* addr() const {return addr_;}
    size_t length() const {return length_;}
};


template <typename T> struct DataTypeTraits;

namespace detail {
    template <typename T> struct Void {
        using Type = void;
    };

    template <typename T>
    struct IsCompleteHelper {
        template <typename U>
        static auto test(U*)  -> std::integral_constant<bool, sizeof(U) == sizeof(U)>;
        static auto test(...) -> std::false_type;
        using Type = decltype(test((T*)0));
    };
}

template <typename T>
struct IsComplete : detail::IsCompleteHelper<T>::Type {};


template <typename T>
struct HasValueCodec: HasValue<bool, IsComplete<ValueCodec<T>>::type::value> {};

template <typename T>
struct HasFieldFactory: HasValue<bool, IsComplete<FieldFactory<T>>::type::value> {};

template <typename T>
struct IsExternalizable: HasValue<bool, HasValueCodec<T>::Value || HasFieldFactory<T>::Value || DataTypeTraits<T>::isDataType> {};


struct Referenceable {
    virtual ~Referenceable() noexcept = default;
};


template <typename T>
constexpr bool IsPackedStructV = std::is_standard_layout<T>::value && std::is_trivially_copyable<T>::value;

template <typename T>
constexpr bool IsPackedAlignedV = alignof (T) <= 8;


[[noreturn]] void terminate(const char* msg) noexcept;


template <typename Profile>
struct IStoreApiBase {
    virtual ~IStoreApiBase() noexcept = default;
};

template <typename Profile>
struct RWStoreApiBase {
    virtual ~RWStoreApiBase() noexcept = default;
};

template <typename IDType>
struct BlockIDValueHolder {
    IDType id;
};

enum class SeqOpType: size_t {
    EQ, NEQ, LT, LE, GT, GE,

    // This is special sequence search mode suitable for "hierarchically-structured"
    // BTFL cotainers. In this mode we are looking for n-th symbol s, but stop if
    // we found any symbol with smaller cardinality ('not less then'). It's particularly
    // suitable for "remove" operations on BTFL continers. For example, if we have a 3-later
    // BTFL container, like, a wide column table, and want to remove a set of columns
    // in a specific row, 'NLT' means that search operations will have a hard stop
    // at the end of the current row (the next row will start from a symbol with
    // lesser numeric value). This functionality can be also implemented with a
    // combination of EQ and LT searches.
    EQ_NLT
};

// This helper class is usable when we need to provide a type
// to a template method, and don't want to pepend keyword
// 'template' at the call site.
template <typename T> struct TypeTag{};


template <typename T>
T div_2(T value) {
    return value / 2;
}

constexpr size_t SizeTMax = std::numeric_limits<size_t>::max();

}

