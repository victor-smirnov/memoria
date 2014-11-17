
// Copyright Victor Smirnov 2014.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CORE_TYPES_FN_TRAITS_HPP_
#define MEMORIA_CORE_TYPES_FN_TRAITS_HPP_

#include <memoria/core/types/types.hpp>

#include <tuple>

namespace memoria {

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

template <typename RtnType_, typename... Args_>
struct FnTraits<RtnType_ (*)(Args_...)> {
	typedef RtnType_ 				RtnType;
	typedef void 					ClassType;
	typedef TypeList<Args_...>		ArgsList;

	static const Int Arity			= sizeof...(Args_);
	static const bool Const			= true;

	template <Int I>
	using Arg = typename std::tuple_element<I, std::tuple<Args_...>>::type;
};

template <typename RtnType_, typename... Args_>
struct FnTraits<RtnType_ (Args_...)> {
	typedef RtnType_ 				RtnType;
	typedef void 					ClassType;
	typedef TypeList<Args_...>		ArgsList;

	static const Int Arity			= sizeof...(Args_);
	static const bool Const			= true;

	template <Int I>
	using Arg = typename std::tuple_element<I, std::tuple<Args_...>>::type;
};


}

#endif /* FN_TRAITS_HPP_ */
