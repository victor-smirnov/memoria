
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_TOOLS_TYPES_TYPEHASH_HPP
#define _MEMORIA_CORE_TOOLS_TYPES_TYPEHASH_HPP

#include <memoria/core/types/typelist.hpp>
#include <memoria/core/tools/bitmap.hpp>
#include <memoria/core/types/types.hpp>
#include <memoria/core/types/static_md5.hpp>

namespace memoria    {

struct TypeHashes {
    enum {SCALAR = 1, ARRAY, CONST_VALUE};
};


template <>
struct TypeHash<void>: UIntValue<1> {
public:
    typedef ValueList<UInt, 1>                                                  VList;
};

template <> struct TypeHash<Byte>:      UIntValue<2> {};
template <> struct TypeHash<UByte>:     UIntValue<3> {};
template <> struct TypeHash<Short>:     UIntValue<4> {};
template <> struct TypeHash<UShort>:    UIntValue<5> {};
template <> struct TypeHash<Int>:       UIntValue<6> {};
template <> struct TypeHash<UInt>:      UIntValue<7> {};
template <> struct TypeHash<BigInt>:    UIntValue<8> {};
template <> struct TypeHash<UBigInt>:   UIntValue<9> {};

template <> struct TypeHash<EmptyValue>:    UIntValue<10> {};



template <
    template <typename> class Profile,
    typename T
>
struct TypeHash<Profile<T>> {
    // FIXME need template assigning unique code to the each profile level
    typedef typename AppendValueTool<UInt, 100, ValueList<UInt, TypeHash<T>::Value>>::Result     VList;

    static const UInt Value = md5::Md5Sum<VList>::Result::Value32;
};



template <UInt Base, UInt ... Values>
struct HashHelper {
    static const UInt Value = md5::Md5Sum<ValueList<UInt, Base, Values...>>::Result::Value32;
};



template <typename T, T V>
struct TypeHash<ConstValue<T, V>> {
    static const UInt Value = HashHelper<TypeHash<T>::Value, TypeHashes::CONST_VALUE, V>::Value;
};



template <typename T, size_t Size>
struct TypeHash<T[Size]> {
    static const UInt Value = HashHelper<TypeHash<T>::Value, TypeHashes::ARRAY, Size>::Value;
};

template <typename Key, typename Value>
struct TypeHash<VectorMap<Key, Value>>: UIntValue<
    HashHelper<1000, TypeHash<Key>::Value, TypeHash<Value>::Value>::Value
> {};


template <typename Key, typename Value>
struct TypeHash<Map<Key, Value>>:   UIntValue<
    HashHelper<1100, TypeHash<Key>::Value, TypeHash<Value>::Value>::Value
> {};

template <typename Key, typename Value>
struct TypeHash<CMap<Key, Value>>:   UIntValue<
    HashHelper<1112, TypeHash<Key>::Value, TypeHash<Value>::Value>::Value
> {};



template <typename Key, typename Value>
struct TypeHash<MapProto<Key, Value>>:   UIntValue<
    HashHelper<1111, TypeHash<Key>::Value, TypeHash<Value>::Value, 1>::Value
> {};


template <typename T>
struct TypeHash<Vector<T>>: UIntValue<HashHelper<1300, TypeHash<T>::Value>::Value> {};



template <> struct TypeHash<Root>: UIntValue<1400> {};

template <Int BitsPerSymbol, bool Dense>
struct TypeHash<Sequence<BitsPerSymbol, Dense>>: UIntValue<HashHelper<1500, BitsPerSymbol, Dense>::Value> {};

template <typename T, Indexed sr>
struct TypeHash<FLabel<T, sr> >: UIntValue<HashHelper<1610, (UInt)sr>::Value> {};

template <Int BitsPerSymbol>
struct TypeHash<FBLabel<BitsPerSymbol>>: UIntValue<HashHelper<1620, BitsPerSymbol>::Value> {};

template <typename T, Indexed sr, Granularity gr>
struct TypeHash<VLabel<T, gr, sr> >: UIntValue<HashHelper<1630, (UInt)sr, (UInt)gr>::Value> {};


template <typename... LabelDescriptors>
struct TypeHash<LabeledTree<LabelDescriptors...>> {
private:
	typedef typename TypeToValueList<TypeList<LabelDescriptors...>>::Type 	ValueList;
	typedef typename AppendValueTool<UInt, 1600, ValueList>::Result			TaggedValueList;

public:
	static const UInt Value = md5::Md5Sum<TaggedValueList>::Result::Value32;
};


template <typename... LabelDescriptors>
struct TypeHash<LabeledTree<TypeList<LabelDescriptors...>>> {
private:
	typedef typename TypeToValueList<TypeList<LabelDescriptors...>>::Type 	ValueList;
	typedef typename AppendValueTool<UInt, 1600, ValueList>::Result			TaggedValueList;

public:
	static const UInt Value = md5::Md5Sum<TaggedValueList>::Result::Value32;
};


template <typename CtrName>
struct TypeHash<CtrWrapper<CtrName>>: UIntValue<HashHelper<1700, TypeHash<CtrName>::Value>::Value> {};

template <>
struct TypeHash<WT>: UIntValue<1800> {};

template <>
struct TypeHash<VTree>: UIntValue<1900> {};

}


#endif  /* _MEMORIA_CORE_TOOLS_TYPES_HIERARCHY_HPP */

