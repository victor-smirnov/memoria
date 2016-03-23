
// Copyright Victor Smirnov 2014-2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/core/types/types.hpp>
#include <memoria/v1/core/types/type2type.hpp>
#include <memoria/v1/core/types/list/sublist.hpp>

#include <memoria/v1/core/types/fn_traits.hpp>
#include <memoria/v1/core/types/list/tuple.hpp>
#include <memoria/v1/core/types/list/misc.hpp>

#include <memoria/v1/core/packed/tools/packed_rtn_type_list.hpp>


namespace memoria {
namespace detail    {
namespace pd        {

class TNotDefined;

template <typename T, typename... Args>
using Fn1Type = auto(Args...) -> decltype(std::declval<T>().stream(std::declval<Args>()...));

template <typename T, Int Idx, typename... Args>
using Fn2Type = auto(Args...) -> decltype(std::declval<T>().template stream<Idx>(std::declval<Args>()...));

template <typename T, Int AllocIdx, Int Idx, typename... Args>
using Fn3Type = auto(Args...) -> decltype(std::declval<T>().template stream<AllocIdx, Idx>(std::declval<Args>()...));

template <typename T, Int GroupIdx, Int AllocIdx, Int Idx, typename... Args>
using Fn4Type = auto(Args...) -> decltype(std::declval<T>().template stream<GroupIdx, AllocIdx, Idx>(std::declval<Args>()...));


template <typename T, typename... Args>
using Rtn1Type = typename FnTraits<Fn1Type<typename std::remove_reference<T>::type, Args...>>::RtnType;

template <typename T, Int Idx, typename... Args>
using Rtn2Type = typename FnTraits<Fn2Type<typename std::remove_reference<T>::type, Idx, Args...>>::RtnType;

template <typename T, Int AllocIdx, Int Idx, typename... Args>
using Rtn3Type = typename FnTraits<Fn3Type<typename std::remove_reference<T>::type, AllocIdx, Idx, Args...>>::RtnType;

template <typename T, Int GroupIdx, Int AllocIdx, Int Idx, typename... Args>
using Rtn4Type = typename FnTraits<Fn4Type<typename std::remove_reference<T>::type, GroupIdx, AllocIdx, Idx, Args...>>::RtnType;


template <typename T, typename... Args>
using Ex1Type = typename FnTraits<Fn1Type<typename std::remove_reference<T>::type, Args...>>::Exists;

template <typename T, Int Idx, typename... Args>
using Ex2Type = typename FnTraits<Fn2Type<typename std::remove_reference<T>::type, Idx, Args...>>::Exists;

template <typename T, Int AllocIdx, Int Idx, typename... Args>
using Ex3Type = typename FnTraits<Fn3Type<typename std::remove_reference<T>::type, AllocIdx, Idx, Args...>>::Exists;

template <typename T, Int GroupIdx, Int AllocIdx, Int Idx, typename... Args>
using Ex4Type = typename FnTraits<Fn4Type<typename std::remove_reference<T>::type, GroupIdx, AllocIdx, Idx, Args...>>::Exists;




template <typename Fn, Int GroupIdx, Int AllocIdx, Int Idx, typename ArgsList, typename T = EmptyType>
struct HasFn1H {
    static const Int Value = 0;
    using RtnType = TNotDefined;
};

template <typename Fn, Int GroupIdx, Int AllocIdx, Int Idx, typename... Args>
struct HasFn1H<Fn, GroupIdx, AllocIdx, Idx, TypeList<Args...>, Ex1Type<Fn, Args...>> {
    static const Int Value = 1;
    using RtnType = Rtn1Type<Fn, Args...>;
};

template <typename Fn, Int GroupIdx, Int AllocIdx, Int Idx, typename... Args>
using HasFn1 = HasFn1H<Fn, GroupIdx, AllocIdx, Idx, TypeList<Args...>>;


template <typename Fn, Int GroupIdx, Int AllocIdx, Int Idx, typename ArgsList, typename T = EmptyType>
struct HasFn2H {
    static const Int Value = 0;
    using RtnType = TNotDefined;
};

template <typename Fn, Int GroupIdx, Int AllocIdx, Int Idx, typename... Args>
struct HasFn2H<Fn, GroupIdx, AllocIdx, Idx, TypeList<Args...>, Ex2Type<Fn, Idx, Args...>> {
    static const Int Value = 2;
    using RtnType = Rtn2Type<Fn, Idx, Args...>;
};

template <typename Fn, Int GroupIdx, Int AllocIdx, Int Idx, typename... Args>
using HasFn2 = HasFn2H<Fn, GroupIdx, AllocIdx, Idx, TypeList<Args...>>;



template <typename Fn, Int GroupIdx, Int AllocIdx, Int Idx, typename ArgsList, typename T = EmptyType>
struct HasFn3H {
    static const Int Value = 0;
    using RtnType = TNotDefined;
};

template <typename Fn, Int GroupIdx, Int AllocIdx, Int Idx, typename... Args>
struct HasFn3H<Fn, GroupIdx, AllocIdx, Idx, TypeList<Args...>, Ex3Type<Fn, AllocIdx, Idx, Args...>> {
    static const Int Value = 3;
    using RtnType = Rtn3Type<Fn, AllocIdx, Idx, Args...>;
};

template <typename Fn, Int GroupIdx, Int AllocIdx, Int Idx, typename... Args>
using HasFn3 = HasFn3H<Fn, GroupIdx, AllocIdx, Idx, TypeList<Args...>>;



template <typename Fn, Int GroupIdx, Int AllocIdx, Int Idx, typename ArgsList, typename T = EmptyType>
struct HasFn4H {
    static const Int Value = 0;
    using RtnType = TNotDefined;
};

template <typename Fn, Int GroupIdx, Int AllocIdx, Int Idx, typename... Args>
struct HasFn4H<Fn, GroupIdx, AllocIdx, Idx, TypeList<Args...>, Ex4Type<Fn, GroupIdx, AllocIdx, Idx, Args...>> {
    static const Int Value = 4;
    using RtnType = Rtn4Type<Fn, GroupIdx, AllocIdx, Idx, Args...>;
};

template <typename Fn, Int GroupIdx, Int AllocIdx, Int Idx, typename... Args>
using HasFn4 = HasFn4H<Fn, GroupIdx, AllocIdx, Idx, TypeList<Args...>>;




template <typename Fn, Int GroupIdx, Int AllocIdx, Int Idx, typename... Args>
using FnList = TypeList<
        IntValue<HasFn4<Fn, GroupIdx, AllocIdx, Idx, Args...>::Value>,
        IntValue<HasFn3<Fn, GroupIdx, AllocIdx, Idx, Args...>::Value>,
        IntValue<HasFn2<Fn, GroupIdx, AllocIdx, Idx, Args...>::Value>,
        IntValue<HasFn1<Fn, GroupIdx, AllocIdx, Idx, Args...>::Value>
>;


template <typename Fn, Int GroupIdx, Int AllocIdx, Int Idx, typename... Args>
using FnRtnType = IfThenElse<
        (HasFn4<Fn, GroupIdx, AllocIdx, Idx, Args...>::Value > 0),
        typename HasFn4<Fn, GroupIdx, AllocIdx, Idx, Args...>::RtnType,
        IfThenElse<
            (HasFn3<Fn, GroupIdx, AllocIdx, Idx, Args...>::Value > 0),
            typename HasFn3<Fn, GroupIdx, AllocIdx, Idx, Args...>::RtnType,
            IfThenElse<
                (HasFn2<Fn, GroupIdx, AllocIdx, Idx, Args...>::Value > 0),
                typename HasFn2<Fn, GroupIdx, AllocIdx, Idx, Args...>::RtnType,
                typename HasFn1<Fn, GroupIdx, AllocIdx, Idx, Args...>::RtnType
            >
        >
>;



template <typename List, Int GroupIdx, Int AllocIdx, Int Idx, typename RtnType>
struct FnDispatcher;


template <typename... Tail, Int GroupIdx, Int AllocIdx, Int Idx, typename RtnType>
struct FnDispatcher<TypeList<IntValue<0>, Tail...>, GroupIdx, AllocIdx, Idx, RtnType> {
    template <typename Fn, typename... Args>
    static RtnType dispatch(Fn&& fn, Args&&... args)
    {
        return FnDispatcher<TypeList<Tail...>, GroupIdx, AllocIdx, Idx, RtnType>::dispatch(std::forward<Fn>(fn), std::forward<Args>(args)...);
    };
};

template <typename... Tail, Int GroupIdx, Int AllocIdx, Int Idx, typename RtnType>
struct FnDispatcher<TypeList<IntValue<1>, Tail...>, GroupIdx, AllocIdx, Idx, RtnType> {
    template <typename Fn, typename... Args>
    static RtnType dispatch(Fn&& fn, Args&&... args)
    {
        return fn.stream(std::forward<Args>(args)...);
    };
};

template <typename... Tail, Int GroupIdx, Int AllocIdx, Int Idx, typename RtnType>
struct FnDispatcher<TypeList<IntValue<2>, Tail...>, GroupIdx, AllocIdx, Idx, RtnType> {
    template <typename Fn, typename... Args>
    static RtnType dispatch(Fn&& fn, Args&&... args)
    {
        return fn.template stream<Idx>(std::forward<Args>(args)...);
    };
};


template <typename... Tail, Int GroupIdx, Int AllocIdx, Int Idx, typename RtnType>
struct FnDispatcher<TypeList<IntValue<3>, Tail...>, GroupIdx, AllocIdx, Idx, RtnType> {
    template <typename Fn, typename... Args>
    static RtnType dispatch(Fn&& fn, Args&&... args)
    {
        return fn.template stream<AllocIdx, Idx>(std::forward<Args>(args)...);
    };
};


template <typename... Tail, Int GroupIdx, Int AllocIdx, Int Idx, typename RtnType>
struct FnDispatcher<TypeList<IntValue<4>, Tail...>, GroupIdx, AllocIdx, Idx, RtnType> {
    template <typename Fn, typename... Args>
    static RtnType dispatch(Fn&& fn, Args&&... args)
    {
        return fn.template stream<GroupIdx, AllocIdx, Idx>(std::forward<Args>(args)...);
    };
};



template <Int GroupIdx, Int AllocIdx, Int Idx, typename RtnType>
struct FnDispatcher<TypeList<>, GroupIdx, AllocIdx, Idx, RtnType>;




template <Int GroupIdx, Int AllocIdx, Int Idx, typename Fn, typename... Args>
auto dispatchFn(Fn&& fn, Args&&... args)
-> FnRtnType<Fn, GroupIdx, AllocIdx, Idx, Args...>
{
    using List = FnList<Fn, GroupIdx, AllocIdx, Idx, Args...>;
    using RtnType = FnRtnType<Fn, GroupIdx, AllocIdx, Idx, Args...>;

    return FnDispatcher<List, GroupIdx, AllocIdx, Idx, RtnType>::dispatch(std::forward<Fn>(fn), std::forward<Args>(args)...);
}


}
}
}
