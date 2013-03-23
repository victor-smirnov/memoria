
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <functional>

//#include <memoria/core/types/types.hpp>
//#include <memoria/core/types/list/typelist.hpp>

using namespace std;
//using namespace memoria;

struct Boo1 {
	template <typename T>
	int call(T arg, int arg1, int arg2) const
	{
		cout<<arg1<<" "<<arg2<<" "<<typeid(arg).name()<<endl;
		return 1;
	}
};

#define FN_WRAPPER(WrapperName, TargetClass, TargetMethod) 		\
struct WrapperName {											\
	const TargetClass& traget_class_;							\
	WrapperName(const TargetClass& v): traget_class_(v) {}		\
	template <typename T, typename... Args>						\
	void operator()(T* arg, Args... args)						\
	{															\
		traget_class_.TargetMethod(arg, args...);				\
	}															\
}


#define FN_WRAPPER_RTN(WrapperName, TargetClass, TargetMethod, ReturnType_) \
struct WrapperName {											\
	const TargetClass& traget_class_;							\
	typedef ReturnType_ ReturnType;								\
	WrapperName(const TargetClass& v): traget_class_(v) {}		\
	template <typename T, typename... Args>						\
	ReturnType operator()(T* arg, Args... args)					\
	{															\
		return traget_class_.TargetMethod(arg, args...);		\
	}															\
}

struct Base {
	int code_;

	Base(int code): code_(code) {}

	int code() const {return code_;}
};


struct C1: Base {
	C1():Base(1) {}
};

struct C2: Base {
	C2():Base(2) {}
};


template <typename T> struct TypeCode;

template <>
struct TypeCode<C1> {
	static const int value = 1;
};

template <>
struct TypeCode<C2> {
	static const int value = 2;
};

template <typename... Types>
struct Dispatcher {

	template <typename Fn, typename... Args>
	static void dispatch(Base* base, Fn&& fn, Args... args){
		cout<<"NOPE!"<<endl;
	}

	template <typename Fn, typename... Args>
	static typename Fn::ReturnType dispatchRtn(Base* base, Fn&& fn, Args... args)
	{
		cout<<"NOPE!"<<endl;
		return 0;
	}
};


template <typename Head, typename... Tail>
struct Dispatcher<Head, Tail...> {
	template <typename Fn, typename... Args>
	static void dispatch(Base* base, Fn&& fn, Args... args)
	{
		if (base->code() == TypeCode<Head>::value)
		{
			fn(static_cast<Head*>(base), args...);
		}
		else {
			Dispatcher<Tail...>::dispatch(base, fn, args...);
		}
	}

	template <typename Fn, typename... Args>
	static typename Fn::ReturnType dispatchRtn(Base* base, Fn&& fn, Args... args)
	{
		if (base->code() == TypeCode<Head>::value)
		{
			return fn(static_cast<Head*>(base), args...);
		}
		else {
			return Dispatcher<Tail...>::dispatchRtn(base, std::move(fn), args...);
		}
	}
};




FN_WRAPPER_RTN(Boo1Fn, Boo1, call, int);

int main(void)
{
	C1 c1;
	C2 c2;

	typedef Dispatcher<C1, C2> CDisp;

	CDisp::dispatchRtn(&c1, Boo1Fn(Boo1()), 1, 2);
	CDisp::dispatchRtn(&c2, Boo1Fn(Boo1()), 3, 4);

}
