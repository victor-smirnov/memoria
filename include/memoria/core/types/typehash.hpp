
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

template <typename Type> class TypeHash;

template <>
class TypeHash<void> {
public:
    typedef ValueList<UInt, 0>                                                  VList;
    static const UInt Value = 0;
};

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

template <
    template <typename> class Profile,
    typename T
>
class TypeHash<Profile<T>> {
public:
    typedef typename AppendValueTool<UInt, 15, typename TypeHash<T>::VList>::Result     VList;

    static const UInt Value = md5::Md5Sum<VList>::Result::Value32;
};

template <typename T, T V>
class TypeHash<ConstValue<T, V>> {
public:
    static const UInt Value = (TypeHash<T>::Value << 16) + V;
};

}

#endif  /* _MEMORIA_CORE_TOOLS_TYPES_HIERARCHY_HPP */

