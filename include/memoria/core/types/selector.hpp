
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_TOOLS_TYPES_SELECTOR_HPP
#define _MEMORIA_CORE_TOOLS_TYPES_SELECTOR_HPP

#include <memoria/core/types/types.hpp>
#include <memoria/core/types/algo/select.hpp>

namespace memoria {

template <bool V>
using Predicate = typename std::enable_if<V>::type;

struct Any {};

template <typename Name>
struct IsVectorName {
    static const bool Value = false;
};

template <typename Element>
struct IsVectorName<Vector<Element>> {
    static const bool Value = true;
};


template <typename Types, typename Element = Any>
class IsVectorTest {
    typedef typename Types::Name        Name;
    typedef typename Types::ElementType ElementType;

    static const bool IsAVector = IsVectorName<Name>::Value;

    static const bool IsTheVector =
            IfThenElse<
                IfTypesEqual<Element, Any>::Value,
                BoolValue<true>,
                BoolValue<IfTypesEqual<Element, ElementType>::Value>
            >::Result::Value;

public:

    static const bool Value = IsAVector && IsTheVector;
};

template <typename Types, typename Element = Any>
using IsVector = Predicate<IsVectorTest<Types, Element>::Value>;





template <typename Name>
struct IsVectorMapName {
    static const bool Value = false;
};

template <typename Key, typename Value_>
struct IsVectorMapName<VectorMap<Key, Value_>> {
    static const bool Value = true;
};


template <typename Types, typename KeyType = Any, typename ValueType = Any>
class IsVectorMapTest {
    typedef typename Types::Name        Name;
    typedef typename Types::Key         Key;
    typedef typename Types::Value       Value_;

    static const bool IsAVectorMap = IsVectorMapName<Name>::Value;

    static const bool IsKey =
            IfThenElse<
                IfTypesEqual<KeyType, Any>::Value,
                BoolValue<true>,
                BoolValue<IfTypesEqual<KeyType, Key>::Value>
            >::Result::Value;

    static const bool IsValue =
                IfThenElse<
                    IfTypesEqual<ValueType, Any>::Value,
                    BoolValue<true>,
                    BoolValue<IfTypesEqual<ValueType, Value_>::Value>
                >::Result::Value;

public:

    static const bool Value = IsAVectorMap && IsKey && IsValue;
};

template <typename Types, typename Key = Any, typename Value = Any>
using IsVectorMap = Predicate<IsVectorMapTest<Types, Key, Value>::Value>;


}


#endif
