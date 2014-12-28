
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

#include <memoria/core/packed/tools/packed_dispatcher_detail.hpp>

#include <utility>

namespace memoria {

template <typename Struct, Int Index> struct StreamDescr;

namespace pd {

template <typename List, Int GroupIdx, Int StreamIdx, typename Fn, typename... Args> class MakeRtnTypeList;
template <typename List, Int GroupIdx, Int StreamIdx, typename Fn, typename... Args> class ContainsVoidRtnType;

template <Int GroupIdx, Int StreamIdx, typename Head, Int Index, typename Fn, typename... Tail, typename... Args>
class MakeRtnTypeList<TypeList<StreamDescr<Head, Index>, Tail...>, GroupIdx, StreamIdx, Fn, Args...> {

    using FnType = typename std::remove_reference<Fn>::type;

    using RtnType = memoria::detail::pd::FnRtnType<Fn, GroupIdx, Index, StreamIdx, Head*, Args...>;

public:
    using Type = MergeLists<
            RtnType,
            typename MakeRtnTypeList<TypeList<Tail...>, GroupIdx, StreamIdx + 1, Fn, Args...>::Type
    >;
};

template <Int GroupIdx, Int StreamIdx, typename Fn, typename... Args>
class MakeRtnTypeList<TypeList<>, GroupIdx, StreamIdx, Fn, Args...> {
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


template <Int GroupIdx, Int StreamIdx, typename Head, Int Index, typename Fn, typename... Tail, typename... Args>
class ContainsVoidRtnType<TypeList<StreamDescr<Head, Index>, Tail...>, GroupIdx, StreamIdx, Fn, Args...> {

    using FnType = typename std::remove_reference<Fn>::type;

    using RtnType = memoria::detail::pd::FnRtnType<Fn, GroupIdx, Index, StreamIdx, Head*, Args...>;

public:
    static const bool Value = IsVoid<RtnType>::Value ||
                                ContainsVoidRtnType<TypeList<Tail...>, GroupIdx, StreamIdx + 1, Fn, Args...>::Value;
};

template <Int GroupIdx, Int StreamIdx, typename Fn, typename... Args>
class ContainsVoidRtnType<TypeList<>, GroupIdx, StreamIdx, Fn, Args...> {
public:
    static const bool Value = false;
};






template <typename List, Int GroupIdx, Int StreamIdx, typename Fn, typename... Args> class MakeRtnTypeListConst;
template <typename List, Int GroupIdx, Int StreamIdx, typename Fn, typename... Args> class ContainsVoidRtnTypeConst;

template <Int GroupIdx, Int StreamIdx, typename Head, Int Index, typename Fn, typename... Tail, typename... Args>
class MakeRtnTypeListConst<TypeList<StreamDescr<Head, Index>, Tail...>, GroupIdx, StreamIdx, Fn, Args...> {

    using FnType = typename std::remove_reference<Fn>::type;

    using RtnType = memoria::detail::pd::FnRtnType<Fn, GroupIdx, Index, StreamIdx, const Head*, Args...>;

public:

    using Type = MergeLists<
            RtnType,
            typename MakeRtnTypeListConst<TypeList<Tail...>, GroupIdx, StreamIdx + 1, Fn, Args...>::Type
    >;
};

template <Int GroupIdx, Int StreamIdx, typename Fn, typename... Args>
class MakeRtnTypeListConst<TypeList<>, GroupIdx, StreamIdx, Fn, Args...> {
public:
    using Type = TypeList<>;
};



template <Int GroupIdx, Int StreamIdx, typename Head, Int Index, typename Fn, typename... Tail, typename... Args>
class ContainsVoidRtnTypeConst<TypeList<StreamDescr<Head, Index>, Tail...>, GroupIdx, StreamIdx, Fn, Args...> {

    using FnType = typename std::remove_reference<Fn>::type;

    using RtnType = memoria::detail::pd::FnRtnType<Fn, GroupIdx, Index, StreamIdx, const Head*, Args...>;
public:

    static const bool Value = IsVoid<RtnType>::Value ||
                                ContainsVoidRtnTypeConst<TypeList<Tail...>, GroupIdx, StreamIdx + 1, Fn, Args...>::Value;
};

template <Int GroupIdx, Int StreamIdx, typename Fn, typename... Args>
class ContainsVoidRtnTypeConst<TypeList<>, GroupIdx, StreamIdx, Fn, Args...> {
public:
    static const bool Value = false;
};

}
}

#endif
