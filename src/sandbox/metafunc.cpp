
// Copyright Victor Smirnov 2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <memoria/prototypes/bt/tools/bt_packed_struct_list_builder.hpp>
#include <memoria/prototypes/bt/tools/bt_size_list_builder.hpp>
#include <memoria/prototypes/bt/tools/bt_substreamgroup_dispatcher.hpp>

#include <memoria/core/tools/type_name.hpp>
#include <memoria/core/types/list/list_tree.hpp>

#include <typeinfo>
#include <iostream>
#include <vector>


using namespace std;
using namespace memoria;
using namespace memoria::core;
using namespace memoria::bt;
using namespace memoria::list_tree;

template <typename T>
struct type {
	constexpr type() = default;
};

template <typename... L1, typename... L2>
constexpr auto merge_lists(TypeList<L1...> l1, TypeList<L2...> l2) -> TypeList<L1..., L2...> {
	return TypeList<L1..., L2...>();
}

template <typename... L1, typename T>
constexpr auto merge_lists(TypeList<L1...> l1, type<T> t) -> TypeList<L1..., T> {
	return TypeList<L1..., T>();
}

template <typename... L1, typename T>
constexpr auto merge_lists(type<T> t, TypeList<L1...> l1) -> TypeList<T, L1...> {
	return TypeList<T, L1...>();
}

template <typename T1, typename T2>
constexpr auto merge_lists(type<T1>, type<T2>) -> TypeList<T1, T2> {
	return TypeList<T1, T2>();
}

namespace detail {

template <bool C, typename T, typename F> struct SelectH;

template <typename T, typename F>
struct SelectH<true, T, F> {
	using Type = T;
};

}


template <Int Idx, typename Head, typename... Tail>
constexpr auto select(TL<Head, Tail...>)
-> type<typename Select<Idx, TL<Head, Tail...>>::Result>
{
	return type<typename Select<Idx, TL<Head, Tail...>>::Result>();
}


template <typename T>
void print_type(T) {
	TypePrinter<T>::println(cout);
}

int main(void)
{
	print_type(merge_lists(TL<Int, bool>(), TL<Int, double>()));
	print_type(merge_lists(TL<Int, bool>(), type<double>()));
	print_type(merge_lists(type<double>(), TL<Int, bool>()));
	print_type(merge_lists(type<double>(), type<Int>()));

	print_type(select<2>(TL<double, int, bool>()));

    return 0;
}

