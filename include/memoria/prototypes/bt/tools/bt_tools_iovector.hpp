
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

#include <memoria/core/iovector/io_vector.hpp>
#include <memoria/core/types/list/sublist.hpp>

#include <memoria/core/packed/misc/packed_sized_struct.hpp>

#include <memoria/core/iovector/io_substream_ssrle_buffer.hpp>
#include <memoria/core/iovector/io_substream_ssrle_buffer_view.hpp>


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


template <int32_t Streams, typename PkdStructsList>
class IOVectorsTFBase
{
protected:
    using NonSizedPackedStructsList = typename SelectNonSizedStruct<Linearize<PkdStructsList>>::Type;

    static constexpr int32_t PkdStructsListSize = ListSize<NonSizedPackedStructsList>;

    using SequencePkdStruct = typename ListHead<
        SublistToEnd<
            NonSizedPackedStructsList,
            PkdStructsListSize - 1
        >
    >::Type;

    using DataStreamsPkdStructsList = SublistFromStart<
        NonSizedPackedStructsList,
        PkdStructsListSize - 1
    >;

    using StreamsSchema = typename StreamsListParserTF<PkdStructsList>::Type;
};



template <int32_t Streams, typename PkdStructsList>
class IOVectorsTF: public IOVectorsTFBase<Streams, PkdStructsList>
{
    using Base = IOVectorsTFBase<Streams, PkdStructsList>;

    struct IOVTypes {
        using PackedStructsList = typename Base::DataStreamsPkdStructsList;
        using SymbolSequence    = typename Base::SequencePkdStruct::GrowableIOSubstream;

        using StreamsSchema     = typename Base::StreamsSchema;
    };

    template <typename T>
    struct CreateIOVectorIVMapperTF {
        using Type = typename T::GrowableIOSubstream;
    };

public:
    using IOVectorT = io::StaticIOVector<IOVTypes, CreateIOVectorIVMapperTF>;
};


template <int32_t Streams, typename PkdStructsList>
class IOVectorViewTF: public IOVectorsTFBase<Streams, PkdStructsList>
{
    using Base = IOVectorsTFBase<Streams, PkdStructsList>;

    struct IOVTypes {
        using PackedStructsList = typename Base::DataStreamsPkdStructsList;
        using SymbolSequence    = typename Base::SequencePkdStruct::IOSubstreamView;
        using StreamsSchema     = typename Base::StreamsSchema;
    };


    template <typename T>
    struct CreateIOVectorViewIVMapperTF {
        using Type = typename T::IOSubstreamView;
    };

public:
    using IOVectorT = io::StaticIOVector<IOVTypes, CreateIOVectorViewIVMapperTF>;
};



template <typename PkdStructsList>
class IOVectorsTF<1, PkdStructsList>
{
protected:
    using NonSizedPackedStructsList = typename SelectNonSizedStruct<Linearize<PkdStructsList>>::Type;

    struct IOVTypes {
        using PackedStructsList = NonSizedPackedStructsList;
        using SymbolSequence    = io::IOSSRLEBufferImpl<1>;
        using StreamsSchema     = IntList<ListSize<NonSizedPackedStructsList>>;
    };

    template <typename T>
    struct CreateIOVectorIVMapperTF {
        using Type = typename T::GrowableIOSubstream;
    };

public:
    using IOVectorT = io::StaticIOVector<IOVTypes, CreateIOVectorIVMapperTF>;
};


template <typename PkdStructsList>
class IOVectorViewTF<1, PkdStructsList>
{
protected:
    using NonSizedPackedStructsList = typename SelectNonSizedStruct<Linearize<PkdStructsList>>::Type;

    struct IOVTypes {
        using PackedStructsList = NonSizedPackedStructsList;
        using SymbolSequence    = io::IOSSRLEBufferView<1>;
        using StreamsSchema = IntList<ListSize<NonSizedPackedStructsList>>;
    };

    template <typename T>
    struct CreateIOVectorIVMapperTF {
        using Type = typename T::IOSubstreamView;
    };

public:
    using IOVectorT = io::StaticIOVector<IOVTypes, CreateIOVectorIVMapperTF>;
};



}}}
