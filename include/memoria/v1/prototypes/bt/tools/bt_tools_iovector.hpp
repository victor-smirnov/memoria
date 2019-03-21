
// Copyright 2019 Victor Smirnov
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

#include <memoria/v1/core/iovector/io_vector.hpp>
#include <memoria/v1/core/types/list/sublist.hpp>

#include <memoria/v1/core/packed/misc/packed_sized_struct.hpp>

namespace memoria {
namespace v1 {
namespace bt {
namespace _ {


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


template <int32_t Streams, typename PkdStructsList>
class IOVectorsTF
{
    using NonSizedPackedStructsList = typename SelectNonSizedStruct<PkdStructsList>::Type;

    static constexpr int32_t PkdStructsListSize = ListSize<NonSizedPackedStructsList>;

    using SequencePkdStruct = typename ListHead<
        typename SublistToEnd<
            NonSizedPackedStructsList,
            PkdStructsListSize - 1
        >::Type
    >::Type;

    using DataStreamsPkdStructsList = typename SublistFromStart<
        NonSizedPackedStructsList,
        PkdStructsListSize - 1
    >::Type;

    struct IOVTypes {
        using PackedStructsList = DataStreamsPkdStructsList;
        using SymbolSequence    = typename SequencePkdStruct::GrowableIOSubstream;
    };

    template <typename T>
    struct CreateIOVectorIVMapperTF {
        using Type = typename T::GrowableIOSubstream;
    };


public:
    using IOVectorT = io::StaticIOVector<IOVTypes, CreateIOVectorIVMapperTF>;
};


}}}}
