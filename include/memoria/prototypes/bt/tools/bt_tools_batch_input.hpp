
// Copyright 2019-2022 Victor Smirnov
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


#include <memoria/core/types/list/sublist.hpp>
#include <memoria/core/packed/misc/packed_sized_struct.hpp>
#include <memoria/core/datatypes/buffer/ssrle_buffer.hpp>


namespace memoria {
namespace bt {
namespace detail {


template <typename List> struct SelectNonSizedStruct;

template <typename Value_, int32_t Indexes, PkdSearchType SearchType, typename... Tail>
struct SelectNonSizedStruct<TL<PackedSizedStruct<Value_, Indexes, SearchType>, Tail...>> {
    using Type = typename SelectNonSizedStruct<TL<Tail...>>::Type;
};

template <typename T, typename... Tail>
struct SelectNonSizedStruct<TL<T, Tail...>> {
    using Type = MergeLists<T, typename SelectNonSizedStruct<TL<Tail...>>::Type>;
};

template <>
struct SelectNonSizedStruct<TL<>> {
    using Type = TL<>;
};


template <typename StructList> struct StreamsListParserTF;

template <typename Head, typename... Tail>
struct StreamsListParserTF<TL<Head, Tail...>> {
    using Type = MergeValueLists<
        IntList<ListSize<typename SelectNonSizedStruct<Linearize<Head>>::Type>>,
        typename StreamsListParserTF<TL<Tail...>>::Type
    >;
};

template <typename Head>
struct StreamsListParserTF<TL<Head>> {
    // This is symbols sequence stream. Do nothing here.
    using Type = IntList<>;
};

}


template <typename CtrT>
struct CtrBatchInputProviderBase {

    using TreeNodePtr = typename CtrT::Types::TreeNodePtr;
    using Position    = typename CtrT::Types::Position;

    virtual ~CtrBatchInputProviderBase() noexcept = default;

    virtual bool hasData() = 0;
    virtual Position fill(const TreeNodePtr& leaf, const Position& from) = 0;
};



}}
