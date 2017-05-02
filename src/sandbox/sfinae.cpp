// Copyright Victor Smirnov 2014.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <memoria/v1/prototypes/bt/tools/bt_tools_packed_struct_list_builder.hpp>
#include <memoria/v1/prototypes/bt/tools/bt_tools_size_list_builder.hpp>

#include <memoria/v1/core/tools/type_name.hpp>
#include <memoria/v1/core/types/list/list_tree.hpp>
#include <memoria/v1/core/types/types.hpp>

#include <typeinfo>
#include <iostream>
#include <vector>


using namespace std;

using namespace memoria::v1;
using namespace memoria::v1::core;
using namespace memoria::v1::bt;
using namespace memoria::v1::list_tree;


template <typename T> struct FnTraitsT;

template <typename RtnType_, typename ClassType_, typename... Args_>
struct FnTraitsT<RtnType_ (ClassType_::*) (Args_&&...)> {
	using RtnType = RtnType_;
	using Exists  = EmptyType;
};

template <typename RtnType_,  typename... Args_>
struct FnTraitsT<RtnType_ (Args_&&...)> {
	using RtnType = RtnType_;
	using Exists = EmptyType;
};


template <typename T, typename... Args>
using Fn1Type = auto(Args&&...) -> decltype(std::declval<T>().stream(std::declval<Args>()...));

template <typename T, int32_t Idx, typename... Args>
using Fn2Type = auto(Args&&...) -> decltype(std::declval<T>().template stream<Idx>(std::declval<Args>()...));

template <typename T, int32_t AccumIdx, int32_t Idx, typename... Args>
using Fn3Type = auto(Args&&...) -> decltype(std::declval<T>().template stream<AccumIdx, Idx>(std::declval<Args>()...));



template <typename T, typename... Args>
using Rtn1Type = typename FnTraits<Fn1Type<typename std::remove_reference<T>::type, Args...>>::RtnType;

template <typename T, int32_t Idx, typename... Args>
using Rtn2Type = typename FnTraits<Fn2Type<typename std::remove_reference<T>::type, Idx, Args...>>::RtnType;

template <typename T, int32_t AccumIdx, int32_t Idx, typename... Args>
using Rtn3Type = typename FnTraits<Fn3Type<typename std::remove_reference<T>::type, AccumIdx, Idx, Args...>>::RtnType;




template <typename T, typename... Args>
using Ex1Type = typename FnTraits<Fn1Type<typename std::remove_reference<T>::type, Args...>>::Exists;

template <typename T, int32_t Idx, typename... Args>
using Ex2Type = typename FnTraits<Fn2Type<typename std::remove_reference<T>::type, Idx, Args...>>::Exists;

template <typename T, int32_t AccumIdx, int32_t Idx, typename... Args>
using Ex3Type = typename FnTraits<Fn3Type<typename std::remove_reference<T>::type, AccumIdx, Idx, Args...>>::Exists;


class TNotDefined;

//template <typename> using void0_t = void;

///////////====================
template <typename Fn, int32_t AccumIdx, int32_t Idx, typename ArgsList, typename T = void>
struct HasFn1H {
	static const int32_t Value = 0;
	using RtnType = TNotDefined;
};

template <typename Fn, int32_t AccumIdx, int32_t Idx, typename... Args> //
struct HasFn1H<Fn, AccumIdx, Idx, TypeList<Args...>, VoidT<decltype(std::declval<Rtn1Type<Fn, Args...>>())>> { //Ex1Type<Fn, Args...>
	static const int32_t Value = 1;
	using RtnType = Rtn1Type<Fn, Args...>;
};

template <typename Fn, int32_t AccumIdx, int32_t Idx, typename... Args>
using HasFn1 = HasFn1H<Fn, AccumIdx, Idx, TypeList<Args...>>;
/////////////==================




template <typename Fn, int32_t AccumIdx, int32_t Idx, typename ArgsList, typename T = void>
struct HasFn2H {
	static const int32_t Value = 0;
	using RtnType = TNotDefined;
};

template <typename Fn, int32_t AccumIdx, int32_t Idx, typename... Args> //
struct HasFn2H<Fn, AccumIdx, Idx, TypeList<Args...>, VoidT<decltype(std::declval<Rtn2Type<Fn, Idx, Args...>>())>> { //Ex2Type<Fn, Idx, Args...>
	static const int32_t Value = 2;
	using RtnType = Rtn2Type<Fn, Idx, Args...>;
};

template <typename Fn, int32_t AccumIdx, int32_t Idx, typename... Args>
using HasFn2 = HasFn2H<Fn, AccumIdx, Idx, TypeList<Args...>>;



template <typename Fn, int32_t AccumIdx, int32_t Idx, typename ArgsList, typename T = void>
struct HasFn3H {
	static const int32_t Value = 0;
	using RtnType = TNotDefined;
};

template <typename Fn, int32_t AccumIdx, int32_t Idx, typename... Args> //
struct HasFn3H<Fn, AccumIdx, Idx, TypeList<Args...>, VoidT<decltype(std::declval<Rtn3Type<Fn, AccumIdx, Idx, Args...>>())>> { //Ex3Type<Fn, AccumIdx, Idx, Args...>
	static const int32_t Value = 3;
	using RtnType = Rtn3Type<Fn, AccumIdx, Idx, Args...>;
};

template <typename Fn, int32_t AccumIdx, int32_t Idx, typename... Args>
using HasFn3 = HasFn3H<Fn, AccumIdx, Idx, TypeList<Args...>>;


template <typename Fn, int AccumIdx, int32_t Idx, typename... Args>
using FnList = TypeList<
		IntValue<HasFn3<Fn, AccumIdx, Idx, Args...>::Value>,
		IntValue<HasFn2<Fn, AccumIdx, Idx, Args...>::Value>,
		IntValue<HasFn1<Fn, AccumIdx, Idx, Args...>::Value>
>;


template <typename Fn, int AccumIdx, int32_t Idx, typename... Args>
using FnRtnType = typename IfThenElse<
		(HasFn3<Fn, AccumIdx, Idx, Args...>::Value > 0),
		typename HasFn3<Fn, AccumIdx, Idx, Args...>::RtnType,
		typename IfThenElse<
			(HasFn2<Fn, AccumIdx, Idx, Args...>::Value > 0),
			typename HasFn2<Fn, AccumIdx, Idx, Args...>::RtnType,
			typename HasFn1<Fn, AccumIdx, Idx, Args...>::RtnType
		>::Result
>::Result;



template <typename List, int32_t AccumIdx, int32_t Idx>
struct FnDispatcher;


template <typename... Tail, int32_t AccumIdx, int32_t Idx>
struct FnDispatcher<TypeList<IntValue<0>, Tail...>, AccumIdx, Idx> {

	template <typename Fn, typename... Args>
	static auto dispatch(Fn&& fn, Args&&... args)
	{
		return FnDispatcher<TypeList<Tail...>, AccumIdx, Idx>::dispatch(std::forward<Fn>(fn), std::forward<Args>(args)...);
	};
};

template <typename... Tail, int32_t AccumIdx, int32_t Idx>
struct FnDispatcher<TypeList<IntValue<1>, Tail...>, AccumIdx, Idx> {
	template <typename Fn, typename... Args>
	static auto dispatch(Fn&& fn, Args&&... args)
	{
		return fn.stream(std::forward<Args>(args)...);
	};
};

template <typename... Tail, int32_t AccumIdx, int32_t Idx>
struct FnDispatcher<TypeList<IntValue<2>, Tail...>, AccumIdx, Idx> {
	template <typename Fn, typename... Args>
	static auto dispatch(Fn&& fn, Args&&... args)
	{
		return fn.template stream<Idx>(std::forward<Args>(args)...);
	};
};


template <typename... Tail, int32_t AccumIdx, int32_t Idx>
struct FnDispatcher<TypeList<IntValue<3>, Tail...>, AccumIdx, Idx> {
	template <typename Fn, typename... Args>
	static auto dispatch(Fn&& fn, Args&&... args)
	{
		return fn.template stream<AccumIdx, Idx>(std::forward<Args>(args)...);
	};
};


template <int32_t AccumIdx, int32_t Idx>
struct FnDispatcher<TypeList<>, AccumIdx, Idx>;


struct Fn0 {};

struct Fn1 {
	void stream(int32_t i, int32_t j) {
		cout<<"Fn1: "<<i<<" "<<j<<endl;
	}
};

struct Fn2 {
	template <int32_t Idx>
	int32_t stream(int32_t i, int32_t j) {
		cout<<"Fn2: "<<Idx<<" "<<i<<" "<<j<<endl;
		return 10;
	}
};

struct Fn3 {
	template <int32_t Idx>
	double stream(int32_t i, int32_t j) {
		cout<<"Fn3: "<<i<<" "<<i<<" "<<j<<endl;
		return 100;
	}

	template <int32_t AccumIdx, int32_t Idx>
	bool stream(int32_t i, int32_t j) {
		cout<<"Fn3: "<<AccumIdx<<" "<<Idx<<" "<<i<<" "<<j<<endl;
		return true;
	}
};

template <int32_t AccumIdx, int32_t Idx, typename Fn, typename... Args>
auto dispatchFn(Fn&& fn, Args&&... args) 
//-> decltype(auto)
//-> FnRtnType<Fn, AccumIdx, Idx, Args...>
{
	using List = FnList<Fn, AccumIdx, Idx, Args...>;
	//using RtnType = FnRtnType<Fn, AccumIdx, Idx, Args...>;

	return FnDispatcher<List, AccumIdx, Idx>::dispatch(std::forward<Fn>(fn), std::forward<Args>(args)...);
}


int main(void)
{
	ListPrinter<TL<decltype(dispatchFn<0,0>(Fn3(), 1, 2))>>::print(cout);

	
	ListPrinter<TL<
		VoidT<decltype(std::declval<Rtn1Type<Fn1, int, int>>())>,
		Ex1Type<Fn1, int, int>,
		HasFn1H<
			Fn1, 1, 2, TL<int, int>
		>
		::RtnType
	>>
	::print(cout);
	
	std::cout << "DispatchFn: " << dispatchFn<555,777>(Fn2(), 1,8) << std::endl;

	//ListPrinter<TL<int>>::print(std::cout);

	ListPrinter<TL<
		FnList<
			Fn3, 1, 2, int, int
		>
	>>
	::print(cout);

	int i;
	std::cin >> i;

    return 0;
}
