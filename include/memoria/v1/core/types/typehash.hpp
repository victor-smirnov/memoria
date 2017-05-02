
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
#include <memoria/v1/core/types/types.hpp>
#include <memoria/v1/core/types/static_md5.hpp>

#include <tuple>

namespace memoria {
namespace v1 {

struct TypeHashes {
    enum {SCALAR = 1, ARRAY, CONST_VALUE};
};


template <>
struct TypeHash<void>: UIntValue<1> {
public:
    typedef ValueList<uint32_t, 1>                                                  VList;
};

template <> struct TypeHash<int8_t>:      UIntValue<2> {};
template <> struct TypeHash<uint8_t>:     UIntValue<3> {};
template <> struct TypeHash<int16_t>:     UIntValue<4> {};
template <> struct TypeHash<uint16_t>:    UIntValue<5> {};
template <> struct TypeHash<int32_t>:       UIntValue<6> {};
template <> struct TypeHash<uint32_t>:      UIntValue<7> {};
template <> struct TypeHash<int64_t>:    UIntValue<8> {};
template <> struct TypeHash<uint64_t>:   UIntValue<9> {};
template <> struct TypeHash<float>:     UIntValue<10> {};
template <> struct TypeHash<double>:    UIntValue<11> {};

template <> struct TypeHash<EmptyValue>:    UIntValue<10> {};



template <
    template <typename> class Profile,
    typename T
>
struct TypeHash<Profile<T>> {
    // FIXME need template assigning unique code to the each profile level
    typedef typename AppendValueTool<uint32_t, 100, ValueList<uint32_t, TypeHash<T>::Value>>::Result     VList;

    static const uint32_t Value = md5::Md5Sum<VList>::Result::Value32;
};





template <uint32_t Base, uint32_t ... Values>
struct HashHelper {
    static const uint32_t Value = md5::Md5Sum<ValueList<uint32_t, Base, Values...>>::Type::Value32;
};






template <typename T, T V>
struct TypeHash<ConstValue<T, V>> {
    static const uint32_t Value = HashHelper<TypeHash<T>::Value, TypeHashes::CONST_VALUE, V>::Value;
};



template <typename T, size_t Size>
struct TypeHash<T[Size]> {
    static const uint32_t Value = HashHelper<TypeHash<T>::Value, TypeHashes::ARRAY, Size>::Value;
};


template <typename Key, typename Value>
struct TypeHash<Map<Key, Value>>:   UIntValue<
    HashHelper<1100, TypeHash<Key>::Value, TypeHash<Value>::Value>::Value
> {};

template <typename Key, typename Value>
struct TypeHash<CowMap<Key, Value>>:   UIntValue<
    HashHelper<1104, TypeHash<Key>::Value, TypeHash<Value>::Value>::Value
> {};


template <typename Key>
struct TypeHash<Set<Key>>:   UIntValue<
    HashHelper<1101, TypeHash<Key>::Value>::Value
> {};







template <typename T, Granularity gr>
struct TypeHash<VLen<gr, T>>: UIntValue<

HashHelper<1113, TypeHash<T>::Value, static_cast<uint32_t>(gr)>::Value
> {};


template <typename T>
struct TypeHash<Vector<T>>: UIntValue<HashHelper<1300, TypeHash<T>::Value>::Value> {};



template <> struct TypeHash<Root>: UIntValue<1400> {};

template <int32_t BitsPerSymbol, bool Dense>
struct TypeHash<Sequence<BitsPerSymbol, Dense>>: UIntValue<HashHelper<1500, BitsPerSymbol, Dense>::Value> {};

template <typename T, Indexed sr>
struct TypeHash<FLabel<T, sr> >: UIntValue<HashHelper<1610, (uint32_t)sr>::Value> {};

template <int32_t BitsPerSymbol>
struct TypeHash<FBLabel<BitsPerSymbol>>: UIntValue<HashHelper<1620, BitsPerSymbol>::Value> {};

template <typename T, Indexed sr, Granularity gr>
struct TypeHash<VLabel<T, gr, sr> >: UIntValue<HashHelper<1630, (uint32_t)sr, (uint32_t)gr>::Value> {};


template <typename... LabelDescriptors>
struct TypeHash<LabeledTree<LabelDescriptors...>> {
private:
    typedef typename TypeToValueList<TypeList<LabelDescriptors...>>::Type   ValueList;
    typedef typename AppendValueTool<uint32_t, 1600, ValueList>::Type         TaggedValueList;

public:
    static const uint32_t Value = md5::Md5Sum<TaggedValueList>::Type::Value32;
};


template <typename... LabelDescriptors>
struct TypeHash<LabeledTree<TypeList<LabelDescriptors...>>>: TypeHash<LabeledTree<LabelDescriptors...>> {};


template <typename CtrName>
struct TypeHash<CtrWrapper<CtrName>>: UIntValue<HashHelper<1700, TypeHash<CtrName>::Value>::Value> {};

template <>
struct TypeHash<WT>: UIntValue<1800> {};

template <>
struct TypeHash<VTree>: UIntValue<1900> {};





template <typename... List>
struct TypeHash<TypeList<List...>> {
private:
    typedef typename TypeToValueList<TypeList<List...>>::Type                   ValueList;
    typedef typename AppendValueTool<uint32_t, 2000, ValueList>::Result             TaggedValueList;
public:

    static const uint32_t Value = md5::Md5Sum<TaggedValueList>::Result::Value32;
};

template <typename... List>
struct TypeHash<std::tuple<List...>> {
private:
    typedef typename TypeToValueList<TypeList<List...>>::Type                   ValueList;
    typedef typename AppendValueTool<uint32_t, 2001, ValueList>::Result             TaggedValueList;
public:

    static const uint32_t Value = md5::Md5Sum<TaggedValueList>::Result::Value32;
};

template <typename Key, typename Value>
struct TypeHash<Table<Key, Value, PackedSizeType::FIXED>>:   UIntValue<
    HashHelper<3098, TypeHash<Key>::Value, TypeHash<Value>::Value>::Value
> {};

template <typename Key, typename Value>
struct TypeHash<Table<Key, Value, PackedSizeType::VARIABLE>>:   UIntValue<
    HashHelper<3099, TypeHash<Key>::Value, TypeHash<Value>::Value>::Value
> {};

}}
