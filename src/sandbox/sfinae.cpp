


#include <memoria/prototypes/bt/tools/bt_packed_struct_list_builder.hpp>
#include <memoria/prototypes/bt/tools/bt_size_list_builder.hpp>

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


template <typename T, typename... Args>
using Fn1Type = auto(Args...) -> decltype(std::declval<T>().stream(std::declval<Args>()...));

template <typename T, Int Idx, typename... Args>
using Fn2Type = auto(Args...) -> decltype(std::declval<T>().template stream<Idx>(std::declval<Args>()...));

template <typename T, Int AccumIdx, Int Idx, typename... Args>
using Fn3Type = auto(Args...) -> decltype(std::declval<T>().template stream<AccumIdx, Idx>(std::declval<Args>()...));


template <typename T, typename... Args>
using Rtn1Type = typename FnTraits<Fn1Type<typename std::remove_reference<T>::type, Args...>>::RtnType;

template <typename T, Int Idx, typename... Args>
using Rtn2Type = typename FnTraits<Fn2Type<typename std::remove_reference<T>::type, Idx, Args...>>::RtnType;

template <typename T, Int AccumIdx, Int Idx, typename... Args>
using Rtn3Type = typename FnTraits<Fn3Type<typename std::remove_reference<T>::type, AccumIdx, Idx, Args...>>::RtnType;


template <typename T, typename... Args>
using Ex1Type = typename FnTraits<Fn1Type<typename std::remove_reference<T>::type, Args...>>::Exists;

template <typename T, Int Idx, typename... Args>
using Ex2Type = typename FnTraits<Fn2Type<typename std::remove_reference<T>::type, Idx, Args...>>::Exists;

template <typename T, Int AccumIdx, Int Idx, typename... Args>
using Ex3Type = typename FnTraits<Fn3Type<typename std::remove_reference<T>::type, AccumIdx, Idx, Args...>>::Exists;


class TNotDefined;


template <typename Fn, Int AccumIdx, Int Idx, typename ArgsList, typename T = EmptyType>
struct HasFn1H {
	static const Int Value = 0;
	using RtnType = TNotDefined;
};

template <typename Fn, Int AccumIdx, Int Idx, typename... Args>
struct HasFn1H<Fn, AccumIdx, Idx, TypeList<Args...>, Ex1Type<Fn, Args...>> {
	static const Int Value = 1;
	using RtnType = Rtn1Type<Fn, Args...>;
};

template <typename Fn, Int AccumIdx, Int Idx, typename... Args>
using HasFn1 = HasFn1H<Fn, AccumIdx, Idx, TypeList<Args...>>;


template <typename Fn, Int AccumIdx, Int Idx, typename ArgsList, typename T = EmptyType>
struct HasFn2H {
	static const Int Value = 0;
	using RtnType = TNotDefined;
};

template <typename Fn, Int AccumIdx, Int Idx, typename... Args>
struct HasFn2H<Fn, AccumIdx, Idx, TypeList<Args...>, Ex2Type<Fn, Idx, Args...>> {
	static const Int Value = 2;
	using RtnType = Rtn2Type<Fn, Idx, Args...>;
};

template <typename Fn, Int AccumIdx, Int Idx, typename... Args>
using HasFn2 = HasFn2H<Fn, AccumIdx, Idx, TypeList<Args...>>;



template <typename Fn, Int AccumIdx, Int Idx, typename ArgsList, typename T = EmptyType>
struct HasFn3H {
	static const Int Value = 0;
	using RtnType = TNotDefined;
};

template <typename Fn, Int AccumIdx, Int Idx, typename... Args>
struct HasFn3H<Fn, AccumIdx, Idx, TypeList<Args...>, Ex3Type<Fn, AccumIdx, Idx, Args...>> {
	static const Int Value = 3;
	using RtnType = Rtn3Type<Fn, AccumIdx, Idx, Args...>;
};

template <typename Fn, Int AccumIdx, Int Idx, typename... Args>
using HasFn3 = HasFn3H<Fn, AccumIdx, Idx, TypeList<Args...>>;


template <typename Fn, int AccumIdx, Int Idx, typename... Args>
using FnList = TypeList<
		IntValue<HasFn3<Fn, AccumIdx, Idx, Args...>::Value>,
		IntValue<HasFn2<Fn, AccumIdx, Idx, Args...>::Value>,
		IntValue<HasFn1<Fn, AccumIdx, Idx, Args...>::Value>
>;


template <typename Fn, int AccumIdx, Int Idx, typename... Args>
using FnRtnType = typename IfThenElse<
		(HasFn3<Fn, AccumIdx, Idx, Args...>::Value > 0),
		typename HasFn3<Fn, AccumIdx, Idx, Args...>::RtnType,
		typename IfThenElse<
			(HasFn2<Fn, AccumIdx, Idx, Args...>::Value > 0),
			typename HasFn2<Fn, AccumIdx, Idx, Args...>::RtnType,
			typename IfThenElse<
				(HasFn1<Fn, AccumIdx, Idx, Args...>::Value > 0),
				typename HasFn1<Fn, AccumIdx, Idx, Args...>::RtnType,
				TNotDefined
			>::Result
		>::Result
>::Result;



template <typename List, Int AccumIdx, Int Idx, typename RtnType>
struct FnDispatcher;


template <typename... Tail, Int AccumIdx, Int Idx, typename RtnType>
struct FnDispatcher<TypeList<IntValue<0>, Tail...>, AccumIdx, Idx, RtnType> {
	template <typename Fn, typename... Args>
	static RtnType dispatch(Fn&& fn, Args&&... args)
	{
		return FnDispatcher<TypeList<Tail...>, AccumIdx, Idx, RtnType>::dispatch(std::forward<Fn>(fn), std::forward<Args>(args)...);
	};
};

template <typename... Tail, Int AccumIdx, Int Idx, typename RtnType>
struct FnDispatcher<TypeList<IntValue<1>, Tail...>, AccumIdx, Idx, RtnType> {
	template <typename Fn, typename... Args>
	static RtnType dispatch(Fn&& fn, Args&&... args)
	{
		return fn.stream(std::forward<Args>(args)...);
	};
};

template <typename... Tail, Int AccumIdx, Int Idx, typename RtnType>
struct FnDispatcher<TypeList<IntValue<2>, Tail...>, AccumIdx, Idx, RtnType> {
	template <typename Fn, typename... Args>
	static RtnType dispatch(Fn&& fn, Args&&... args)
	{
		return fn.template stream<Idx>(std::forward<Args>(args)...);
	};
};


template <typename... Tail, Int AccumIdx, Int Idx, typename RtnType>
struct FnDispatcher<TypeList<IntValue<3>, Tail...>, AccumIdx, Idx, RtnType> {
	template <typename Fn, typename... Args>
	static RtnType dispatch(Fn&& fn, Args&&... args)
	{
		return fn.template stream<AccumIdx, Idx>(std::forward<Args>(args)...);
	};
};


template <Int AccumIdx, Int Idx, typename RtnType>
struct FnDispatcher<TypeList<>, AccumIdx, Idx, RtnType>;


struct Fn0 {};

struct Fn1 {
	void stream(Int i, Int j) {
		cout<<"Fn1: "<<i<<" "<<j<<endl;
	}
};

struct Fn2 {
	template <Int Idx>
	Int stream(Int i, Int j) {
		cout<<"Fn2: "<<Idx<<" "<<i<<" "<<j<<endl;
		return 10;
	}
};

struct Fn3 {
	template <Int Idx>
	double stream(Int i, Int j) {
		cout<<"Fn3: "<<i<<" "<<i<<" "<<j<<endl;
		return 100;
	}

	template <Int AccumIdx, Int Idx>
	bool stream(Int i, Int j) {
		cout<<"Fn3: "<<AccumIdx<<" "<<Idx<<" "<<i<<" "<<j<<endl;
		return true;
	}
};

template <Int AccumIdx, Int Idx, typename Fn, typename... Args>
auto dispatchFn(Fn&& fn, Args&&... args)
-> FnRtnType<Fn, AccumIdx, Idx, Args...>
{
	using List = FnList<Fn, AccumIdx, Idx, Args...>;
	using RtnType = FnRtnType<Fn, AccumIdx, Idx, Args...>;

	return FnDispatcher<List, AccumIdx, Idx, RtnType>::dispatch(std::forward<Fn>(fn), std::forward<Args>(args)...);
}


int main(void)
{
	ListPrinter<TL<decltype(dispatchFn<0,0>(Fn3(), 1, 2))>>::print(cout);

	cout<<"DispatchFn: "<<dispatchFn<0,0>(Fn3(), 1,2)<<endl;

    return 0;
}

