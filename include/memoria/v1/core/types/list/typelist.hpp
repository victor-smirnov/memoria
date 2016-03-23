
// Copyright Victor Smirnov 2011-2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/core/types/types.hpp>

#include <memoria/v1/core/tools/type_name.hpp>

#include <iostream>

namespace memoria    {

template<typename...> struct False {
    static const bool Value = false;
};


template <typename List> struct ListSize {
    static_assert(False<List>::Value, "Type supplied to ListSize<> template is not allowed");
};

//template <typename List> struct ListSize;

template <typename ... List>
struct ListSize<TypeList<List...> > {
    static const Int Value = sizeof...(List);
};

template <typename T, T ... List>
struct ListSize<ValueList<T, List...> > {
    static const Int Value = sizeof...(List);
};


template <typename ... List> struct ListHead;

template <typename Head, typename ... Tail>
struct ListHead<TypeList<Head, Tail...>> {
    typedef Head Type;
};

template <typename T, T Head, T ... Tail>
struct ListHead<ValueList<T, Head, Tail...>> {
    static const T Value = Head;
};


template <typename ... List> struct ListTail;

template <typename Head, typename ... Tail>
struct ListTail<TypeList<Head, Tail...>> {
    typedef TypeList<Tail...> Type;
};

template <typename T, T Head, T ... Tail>
struct ListTail<ValueList<T, Head, Tail...>> {
    typedef ValueList<T, Tail...> Type;
};


template <typename T>
struct TypePrinter {
    static std::ostream& print(std::ostream& out)
    {
        out<<::memoria::TypeNameFactory<T>::name();
        return out;
    }

    static std::ostream& println(std::ostream& out)
    {
        print(out);
        out<<std::endl;
        return out;
    }
};


template <typename T> struct ListPrinter;

template <typename Head, typename... Tail>
struct ListPrinter<TypeList<Head, Tail...>> {
    static std::ostream& print(std::ostream& out = std::cout)
    {
        out<<::memoria::TypeNameFactory<Head>::name()<<std::endl;
        return ListPrinter<TypeList<Tail...>>::print(out);
    }
};

template <>
struct ListPrinter<TypeList<>> {
    static std::ostream& print(std::ostream& out = std::cout)
    {
        return out;
    }
};


template <typename T, T Head, T... Tail>
struct ListPrinter<ValueList<T, Head, Tail...>> {
    static std::ostream& print(std::ostream& out = std::cout)
    {
        out<<Head<<std::endl;
        return ListPrinter<ValueList<T, Tail...>>::print(out);
    }
};

template <typename T>
struct ListPrinter<ValueList<T>> {
    static std::ostream& print(std::ostream& out = std::cout)
    {
        return out;
    }
};


template <typename... T>
struct TypesPrinter {
    static std::ostream& print(std::ostream& out = std::cout)
    {
        return ListPrinter<TL<T...>>::print(out);
    }
};

}
