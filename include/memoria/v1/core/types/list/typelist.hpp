
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

#include <memoria/v1/core/types/types.hpp>

#include <memoria/v1/core/tools/type_name.hpp>

#include <iostream>

namespace memoria {
namespace v1 {

template<typename...> struct False {
    static const bool Value = false;
};


template <typename List> struct ListSize {
    static_assert(False<List>::Value, "Type supplied to ListSize<> template is not allowed");
};

//template <typename List> struct ListSize;

template <typename ... List>
struct ListSize<v1::TypeList<List...> > {
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
        out<<v1::TypeNameFactory<T>::name();
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
        out<<v1::TypeNameFactory<Head>::name()<<std::endl;
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

}}
