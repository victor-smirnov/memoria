
// Copyright Victor Smirnov 2014.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_CORE_PACKED_RTN_TYPE_LIST_HPP_
#define MEMORIA_CORE_PACKED_RTN_TYPE_LIST_HPP_

#include <memoria/core/types/types.hpp>
#include <memoria/core/types/type2type.hpp>
#include <memoria/core/types/list/sublist.hpp>

#include <memoria/core/types/fn_traits.hpp>
#include <memoria/core/types/list/tuple.hpp>

#include <utility>

namespace memoria {

template <typename Struct, Int Index> struct StreamDescr;

template <typename List, Int StreamIdx, typename Fn, typename... Args> class MakeRtnTypeList;
template <typename List, Int StreamIdx, typename Fn, typename... Args> class ContainsVoidRtnType;

template <Int StreamIdx, typename Head, Int Index, typename Fn, typename... Tail, typename... Args>
class MakeRtnTypeList<TypeList<StreamDescr<Head, Index>, Tail...>, StreamIdx, Fn, Args...> {

	using FnType = typename std::remove_reference<Fn>::type;

	using RtnType = decltype(
		std::declval<FnType>().template stream<StreamIdx>(std::declval<Head*>(), std::declval<Args>()...)
	);

public:
	using Type = MergeLists<
			RtnType,
			typename MakeRtnTypeList<TypeList<Tail...>, StreamIdx + 1, Fn, Args...>::Type
	>;
};

template <Int StreamIdx, typename Fn, typename... Args>
class MakeRtnTypeList<TypeList<>, StreamIdx, Fn, Args...> {
public:
	using Type = TypeList<>;
};


template <typename T>
struct IsVoid {
	static const bool Value = false;
};

template <>
struct IsVoid<void> {
	static const bool Value = true;
};


template <Int StreamIdx, typename Head, Int Index, typename Fn, typename... Tail, typename... Args>
class ContainsVoidRtnType<TypeList<StreamDescr<Head, Index>, Tail...>, StreamIdx, Fn, Args...> {

	using FnType = typename std::remove_reference<Fn>::type;

	using RtnType = decltype(
		std::declval<FnType>().template stream<StreamIdx>(std::declval<Head*>(), std::declval<Args>()...)
	);

public:
	static const bool Value = IsVoid<RtnType>::Value ||
								ContainsVoidRtnType<TypeList<Tail...>, StreamIdx + 1, Fn, Args...>::Value;
};

template <Int StreamIdx, typename Fn, typename... Args>
class ContainsVoidRtnType<TypeList<>, StreamIdx, Fn, Args...> {
public:
	static const bool Value = false;
};






template <typename List, Int StreamIdx, typename Fn, typename... Args> class MakeRtnTypeListConst;
template <typename List, Int StreamIdx, typename Fn, typename... Args> class ContainsVoidRtnTypeConst;

template <Int StreamIdx, typename Head, Int Index, typename Fn, typename... Tail, typename... Args>
class MakeRtnTypeListConst<TypeList<StreamDescr<Head, Index>, Tail...>, StreamIdx, Fn, Args...> {

	using FnType = typename std::remove_reference<Fn>::type;

	using RtnType = decltype(
		std::declval<FnType>().template stream<StreamIdx>(std::declval<const Head*>(), std::declval<Args>()...)
	);

public:

	using Type = MergeLists<
			RtnType,
			typename MakeRtnTypeListConst<TypeList<Tail...>, StreamIdx + 1, Fn, Args...>::Type
	>;
};

template <Int StreamIdx, typename Fn, typename... Args>
class MakeRtnTypeListConst<TypeList<>, StreamIdx, Fn, Args...> {
public:
	using Type = TypeList<>;
};



template <Int StreamIdx, typename Head, Int Index, typename Fn, typename... Tail, typename... Args>
class ContainsVoidRtnTypeConst<TypeList<StreamDescr<Head, Index>, Tail...>, StreamIdx, Fn, Args...> {

	using FnType = typename std::remove_reference<Fn>::type;

	using RtnType = decltype(
		std::declval<FnType>().template stream<StreamIdx>(std::declval<const Head*>(), std::declval<Args>()...)
	);

public:
	static const bool Value = IsVoid<RtnType>::Value ||
								ContainsVoidRtnTypeConst<TypeList<Tail...>, StreamIdx + 1, Fn, Args...>::Value;
};

template <Int StreamIdx, typename Fn, typename... Args>
class ContainsVoidRtnTypeConst<TypeList<>, StreamIdx, Fn, Args...> {
public:
	static const bool Value = false;
};


}

#endif
