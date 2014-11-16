#include <memoria/core/types/types.hpp>
#include <memoria/core/types/list/append.hpp>

#include <memoria/core/tools/type_name.hpp>

#include <tuple>
#include <iostream>
#include <memory>

using namespace memoria;
using namespace memoria::vapi;
using namespace std;

template <typename T> struct FnTraits;

template <typename RtnType_, typename ClassType_, typename... Args_>
struct FnTraits<RtnType_ (ClassType_::*)(Args_...)> {
	typedef RtnType_ 				RtnType;
	typedef ClassType_ 				ClassType;
	typedef TypeList<Args_...>		ArgsList;

	static const Int Arity			= sizeof...(Args_);
	static const bool Const			= false;


	template <Int I>
	using Arg = typename std::tuple_element<I, std::tuple<Args_...>>::type;
};

template <typename RtnType_, typename ClassType_, typename... Args_>
struct FnTraits<RtnType_ (ClassType_::*)(Args_...) const> {
	typedef RtnType_ 				RtnType;
	typedef ClassType_ 				ClassType;
	typedef TypeList<Args_...>		ArgsList;

	static const Int Arity			= sizeof...(Args_);
	static const bool Const			= true;

	template <Int I>
	using Arg = typename std::tuple_element<I, std::tuple<Args_...>>::type;
};



template <typename T, Int Size>
struct MakeList {
	typedef typename MergeLists<
				T,
				typename MakeList<T, Size - 1>::Type
	>::Result 																	Type;
};

template <typename T>
struct MakeList<T, 0> {
	typedef TypeList<> 															Type;
};

template <typename T> struct MakeTupleH;

template <typename... List>
struct MakeTupleH<TypeList<List...>> {
	typedef std::tuple<List...> 												Type;
};

template <typename T, Int Size>
using MakeTuple = typename MakeTupleH<typename MakeList<T, Size>::Type>::Type;

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

int main() {

	int r = 1;

	auto l = [&r](int a, int b, int c) -> int {
		cout<<a<<" "<<b<<" "<<c<<endl;
		return r++;
	};

	cout<<TypeNameFactory<decltype(Disp::dispatchAllRtn(l, 3, 2, 1))>::name()<<endl;

	auto result = Disp::dispatchAllRtn(l, 3, 2, 1);

	PrintTuple(cout, result);
}

