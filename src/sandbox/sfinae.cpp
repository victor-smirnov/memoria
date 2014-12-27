


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
using Fn0Type = auto(Args...) -> decltype(std::declval<T>().stream(std::declval<Args>()...));

template <typename T, Int Idx, typename... Args>
using Fn1Type = auto(Args...) -> decltype(std::declval<T>().template stream<Idx>(std::declval<Args>()...));

template <typename T, Int AccumIdx, Int Idx, typename... Args>
using Fn2Type = auto(Args...) -> decltype(std::declval<T>().template stream<AccumIdx, Idx>(std::declval<Args>()...));



template <typename T, typename... Args>
using Rtn0Type = typename FnTraits<Fn0Type<typename std::remove_reference<T>::type, Args...>>::Exists;

template <typename T, Int Idx, typename... Args>
using Rtn1Type = typename FnTraits<Fn1Type<typename std::remove_reference<T>::type, Idx, Args...>>::Exists;

template <typename T, Int AccumIdx, Int Idx, typename... Args>
using Rtn2Type = typename FnTraits<Fn2Type<typename std::remove_reference<T>::type, AccumIdx, Idx, Args...>>::Exists;



template <typename Fn, Int AccumIdx, Int Idx, typename ArgsList, typename T = EmptyType>
struct HasFn0H {
	static const Int Value = 0;
};

template <typename Fn, Int AccumIdx, Int Idx, typename... Args>
struct HasFn0H<Fn, AccumIdx, Idx, TypeList<Args...>, Rtn0Type<Fn, Args...>> {
	static const Int Value = 1;
};

template <typename Fn, Int AccumIdx, Int Idx, typename... Args>
using HasFn0 = HasFn0H<Fn, AccumIdx, Idx, TypeList<Args...>>;


template <typename Fn, Int AccumIdx, Int Idx, typename ArgsList, typename T = EmptyType>
struct HasFn1H {
	static const Int Value = 0;
};

template <typename Fn, Int AccumIdx, Int Idx, typename... Args>
struct HasFn1H<Fn, AccumIdx, Idx, TypeList<Args...>, Rtn1Type<Fn, Idx, Args...>> {
	static const Int Value = 1;
};

template <typename Fn, Int AccumIdx, Int Idx, typename... Args>
using HasFn1 = HasFn1H<Fn, AccumIdx, Idx, TypeList<Args...>>;



template <typename Fn, Int AccumIdx, Int Idx, typename ArgsList, typename T = EmptyType>
struct HasFn2H {
	static const Int Value = 0;
};

template <typename Fn, Int AccumIdx, Int Idx, typename... Args>
struct HasFn2H<Fn, AccumIdx, Idx, TypeList<Args...>, Rtn2Type<Fn, AccumIdx, Idx, Args...>> {
	static const Int Value = 1;
};

template <typename Fn, Int AccumIdx, Int Idx, typename... Args>
using HasFn2 = HasFn2H<Fn, AccumIdx, Idx, TypeList<Args...>>;


template <typename Fn, int AccumIdx, Int Idx, typename... Args>
using FnList = TypeList<
		IntValue<HasFn2<Fn, AccumIdx, Idx, Args...>::Value>,
		IntValue<HasFn1<Fn, AccumIdx, Idx, Args...>::Value>,
		IntValue<HasFn0<Fn, AccumIdx, Idx, Args...>::Value>
>;


struct Fn0 {
	void stream(Int i, Int j) {
		cout<<"Fn0: "<<i<<" "<<j<<endl;
	}
};

struct Fn1 {
	template <Int Idx>
	Int stream(Int i, Int j) {
		cout<<"Fn1: "<<Idx<<" "<<i<<" "<<j<<endl;
		return 0;
	}
};

struct Fn2 {
//	template <Int Idx>
	double stream(Int i, Int j) {
		cout<<"Fn2: "<<i<<" "<<i<<" "<<j<<endl;
		return 0;
	}

	template <Int AccumIdx, Int Idx>
	bool stream(Int i, Int j) {
		cout<<"Fn2: "<<AccumIdx<<" "<<Idx<<" "<<i<<" "<<j<<endl;
		return false;
	}
};


template <typename T, typename V = Int> struct PropValue;

template <typename T>
struct PropValue<T, decltype((void)T::VALUE, 0)> {
    static const Int Value = T::VALUE;
};

template <typename T>
struct PropValue<T, decltype((void)T::VALUE1, 0)> {
    static const Int Value = T::VALUE1;
};



struct VV {
    static const Int VALUE = 42;
};

struct VVV {
    static const Int VALUE1 = 42;
};


int main(void)
{
	ListPrinter<FnList<Fn2, 0, 0, Int, Int>>::print(cout);
    return 0;
}

