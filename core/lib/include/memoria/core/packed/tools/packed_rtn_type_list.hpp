
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

#include <memoria/core/packed/tools/packed_dispatcher_detail.hpp>

#include <utility>

namespace memoria {

template <typename Struct, size_t Index> struct SubstreamDescr;

namespace pd {

template <typename List, size_t GroupIdx, size_t StreamIdx, typename Fn, typename... Args> class MakeRtnTypeList;
template <typename List, size_t GroupIdx, size_t StreamIdx, typename Fn, typename... Args> class ContainsVoidRtnType;

template <size_t GroupIdx, size_t StreamIdx, typename Head, size_t Index, typename Fn, typename... Tail, typename... Args>
class MakeRtnTypeList<TypeList<SubstreamDescr<Head, Index>, Tail...>, GroupIdx, StreamIdx, Fn, Args...> {

    using FnType = typename std::remove_reference<Fn>::type;

    using RtnType = details::pd::FnRtnType<Fn, GroupIdx, Index, StreamIdx, Head*, Args...>;

public:
    using Type = MergeLists<
            RtnType,
            typename MakeRtnTypeList<TypeList<Tail...>, GroupIdx, StreamIdx + 1, Fn, Args...>::Type
    >;
};

template <size_t GroupIdx, size_t StreamIdx, typename Fn, typename... Args>
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


template <size_t GroupIdx, size_t StreamIdx, typename Head, size_t Index, typename Fn, typename... Tail, typename... Args>
class ContainsVoidRtnType<TypeList<SubstreamDescr<Head, Index>, Tail...>, GroupIdx, StreamIdx, Fn, Args...> {

    using FnType = typename std::remove_reference<Fn>::type;

    using RtnType = details::pd::FnRtnType<Fn, GroupIdx, Index, StreamIdx, Head*, Args...>;

public:
    static const bool Value = IsVoid<RtnType>::Value ||
                                ContainsVoidRtnType<TypeList<Tail...>, GroupIdx, StreamIdx + 1, Fn, Args...>::Value;
};

template <size_t GroupIdx, size_t StreamIdx, typename Fn, typename... Args>
class ContainsVoidRtnType<TypeList<>, GroupIdx, StreamIdx, Fn, Args...> {
public:
    static const bool Value = false;
};






template <typename List, size_t GroupIdx, size_t StreamIdx, typename Fn, typename... Args> class MakeRtnTypeListConst;
template <typename List, size_t GroupIdx, size_t StreamIdx, typename Fn, typename... Args> class ContainsVoidRtnTypeConst;

template <size_t GroupIdx, size_t StreamIdx, typename Head, size_t Index, typename Fn, typename... Tail, typename... Args>
class MakeRtnTypeListConst<TypeList<SubstreamDescr<Head, Index>, Tail...>, GroupIdx, StreamIdx, Fn, Args...> {

    using FnType = typename std::remove_reference<Fn>::type;

    using RtnType = details::pd::FnRtnType<Fn, GroupIdx, Index, StreamIdx, const Head*, Args...>;

public:

    using Type = MergeLists<
            RtnType,
            typename MakeRtnTypeListConst<TypeList<Tail...>, GroupIdx, StreamIdx + 1, Fn, Args...>::Type
    >;
};

template <size_t GroupIdx, size_t StreamIdx, typename Fn, typename... Args>
class MakeRtnTypeListConst<TypeList<>, GroupIdx, StreamIdx, Fn, Args...> {
public:
    using Type = TypeList<>;
};



template <size_t GroupIdx, size_t StreamIdx, typename Head, size_t Index, typename Fn, typename... Tail, typename... Args>
class ContainsVoidRtnTypeConst<TypeList<SubstreamDescr<Head, Index>, Tail...>, GroupIdx, StreamIdx, Fn, Args...> {

    using FnType = typename std::remove_reference<Fn>::type;

    using RtnType = details::pd::FnRtnType<Fn, GroupIdx, Index, StreamIdx, const Head*, Args...>;
public:

    static const bool Value = IsVoid<RtnType>::Value ||
                                ContainsVoidRtnTypeConst<TypeList<Tail...>, GroupIdx, StreamIdx + 1, Fn, Args...>::Value;
};

template <size_t GroupIdx, size_t StreamIdx, typename Fn, typename... Args>
class ContainsVoidRtnTypeConst<TypeList<>, GroupIdx, StreamIdx, Fn, Args...> {
public:
    static const bool Value = false;
};








}}
