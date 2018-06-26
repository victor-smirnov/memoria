
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

#include <memoria/v1/core/types/typelist.hpp>
#include <memoria/v1/core/tools/bitmap.hpp>
#include <memoria/v1/core/types.hpp>
#include <memoria/v1/core/types/static_md5.hpp>

#include <tuple>

namespace memoria {
namespace v1 {

struct TypeHashes {
    enum {SCALAR = 1, ARRAY, CONST_VALUE};
};


template <>
struct TypeHash<void>: UInt64Value<1> {
public:
    using VList = ValueList<uint64_t, 1>;
};

template <> struct TypeHash<int8_t>:      UInt64Value<2> {};
template <> struct TypeHash<uint8_t>:     UInt64Value<3> {};
template <> struct TypeHash<int16_t>:     UInt64Value<4> {};
template <> struct TypeHash<uint16_t>:    UInt64Value<5> {};
template <> struct TypeHash<int32_t>:     UInt64Value<6> {};
template <> struct TypeHash<uint32_t>:    UInt64Value<7> {};
template <> struct TypeHash<int64_t>:     UInt64Value<8> {};
template <> struct TypeHash<uint64_t>:    UInt64Value<9> {};
template <> struct TypeHash<float>:       UInt64Value<10> {};
template <> struct TypeHash<double>:      UInt64Value<11> {};

template <> struct TypeHash<EmptyValue>:  UInt64Value<10> {};



template <
    template <typename> class Profile,
    typename T
>
struct TypeHash<Profile<T>> {
    // FIXME need template assigning unique code to the each profile level
    using VList = UInt64List<100, TypeHash<T>::Value>;

    static constexpr uint64_t Value = md5::Md5Sum<VList>::Result::Value64;
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


template <typename Key, typename Value>
struct TypeHash<Map<Key, Value>>: UInt64Value<
    HashHelper<1100, TypeHashV<Key>, TypeHashV<Value>>
> {};

template <typename Key, typename Value>
struct TypeHash<CowMap<Key, Value>>: UInt64Value<
    HashHelper<1104, TypeHashV<Key>, TypeHashV<Value>>
> {};


template <typename Key>
struct TypeHash<Set<Key>>: UInt64Value<
    HashHelper<1101, TypeHashV<Key>>
> {};


template <typename Key, typename Value>
struct TypeHash<Multimap<Key, Value>>: UInt64Value<
    HashHelper<1102, TypeHashV<Key>, TypeHashV<Value>>
> {};




template <typename T, Granularity gr>
struct TypeHash<VLen<gr, T>>: UInt64Value<
    HashHelper<1113, TypeHashV<T>, static_cast<uint64_t>(gr)>
> {};


template <typename T>
struct TypeHash<Vector<T>>: UInt64Value<HashHelper<1300, TypeHashV<T>>> {};



template <> struct TypeHash<Root>: UInt64Value<1400> {};

template <int32_t BitsPerSymbol, bool Dense>
struct TypeHash<Sequence<BitsPerSymbol, Dense>>: UInt64Value<HashHelper<1500, BitsPerSymbol, Dense>> {};

template <typename T, Indexed sr>
struct TypeHash<FLabel<T, sr> >: UInt64Value<HashHelper<1610, (uint64_t)sr>> {};

template <int32_t BitsPerSymbol>
struct TypeHash<FBLabel<BitsPerSymbol>>: UInt64Value<HashHelper<1620, BitsPerSymbol>> {};

template <typename T, Indexed sr, Granularity gr>
struct TypeHash<VLabel<T, gr, sr> >: UInt64Value<HashHelper<1630, (uint64_t)sr, (uint64_t)gr>> {};


template <typename... LabelDescriptors>
struct TypeHash<LabeledTree<LabelDescriptors...>> {
private:
    using ValueList = typename TypeToValueList<TypeList<LabelDescriptors...>>::Type;
    using TaggedValueList = MergeValueLists<UInt64Value<1600>, ValueList>;

public:
    static const uint64_t Value = md5::Md5Sum<TaggedValueList>::Type::Value64;
};


template <typename... LabelDescriptors>
struct TypeHash<LabeledTree<TypeList<LabelDescriptors...>>>: TypeHash<LabeledTree<LabelDescriptors...>> {};


template <typename CtrName>
struct TypeHash<CtrWrapper<CtrName>>: UInt32Value<HashHelper<1700, TypeHashV<CtrName>>> {};

template <>
struct TypeHash<WT>: UInt64Value<1800> {};

template <>
struct TypeHash<VTree>: UInt64Value<1900> {};





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
struct TypeHash<Table<Key, Value, PackedSizeType::FIXED>>: UInt64Value <
    HashHelper<3098, TypeHashV<Key>, TypeHashV<Value>>
> {};

template <typename Key, typename Value>
struct TypeHash<Table<Key, Value, PackedSizeType::VARIABLE>>: UInt64Value <
    HashHelper<3099, TypeHashV<Key>, TypeHashV<Value>>
> {};

template <>
struct TypeHash<EdgeMap>: UInt64Value <
    HashHelper<4000>
> {};

template <>
struct TypeHash<UpdateLog>: UInt64Value <
    HashHelper<4001>
> {};


}}
