
// Copyright 2014 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#pragma once

#include <memoria/core/types.hpp>

#include <memoria/core/types/list/sublist.hpp>

#include <memoria/core/types/fn_traits.hpp>
#include <memoria/core/types/list/tuple.hpp>
#include <memoria/core/types/list/misc.hpp>
#include <memoria/core/types/algo/select.hpp>

#include <memoria/core/packed/tools/packed_rtn_type_list.hpp>


namespace memoria {
namespace details {
namespace pd {

class TNotDefined;

template <typename T, typename... Args>
using Fn1Type = auto(Args&&...) -> decltype(std::declval<T>().stream(std::declval<Args>()...));

template <typename T, size_t Idx, typename... Args>
using Fn2Type = auto(Args&&...) -> decltype(std::declval<T>().template stream<Idx>(std::declval<Args>()...));

template <typename T, size_t AllocIdx, size_t Idx, typename... Args>
using Fn3Type = auto(Args&&...) -> decltype(std::declval<T>().template stream<AllocIdx, Idx>(std::declval<Args>()...));

template <typename T, size_t GroupIdx, size_t AllocIdx, size_t Idx, typename... Args>
using Fn4Type = auto(Args&&...) -> decltype(std::declval<T>().template stream<GroupIdx, AllocIdx, Idx>(std::declval<Args>()...));


template <typename T, typename... Args>
using Rtn1Type = typename FnTraits<Fn1Type<typename std::remove_reference<T>::type, Args...>>::RtnType;

template <typename T, size_t Idx, typename... Args>
using Rtn2Type = typename FnTraits<Fn2Type<typename std::remove_reference<T>::type, Idx, Args...>>::RtnType;

template <typename T, size_t AllocIdx, size_t Idx, typename... Args>
using Rtn3Type = typename FnTraits<Fn3Type<typename std::remove_reference<T>::type, AllocIdx, Idx, Args...>>::RtnType;

template <typename T, size_t GroupIdx, size_t AllocIdx, size_t Idx, typename... Args>
using Rtn4Type = typename FnTraits<Fn4Type<typename std::remove_reference<T>::type, GroupIdx, AllocIdx, Idx, Args...>>::RtnType;

/*
template <typename T, typename... Args>
using Ex1Type = typename FnTraits<Fn1Type<typename std::remove_reference<T>::type, Args...>>::Exists;

template <typename T, size_t Idx, typename... Args>
using Ex2Type = typename FnTraits<Fn2Type<typename std::remove_reference<T>::type, Idx, Args...>>::Exists;

template <typename T, size_t AllocIdx, size_t Idx, typename... Args>
using Ex3Type = typename FnTraits<Fn3Type<typename std::remove_reference<T>::type, AllocIdx, Idx, Args...>>::Exists;

template <typename T, size_t GroupIdx, size_t AllocIdx, size_t Idx, typename... Args>
using Ex4Type = typename FnTraits<Fn4Type<typename std::remove_reference<T>::type, GroupIdx, AllocIdx, Idx, Args...>>::Exists;
*/



template <typename Fn, size_t GroupIdx, size_t AllocIdx, size_t Idx, typename ArgsList, typename T = void>
struct HasFn1H {
    static const size_t Value = 0;
    using RtnType = TNotDefined;
};

template <typename Fn, size_t GroupIdx, size_t AllocIdx, size_t Idx, typename... Args> //VoidT<decltype(std::declval<Rtn1Type<Fn, Args...>)>
struct HasFn1H<Fn, GroupIdx, AllocIdx, Idx, TypeList<Args...>, VoidT<decltype(std::declval<Rtn1Type<Fn, Args...>>())>> { //Ex1Type<Fn, Args...>
    static const size_t Value = 1;
    using RtnType = Rtn1Type<Fn, Args...>;
};

template <typename Fn, size_t GroupIdx, size_t AllocIdx, size_t Idx, typename... Args>
using HasFn1 = HasFn1H<Fn, GroupIdx, AllocIdx, Idx, TypeList<Args...>>;


template <typename Fn, size_t GroupIdx, size_t AllocIdx, size_t Idx, typename ArgsList, typename T = void>
struct HasFn2H {
    static const size_t Value = 0;
    using RtnType = TNotDefined;
};

template <typename Fn, size_t GroupIdx, size_t AllocIdx, size_t Idx, typename... Args> //
struct HasFn2H<Fn, GroupIdx, AllocIdx, Idx, TypeList<Args...>, VoidT<decltype(std::declval<Rtn2Type<Fn, Idx, Args...>>())>> {//Ex2Type<Fn, Idx, Args...>
    static const size_t Value = 2;
    using RtnType = Rtn2Type<Fn, Idx, Args...>;
};

template <typename Fn, size_t GroupIdx, size_t AllocIdx, size_t Idx, typename... Args>
using HasFn2 = HasFn2H<Fn, GroupIdx, AllocIdx, Idx, TypeList<Args...>>;



template <typename Fn, size_t GroupIdx, size_t AllocIdx, size_t Idx, typename ArgsList, typename T = void>
struct HasFn3H {
    static const size_t Value = 0;
    using RtnType = TNotDefined;
};

template <typename Fn, size_t GroupIdx, size_t AllocIdx, size_t Idx, typename... Args> //VoidT<decltype(std::declval<Rtn1Type<Fn, Args...>)>
struct HasFn3H<Fn, GroupIdx, AllocIdx, Idx, TypeList<Args...>, VoidT<decltype(std::declval<Rtn3Type<Fn, AllocIdx, Idx, Args...>>())>> {//Ex3Type<Fn, AllocIdx, Idx, Args...>
    static const size_t Value = 3;
    using RtnType = Rtn3Type<Fn, AllocIdx, Idx, Args...>;
};

template <typename Fn, size_t GroupIdx, size_t AllocIdx, size_t Idx, typename... Args>
using HasFn3 = HasFn3H<Fn, GroupIdx, AllocIdx, Idx, TypeList<Args...>>;



template <typename Fn, size_t GroupIdx, size_t AllocIdx, size_t Idx, typename ArgsList, typename T = void>
struct HasFn4H {
    static const size_t Value = 0;
    using RtnType = TNotDefined;
};

template <typename Fn, size_t GroupIdx, size_t AllocIdx, size_t Idx, typename... Args> //VoidT<decltype(std::declval<Rtn1Type<Fn, Args...>)>
struct HasFn4H<Fn, GroupIdx, AllocIdx, Idx, TypeList<Args...>, VoidT<decltype(std::declval<Rtn4Type<Fn, GroupIdx, AllocIdx, Idx, Args...>>())>> {//Ex4Type<Fn, GroupIdx, AllocIdx, Idx, Args...>
    static const size_t Value = 4;
    using RtnType = Rtn4Type<Fn, GroupIdx, AllocIdx, Idx, Args...>;
};

template <typename Fn, size_t GroupIdx, size_t AllocIdx, size_t Idx, typename... Args>
using HasFn4 = HasFn4H<Fn, GroupIdx, AllocIdx, Idx, TypeList<Args...>>;




template <typename Fn, size_t GroupIdx, size_t AllocIdx, size_t Idx, typename... Args>
using FnList = TypeList<
        IntValue<HasFn4<Fn, GroupIdx, AllocIdx, Idx, Args...>::Value>,
        IntValue<HasFn3<Fn, GroupIdx, AllocIdx, Idx, Args...>::Value>,
        IntValue<HasFn2<Fn, GroupIdx, AllocIdx, Idx, Args...>::Value>,
        IntValue<HasFn1<Fn, GroupIdx, AllocIdx, Idx, Args...>::Value>
>;


template <typename Fn, size_t GroupIdx, size_t AllocIdx, size_t Idx, typename... Args>
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



template <typename List, size_t GroupIdx, size_t AllocIdx, size_t Idx, typename RtnType>
struct FnDispatcher;


template <typename... Tail, size_t GroupIdx, size_t AllocIdx, size_t Idx, typename RtnType>
struct FnDispatcher<TypeList<IntValue<0>, Tail...>, GroupIdx, AllocIdx, Idx, RtnType> {
    template <typename Fn, typename... Args>
    static RtnType dispatch(Fn&& fn, Args&&... args)
    {
        return FnDispatcher<TypeList<Tail...>, GroupIdx, AllocIdx, Idx, RtnType>::dispatch(std::forward<Fn>(fn), std::forward<Args>(args)...);
    }
};

template <typename... Tail, size_t GroupIdx, size_t AllocIdx, size_t Idx, typename RtnType>
struct FnDispatcher<TypeList<IntValue<1>, Tail...>, GroupIdx, AllocIdx, Idx, RtnType> {
    template <typename Fn, typename... Args>
    static RtnType dispatch(Fn&& fn, Args&&... args)
    {
        return fn.stream(std::forward<Args>(args)...);
    }
};

template <typename... Tail, size_t GroupIdx, size_t AllocIdx, size_t Idx, typename RtnType>
struct FnDispatcher<TypeList<IntValue<2>, Tail...>, GroupIdx, AllocIdx, Idx, RtnType> {
    template <typename Fn, typename... Args>
    static RtnType dispatch(Fn&& fn, Args&&... args)
    {
        return fn.template stream<Idx>(std::forward<Args>(args)...);
    }
};


template <typename... Tail, size_t GroupIdx, size_t AllocIdx, size_t Idx, typename RtnType>
struct FnDispatcher<TypeList<IntValue<3>, Tail...>, GroupIdx, AllocIdx, Idx, RtnType> {
    template <typename Fn, typename... Args>
    static RtnType dispatch(Fn&& fn, Args&&... args)
    {
        return fn.template stream<AllocIdx, Idx>(std::forward<Args>(args)...);
    }
};


template <typename... Tail, size_t GroupIdx, size_t AllocIdx, size_t Idx, typename RtnType>
struct FnDispatcher<TypeList<IntValue<4>, Tail...>, GroupIdx, AllocIdx, Idx, RtnType> {
    template <typename Fn, typename... Args>
    static RtnType dispatch(Fn&& fn, Args&&... args)
    {
        return fn.template stream<GroupIdx, AllocIdx, Idx>(std::forward<Args>(args)...);
    }
};



template <size_t GroupIdx, size_t AllocIdx, size_t Idx, typename RtnType>
struct FnDispatcher<TypeList<>, GroupIdx, AllocIdx, Idx, RtnType>;




template <size_t GroupIdx, size_t AllocIdx, size_t Idx, typename Fn, typename... Args>
auto dispatchFn(Fn&& fn, Args&&... args)
-> FnRtnType<Fn, GroupIdx, AllocIdx, Idx, Args...>
{
    using List = FnList<Fn, GroupIdx, AllocIdx, Idx, Args...>;
    using RtnType = FnRtnType<Fn, GroupIdx, AllocIdx, Idx, Args...>;

    return FnDispatcher<List, GroupIdx, AllocIdx, Idx, RtnType>::dispatch(std::forward<Fn>(fn), std::forward<Args>(args)...);
}


}}}
