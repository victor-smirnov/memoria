
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
    typedef typename AppendValueTool<UInt, 100, typename TypeHash<T>::VList>::Result     VList;

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

template <>             struct TypeHash<VectorMapCtr>:      UIntValue<1000> {};

template <typename Key, typename Value, Int Indexes>
struct TypeHash<MapCtr<Key, Value, Indexes>>:   UIntValue<HashHelper<1100, TypeHash<Key>::Value, TypeHash<Value>::Value, Indexes>::Value> {};

template <Int Indexes>  struct TypeHash<SetCtr<Indexes>>:   UIntValue<HashHelper<1200, Indexes>::Value> {};
template <>             struct TypeHash<VectorCtr>:         UIntValue<1300> {};
template <>             struct TypeHash<RootCtr>:           UIntValue<1400> {};

}

#endif  /* _MEMORIA_CORE_TOOLS_TYPES_HIERARCHY_HPP */

