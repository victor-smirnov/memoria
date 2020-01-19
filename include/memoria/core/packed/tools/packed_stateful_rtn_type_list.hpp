
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
#include <memoria/core/types/type2type.hpp>
#include <memoria/core/types/list/sublist.hpp>

#include <memoria/core/types/fn_traits.hpp>
#include <memoria/core/types/list/tuple.hpp>

#include <memoria/core/packed/tools/packed_dispatcher_detail.hpp>

#include <utility>

namespace memoria {

template <typename Struct, int32_t Index> struct SubstreamDescr;

namespace pd_stateful {

template <typename List, int32_t GroupIdx, int32_t StreamIdx, typename Fn, typename... Args> class MakeRtnTypeList;
template <typename List, int32_t GroupIdx, int32_t StreamIdx, typename Fn, typename... Args> class ContainsVoidRtnType;

template <int32_t GroupIdx, int32_t StreamIdx, typename Head, int32_t Index, typename Fn, typename... Tail, typename... Args>
class MakeRtnTypeList<TypeList<SubstreamDescr<Head, Index>, Tail...>, GroupIdx, StreamIdx, Fn, Args...> {

    using FnType = typename std::remove_reference<Fn>::type;

    using RtnType = details::pd::FnRtnType<Fn, GroupIdx, Index, StreamIdx, typename Head::SparseObject&, Args...>;

public:
    using Type = MergeLists<
            RtnType,
            typename MakeRtnTypeList<TypeList<Tail...>, GroupIdx, StreamIdx + 1, Fn, Args...>::Type
    >;
};

template <int32_t GroupIdx, int32_t StreamIdx, typename Fn, typename... Args>
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


template <int32_t GroupIdx, int32_t StreamIdx, typename Head, int32_t Index, typename Fn, typename... Tail, typename... Args>
class ContainsVoidRtnType<TypeList<SubstreamDescr<Head, Index>, Tail...>, GroupIdx, StreamIdx, Fn, Args...> {

    using FnType = typename std::remove_reference<Fn>::type;

    using RtnType = details::pd::FnRtnType<Fn, GroupIdx, Index, StreamIdx, typename Head::SparseObject&, Args...>;

public:
    static const bool Value = IsVoid<RtnType>::Value ||
                                ContainsVoidRtnType<TypeList<Tail...>, GroupIdx, StreamIdx + 1, Fn, Args...>::Value;
};

template <int32_t GroupIdx, int32_t StreamIdx, typename Fn, typename... Args>
class ContainsVoidRtnType<TypeList<>, GroupIdx, StreamIdx, Fn, Args...> {
public:
    static const bool Value = false;
};






template <typename List, int32_t GroupIdx, int32_t StreamIdx, typename Fn, typename... Args> class MakeRtnTypeListConst;
template <typename List, int32_t GroupIdx, int32_t StreamIdx, typename Fn, typename... Args> class ContainsVoidRtnTypeConst;

template <int32_t GroupIdx, int32_t StreamIdx, typename Head, int32_t Index, typename Fn, typename... Tail, typename... Args>
class MakeRtnTypeListConst<TypeList<SubstreamDescr<Head, Index>, Tail...>, GroupIdx, StreamIdx, Fn, Args...> {

    using FnType = typename std::remove_reference<Fn>::type;

    using RtnType = details::pd::FnRtnType<Fn, GroupIdx, Index, StreamIdx, const typename Head::SparseObject&, Args...>;

public:

    using Type = MergeLists<
            RtnType,
            typename MakeRtnTypeListConst<TypeList<Tail...>, GroupIdx, StreamIdx + 1, Fn, Args...>::Type
    >;
};

template <int32_t GroupIdx, int32_t StreamIdx, typename Fn, typename... Args>
class MakeRtnTypeListConst<TypeList<>, GroupIdx, StreamIdx, Fn, Args...> {
public:
    using Type = TypeList<>;
};



template <int32_t GroupIdx, int32_t StreamIdx, typename Head, int32_t Index, typename Fn, typename... Tail, typename... Args>
class ContainsVoidRtnTypeConst<TypeList<SubstreamDescr<Head, Index>, Tail...>, GroupIdx, StreamIdx, Fn, Args...> {

    using FnType = typename std::remove_reference<Fn>::type;

    using RtnType = details::pd::FnRtnType<Fn, GroupIdx, Index, StreamIdx, const typename Head::SparseObject&, Args...>;
public:

    static const bool Value = IsVoid<RtnType>::Value ||
                                ContainsVoidRtnTypeConst<TypeList<Tail...>, GroupIdx, StreamIdx + 1, Fn, Args...>::Value;
};

template <int32_t GroupIdx, int32_t StreamIdx, typename Fn, typename... Args>
class ContainsVoidRtnTypeConst<TypeList<>, GroupIdx, StreamIdx, Fn, Args...> {
public:
    static const bool Value = false;
};


}}
