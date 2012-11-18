
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_TOOLS_TYPES_TYPEHASH_HPP
#define _MEMORIA_CORE_TOOLS_TYPES_TYPEHASH_HPP

#include <memoria/core/types/typelist.hpp>
#include <memoria/core/tools/bitmap.hpp>
#include <memoria/core/types/types.hpp>

namespace memoria    {

template <typename Type>
class TypeHash {
public:
    static const UInt Value = Type::kHashCode;
};

template <UInt HashValue>
class HashCode{};

template <UInt HashValue>
class TypeHash<HashCode<HashValue> > {
public:
    static const UInt Value = HashValue;
};


template <typename List> struct ListHash;

template <typename Head, typename Tail>
struct ListHash<TL<Head, Tail> > {
    static const UInt Value = CSHR<UInt, TypeHash<Head>::Value, 1>::Value ^ ListHash<Tail>::Value;
};

template <>
struct ListHash<NullType> {
    static const UInt Value = 0;
};


inline Int PtrToInt(const void *ptr)
{
    return (Int) (T2T<BigInt>(ptr) & 0xFFFFFFFF);
}

template <typename Type>
Int Hash(Type *ptr) {
    return CShr(PtrToInt(ptr), 2) ^ TypeHash<Type>::Value;
}



template <>
class TypeHash<Byte> {
public:
    static const UInt Value = 1;
};

template <>
class TypeHash<Short> {
public:
    static const UInt Value = 2;
};

template <>
class TypeHash<Int> {
public:
    static const UInt Value = 3;
};

template <>
class TypeHash<BigInt> {
public:
    static const UInt Value = 5;
};

template <>
class TypeHash<UByte> {
public:
    static const UInt Value = 11;
};

template <>
class TypeHash<UShort> {
public:
    static const UInt Value = 12;
};

template <>
class TypeHash<UInt> {
public:
    static const UInt Value = 13;
};

template <>
class TypeHash<EmptyValue> {
public:
    static const UInt Value = 14;
};


}

#endif  /* _MEMORIA_CORE_TOOLS_TYPES_HIERARCHY_HPP */

