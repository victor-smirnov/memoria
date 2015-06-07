
// Copyright Victor Smirnov 2014.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CORE_TYPES_LIST_SUBLIST_HPP_
#define MEMORIA_CORE_TYPES_LIST_SUBLIST_HPP_

#include <memoria/core/types/types.hpp>
#include <memoria/core/types/list/append.hpp>

namespace memoria {


template <typename List, Int Len> struct SublistFromStart;

template <typename T, T Head, T... Tail, Int Len>
struct SublistFromStart<ValueList<T, Head, Tail...>, Len> {
    static_assert(Len >= 0, "Len parameter must be >= 0");
    static_assert(Len <= sizeof...(Tail) + 1, "Len parameter must be <= the Length of value list");

    using Type = MergeValueLists<
                ConstValue<T, Head>,
                typename SublistFromStart<ValueList<T, Tail...>, Len - 1>::Type
    >;
};



template <typename T, T Head, T... Tail>
struct SublistFromStart<ValueList<T, Head, Tail...>, 0> {
    using Type = ValueList<T>;
};

template <typename T>
struct SublistFromStart<ValueList<T>, 0> {
    using Type = ValueList<T>;
};




template <typename Head, typename... Tail, Int Len>
struct SublistFromStart<TypeList<Head, Tail...>, Len> {
    static_assert(Len >= 0, "Len parameter must be >= 0");
    static_assert(Len <= sizeof...(Tail) + 1, "Len parameter must be <= the Length of value list");

    using Type = MergeLists<
                Head,
                typename SublistFromStart<TypeList<Tail...>, Len - 1>::Type
    >;
};

template <typename Head, typename... Tail>
struct SublistFromStart<TypeList<Head, Tail...>, 0> {
    using Type = TypeList<>;
};

template <>
struct SublistFromStart<TypeList<>, 0> {
    using Type = TypeList<>;
};



template <typename List, Int From> struct SublistToEnd;

template <typename T, T Head, T... Tail>
struct SublistToEnd<ValueList<T, Head, Tail...>, 0> {
    using Type = ValueList<T, Head, Tail...>;
};

template <typename T>
struct SublistToEnd<ValueList<T>, 0> {
    using Type = ValueList<T>;
};


template <typename T, T Head, T ... Tail, Int From>
struct SublistToEnd<ValueList<T, Head, Tail...>, From> {
    static_assert(From >= 0, "Form must be >= 0");
    static_assert(From < sizeof...(Tail) + 1, "Form must be <= length of the list");

    using Type = typename SublistToEnd<ValueList<T, Tail...>, From - 1>::Type;
};



template <typename Head, typename... Tail>
struct SublistToEnd<TypeList<Head, Tail...>, 0> {
    using Type = TypeList<Head, Tail...>;
};

template <>
struct SublistToEnd<TypeList<>, 0> {
    using Type = TypeList<>;
};

template <typename Head, typename... Tail, Int From>
struct SublistToEnd<TypeList<Head, Tail...>, From> {
    static_assert(From >= 0, "Form must be >= 0");
    static_assert(From <= sizeof...(Tail) + 1, "Form must be <= length of the list");

    using Type = typename SublistToEnd<TypeList<Tail...>, From - 1>::Type;
};



template <typename List, Int From, Int To>
struct Sublist {
    static_assert(From <= To, "Form must be <= To");

    using Type = typename SublistFromStart<
            typename SublistToEnd<List, From>::Type,
            To - From
    >::Type;
};

}

#endif
