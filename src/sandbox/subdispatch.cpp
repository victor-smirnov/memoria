
// Copyright Victor Smirnov 2014.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <memoria/core/types/types.hpp>
#include <memoria/core/types/fn_traits.hpp>
#include <memoria/core/types/list/append.hpp>
#include <memoria/core/types/list/tuple.hpp>

#include <memoria/core/tools/type_name.hpp>

#include <tuple>
#include <iostream>
#include <memory>

using namespace memoria;
using namespace memoria::vapi;
using namespace std;

template <typename List> struct Dispatcher;

template <
    typename Head,
    typename... Tail
>
struct Dispatcher<TypeList<Head, Tail...>> {

    template <typename Fn>
    using RtnType = typename FnTraits<decltype(&std::remove_reference<Fn>::type::operator())>::RtnType;

    template <typename Fn, Int Size>
    using RtnTuple = MakeTuple<RtnType<Fn>, sizeof...(Tail) + 1>;

    template<typename List> friend struct Dispatcher;

    template <typename Fn, typename... Args>
    static void dispatchAll(Fn&& fn, Args&&... args)
    {
        fn(args...);
        Dispatcher<TypeList<Tail...>>::dispatchAll(std::forward<Fn>(fn), std::forward<Args>(args)...);
    }

    template <typename Fn, typename... Args>
    static RtnTuple<Fn, 3> dispatchAllRtn(Fn&& fn, Args&&... args)
    {
        RtnTuple<Fn, 3> result;

        dispatchAll_<0>(result, std::forward<Fn>(fn), std::forward<Args>(args)...);

        return result;
    }

private:
    template <Int Idx, typename Fn, typename Tuple, typename... Args>
    static void dispatchAll_(Tuple& tuple, Fn&& fn, Args&&... args)
    {
        std::get<Idx>(tuple) = fn(args...);
        Dispatcher<TypeList<Tail...>>::template dispatchAll_<Idx + 1>(tuple, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }
};

template <>
struct Dispatcher<TypeList<>> {
    template<typename List> friend struct Dispatcher;

    template <typename Fn, typename... Args>
    static void dispatchAll(Fn&& fn, Args&&... args){}

private:
    template <Int Idx, typename Fn, typename Tuple, typename... Args>
    static void dispatchAll_(Tuple& tuple, Fn&& fn, Args&&... args) {}
};


class T{};

typedef TypeList <T, T, T> List;

typedef Dispatcher<List> Disp;

namespace {

template <typename T, Int Idx = std::tuple_size<T>::value> struct PrintTupleH;

template <typename... List, Int Idx>
struct PrintTupleH<std::tuple<List...>, Idx> {

    typedef std::tuple<List...> T;

    static void print(std::ostream& out, const T& t)
    {
        out<<std::get<std::tuple_size<T>::value - Idx>(t)<<std::endl;
        PrintTupleH<T, Idx - 1>::print(out, t);
    }
};


template <typename... List>
struct PrintTupleH<std::tuple<List...>, 0> {
    typedef std::tuple<List...> T;
    static void print(std::ostream& out, const T& t) {}
};

}

template <typename T>
void PrintTuple(std::ostream& out, const T& t) {
    PrintTupleH<T>::print(out, t);
}



template <typename T> struct TT1 {};
template <typename T> struct TT2 {};


//template <typename Fn, typename... Args>
//using FooRtnType = typename FnTraits<decltype(&Fn::template foo<0>(Args...))>::RtnType;



struct Boo {
    template <Int Idx, typename T>
    int foo(const TT1<T>*, int, long) {
        return 1;
    }

    template <Int Idx, typename T>
    int foo(const TT2<T>*, int, long) {
        return 2;
    }

    template <Int Idx, typename T>
    int foo(const T*, int, long) {
        return 2;
    }
};

struct Foo: Boo {

    Foo(int) {}

//  template <Int Idx>
//  static int foo(int, int, long) {
//      return 0;
//  }
};


template <typename T, typename... Args>
using FnType = auto(Args...) -> decltype(std::declval<T>().template foo<0>(std::declval<Args>()...));

int main() {

    int r = 1;

    auto l = [&r](int a, int b, int c) -> int {
        cout<<a<<" "<<b<<" "<<c<<endl;
        return r++;
    };

    cout<<TypeNameFactory<decltype(Disp::dispatchAllRtn(l, 3, 2, 1))>::name()<<endl;

    auto result = Disp::dispatchAllRtn(l, 3, 2, 1);

    PrintTuple(cout, result);

    const TT1<T>* t = nullptr;

    cout<<"foo: "<<Foo(1).foo<0>(t, 1, 2)<<endl;

//  cout<<TypeNameFactory<FooRtnType<Boo, T>>::name()<<endl;

    cout<<"fooT: "<<TypeNameFactory<FnTraits<FnType<Foo, const TT1<T>*, int, int>>::RtnType>::name()<<endl;

}

