
// Copyright 2011 Victor Smirnov
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

#include <memoria/core/types/typelist.hpp>
#include <memoria/core/tools/bitmap.hpp>
#include <memoria/core/types/static_md5.hpp>

#include <tuple>

namespace memoria {

static constexpr uint8_t SHORT_TYPEHASH_LENGTH_BASE = 250;

struct TypeHashes {
    enum {SCALAR = 1, ARRAY, CONST_VALUE};
};

// Type Code value 0 means 'no code'

template <> struct TypeHash<int8_t>:    UInt64Value<1> {};
template <> struct TypeHash<uint8_t>:   UInt64Value<2> {};
template <> struct TypeHash<int16_t>:   UInt64Value<3> {};
template <> struct TypeHash<uint16_t>:  UInt64Value<4> {};
template <> struct TypeHash<int32_t>:   UInt64Value<5> {};
template <> struct TypeHash<uint32_t>:  UInt64Value<6> {};
template <> struct TypeHash<int64_t>:   UInt64Value<7> {};
template <> struct TypeHash<uint64_t>:  UInt64Value<8> {};
template <> struct TypeHash<float>:     UInt64Value<9> {};
template <> struct TypeHash<double>:    UInt64Value<10> {};
template <> struct TypeHash<void>:      UInt64Value<11> {};

// Core datatypes: 20 - 39
// LinkedData: 40 - 44
// Reserved for Linked Data : 250 - 255

template <
    template <typename> class Profile,
    typename T
>
struct TypeHash<Profile<T>> {
    // FIXME need template assigning unique code to the each profile level
    using VList = UInt64List<100, TypeHash<T>::Value>;
    static constexpr uint64_t Value = md5::Md5Sum<VList>::Type::Value64;
};





template <uint64_t Base, uint64_t ... Values>
static constexpr uint64_t HashHelper = md5::Md5Sum<UInt64List<Base, Values...>>::Type::Value64;


template <typename T, T V>
struct TypeHash<ConstValue<T, V>> {
    static const uint64_t Value = HashHelper<TypeHashV<T>, TypeHashes::CONST_VALUE, V>;
};



template <typename T, size_t Size>
struct TypeHash<T[Size]> {
    static const uint64_t Value = HashHelper<TypeHashV<T>, TypeHashes::ARRAY, Size>;
};


template <> struct TypeHash<Root>: UInt64Value<1400> {};

template <int32_t BitsPerSymbol>
struct TypeHash<Sequence<BitsPerSymbol>>: UInt64Value<HashHelper<1500, BitsPerSymbol>> {};


template <typename CtrName>
struct TypeHash<CtrWrapper<CtrName>>: UInt32Value<HashHelper<1700, TypeHashV<CtrName>>> {};


template <typename... List>
struct TypeHash<TypeList<List...>> {
private:
    using ValueList       = typename TypeToValueList<TypeList<List...>>::Type;
    using TaggedValueList = MergeValueLists<UInt64Value<2000>, ValueList>;
public:

    static const uint64_t Value = md5::Md5Sum<TaggedValueList>::Result::Value64;
};

template <typename... List>
struct TypeHash<std::tuple<List...>> {
private:
    using ValueList       = typename TypeToValueList<TypeList<List...>>::Type;
    using TaggedValueList = MergeValueLists<UInt64Value<2001>, ValueList>;
public:

    static const uint64_t Value = md5::Md5Sum<TaggedValueList>::Result::Value64;
};

template <typename Key, typename Value>
struct TypeHash<Table<Key, Value, PackedDataTypeSize::FIXED>>: UInt64Value <
    HashHelper<3098, TypeHashV<Key>, TypeHashV<Value>>
> {};

template <typename Key, typename Value>
struct TypeHash<Table<Key, Value, PackedDataTypeSize::VARIABLE>>: UInt64Value <
    HashHelper<3099, TypeHashV<Key>, TypeHashV<Value>>
> {};




}
