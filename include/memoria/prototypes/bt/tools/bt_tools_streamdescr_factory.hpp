
// Copyright 2015 Victor Smirnov
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

#include <memoria/core/tools/static_array.hpp>

#include <memoria/core/types/list/append.hpp>
#include <memoria/core/types/list/linearize.hpp>
#include <memoria/core/types/list/list_tree.hpp>
#include <memoria/core/exceptions/exceptions.hpp>



#include <memoria/core/packed/tools/packed_allocator_types.hpp>
#include <memoria/core/packed/packed.hpp>

#ifdef HAVE_BOOST
#include <memoria/core/bignum/bigint.hpp>
#endif

#include <memoria/core/strings/string.hpp>

#include <memoria/core/tools/i7_codec.hpp>
#include <memoria/core/tools/elias_codec.hpp>
#include <memoria/core/tools/exint_codec.hpp>

#include <iostream>
#include <tuple>
#include <utility>
#include <type_traits>

namespace memoria {
namespace bt {


template <PkdSearchType SearchType, typename KeyType_, int32_t Indexes_>
struct IdxSearchType {
    static constexpr PkdSearchType  Value = SearchType;
    static constexpr int32_t Indexes      = Indexes_;

    using KeyType = KeyType_;
};

template <typename T> struct FSEBranchStructTF;


template <typename KeyType, int32_t Indexes>
struct FSEBranchStructTF<IdxSearchType<PkdSearchType::SUM, KeyType, Indexes>> {
    using Type = PkdFQTreeT<KeyType, Indexes>;
};

template <typename KeyType>
struct FSEBranchStructTF<IdxSearchType<PkdSearchType::SUM, KeyType, 0>> {
    using Type = PackedEmptyStruct<KeyType, PkdSearchType::SUM>;
};



template <typename KeyType>
struct FSEBranchStructTF<IdxSearchType<PkdSearchType::MAX, KeyType, 0>> {
    using Type = PackedEmptyStruct<KeyType, PkdSearchType::MAX>;
};


//template <typename T> struct VLQBranchStructTF;

//template <typename KeyType, int32_t Indexes>
//struct VLQBranchStructTF<IdxSearchType<PkdSearchType::SUM, KeyType, Indexes>> {
//    using Type = PkdVQTreeT<KeyType, Indexes, ValueCodec>;
//};

//template <typename KeyType>
//struct VLQBranchStructTF<IdxSearchType<PkdSearchType::MAX, KeyType, 0>> {
//    using Type = PackedEmptyStruct<KeyType, PkdSearchType::MAX>;
//};

//template <typename KeyType>
//struct VLQBranchStructTF<IdxSearchType<PkdSearchType::SUM, KeyType, 0>> {
//    using Type = PackedEmptyStruct<KeyType, PkdSearchType::SUM>;
//};





//template <typename T> struct VLDBranchStructTF;

//template <typename KeyType, int32_t Indexes>
//struct VLDBranchStructTF<IdxSearchType<PkdSearchType::SUM, KeyType, Indexes>> {
//    using Type = PkdVDTreeT<KeyType, Indexes, UByteI7Codec>;
//};






namespace detail {

    template <typename List> struct CheckPackedStructsHaveSameSearchType;

    template <typename PkdStruct1, typename PkdStruct2, typename... Tail>
    struct CheckPackedStructsHaveSameSearchType<TL<PkdStruct1, PkdStruct2, Tail...>>
    {
        static constexpr PkdSearchType SearchType1 = PkdKeySearchType<PkdStruct1>;
        static constexpr PkdSearchType SearchType2 = PkdKeySearchType<PkdStruct2>;

        static constexpr bool Value = SearchType1 == SearchType2 && CheckPackedStructsHaveSameSearchType<
                TL<PkdStruct2, Tail...>
        >::Value;
    };

	

    template <typename PkdStruct1, typename PkdStruct2>
    struct CheckPackedStructsHaveSameSearchType<TL<PkdStruct1, PkdStruct2>>
    {
        static constexpr PkdSearchType SearchType1 = PkdKeySearchType<PkdStruct1>;
        static constexpr PkdSearchType SearchType2 = PkdKeySearchType<PkdStruct2>;

        static constexpr bool Value = SearchType1 == SearchType2;
    };

    template <typename PkdStruct>
    struct CheckPackedStructsHaveSameSearchType<TL<PkdStruct>>: HasValue<bool, true> {};

	



	
    template <typename List> struct CheckPackedStructsHaveSameKeyType;

    template <typename PkdStruct1, typename PkdStruct2, typename... Tail>
    struct CheckPackedStructsHaveSameKeyType<TL<PkdStruct1, PkdStruct2, Tail...>>
    {
        using KeyType1 = PkdSearchKeyType<PkdStruct1>;
        using KeyType2 = PkdSearchKeyType<PkdStruct2>;

        static constexpr bool Value = std::is_same<KeyType1, KeyType2>::value
            && CheckPackedStructsHaveSameKeyType<
                TL<PkdStruct2, Tail...>
        >::Value;
    };

    template <typename PkdStruct1, typename PkdStruct2>
    struct CheckPackedStructsHaveSameKeyType<TL<PkdStruct1, PkdStruct2>>
    {
        using KeyType1 = PkdSearchKeyType<PkdStruct1>;
        using KeyType2 = PkdSearchKeyType<PkdStruct2>;

        static constexpr bool Value = std::is_same<KeyType1, KeyType2>::value;
    };

    template <typename PkdStruct>
    struct CheckPackedStructsHaveSameKeyType<TL<PkdStruct>>: HasValue<bool, true> {};




    template <typename List> struct SumIndexes;

    template <PkdSearchType SearchType, typename KeyType, int32_t Indexes, typename... Tail>
    struct SumIndexes<TL<IdxSearchType<SearchType, KeyType, Indexes>, Tail...>> {
        static constexpr int32_t Value = Indexes + SumIndexes<TL<Tail...>>::Value;
    };

    template <>
    struct SumIndexes<TL<>> {
        static constexpr int32_t Value = 0;
    };


    template <typename T1, typename T2, bool LT = (sizeof(T1) < sizeof(T2)) > struct SelectMaxTypeT;

    template <typename T1, typename T2>
    using SelectMaxType = typename SelectMaxTypeT<T1, T2>::Type;

    template <typename T1, typename T2>
    struct SelectMaxTypeT<T1, T2, true>: HasType<T2> {};

    template <typename T1, typename T2>
    struct SelectMaxTypeT<T1, T2, false>: HasType<T1> {};


    template <typename List> struct GetSUPSearchKeyTypeT;
    template <typename List> using GetSUPSearchKeyType = typename GetSUPSearchKeyTypeT<List>::Type;

    template <PkdSearchType SearchType, typename KeyType, int32_t Indexes, typename... Tail>
    struct GetSUPSearchKeyTypeT<TL<IdxSearchType<SearchType, KeyType, Indexes>, Tail...>>: HasType<
        SelectMaxType<
                KeyType,
                GetSUPSearchKeyType<TL<Tail...>>
        >
    > {};

    template <PkdSearchType SearchType, typename KeyType, int32_t Indexes>
    struct GetSUPSearchKeyTypeT<TL<IdxSearchType<SearchType, KeyType, Indexes>>>: HasType <KeyType> {};





    template <typename List, typename SumType, int32_t Idx> struct BuildKeyMetadataListT;

    template <typename List, typename SumType, int32_t Idx = 0>
    using BuildKeyMetadataList = typename BuildKeyMetadataListT<List, SumType, Idx>::Type;


    template <typename PkdStruct, typename... Tail, typename SumType, int32_t Idx>
    struct BuildKeyMetadataListT<TL<PkdStruct, Tail...>, SumType, Idx>
    {

        using KeyType = PkdSearchKeyDataType<PkdStruct>;
        static constexpr PkdSearchType SearchType = PkdKeySearchType<PkdStruct>;

        using Type = MergeLists<
                IdxSearchType<
                    PkdKeySearchType<PkdStruct>,
                    IfThenElse<
                        SearchType == PkdSearchType::SUM,
                        SelectMaxType<
                            SumType,
                            KeyType
                        >,
                        KeyType
                    >,
                    PkdStructIndexes<PkdStruct>
                >,
                BuildKeyMetadataList<TL<Tail...>, SumType, Idx + 1>
        >;
    };

    template <typename PkdStruct, typename... Tail1, typename... Tail2, typename SumType, int32_t Idx>
    struct BuildKeyMetadataListT<TL<TL<PkdStruct, Tail1...>, Tail2...>, SumType, Idx>
    {
        //static_assert(!std::is_same_v<PkdStruct, int64_t>, "");

        using List = TL<PkdStruct, Tail1...>;

        static_assert(
                CheckPackedStructsHaveSameSearchType<List>::Value,
                "All grouped together leaf packed structs must be of the same search type"
        );

        static constexpr PkdSearchType SearchType = PkdKeySearchType<PkdStruct>;

        static_assert(
                SearchType != PkdSearchType::MAX || CheckPackedStructsHaveSameKeyType<List>::Value,
                "All grouped together leaf packed structs with MAX search type must have the same key type"
        );

        using LeafStructGroupKeyMetadataList = BuildKeyMetadataList<List, SumType>;

        static constexpr PkdSearchType GroupSearchType = PkdKeySearchType<PkdStruct>;

        static constexpr int32_t TotalIndexes = SumIndexes<LeafStructGroupKeyMetadataList>::Value;

        using GroupKeyType = IfThenElse<
                GroupSearchType == PkdSearchType::SUM,
                SelectMaxType<
                    SumType,
                    GetSUPSearchKeyType<LeafStructGroupKeyMetadataList>
                >,
                PkdSearchKeyDataType<PkdStruct>
        >;

        using Type = MergeLists<
                IdxSearchType<
                    GroupSearchType,
                    GroupKeyType,
                    TotalIndexes
                >,
                BuildKeyMetadataList<TL<Tail2...>, SumType, Idx + 1>
        >;
    };

    template <typename SumType, int32_t Idx>
    struct BuildKeyMetadataListT<TL<>, SumType, Idx> {
        using Type = TL<>;
    };



    template <typename List, template <typename> class BranchStructTF> struct BranchStructListBuilderT;

    template <typename List, template <typename> class BranchStructTF>
    using BranchStructListBuilder = typename BranchStructListBuilderT<List, BranchStructTF>::Type;

    template <typename IdxSearchTypeT, typename... Tail, template <typename> class BranchStructTF>
    struct BranchStructListBuilderT<TL<IdxSearchTypeT, Tail...>, BranchStructTF>: HasType<
        MergeLists<
            typename BranchStructTF<IdxSearchTypeT>::Type,
            BranchStructListBuilder<TL<Tail...>, BranchStructTF>
        >
    > {};

    template <template <typename> class BranchStructTF>
    struct BranchStructListBuilderT<TL<>, BranchStructTF>: HasType<TL<>>{};
}




template <typename LeafStructList, template <typename> class BranchStructTF, typename SumType> struct BTStreamDescritorsBuilder;

template <typename LeafStruct, typename... Tail, template <typename> class BranchStructTF, typename SumType>
struct BTStreamDescritorsBuilder<TL<LeafStruct, Tail...>, BranchStructTF, SumType>
{
    static_assert(
            PkdKeySearchType<LeafStruct> == PkdSearchType::SUM,
            "First packed struct in each stream must has PkdSearchType::SUM search type. Consider prepending PackedSizedStruct"
    );

    using StructList = TL<LeafStruct, Tail...>;

    using KeyMetadataList = detail::BuildKeyMetadataList<StructList, SumType>;

    using Type = detail::BranchStructListBuilder<KeyMetadataList, BranchStructTF>;
};

template <typename LeafStruct, typename... Tail1, typename... Tail2, template <typename> class BranchStructTF, typename SumType>
struct BTStreamDescritorsBuilder<TL<TL<LeafStruct, Tail1...>, Tail2...>, BranchStructTF, SumType>
{
    static_assert(
            PkdKeySearchType<LeafStruct> == PkdSearchType::SUM,
            "First packed struct in each stream must has PkdSearchType::SUM search type. Consider prepending PackedSizedStruct"
    );

    using StructList = TL<TL<LeafStruct, Tail1...>, Tail2...>;

    using KeyMetadataList = detail::BuildKeyMetadataList<StructList, SumType>;

    using Type = detail::BranchStructListBuilder<KeyMetadataList, BranchStructTF>;
};


}}
