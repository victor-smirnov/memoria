
// Copyright 2013 Victor Smirnov
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

#include <memoria/v1/core/tools/isequencedata.hpp>
#include <memoria/v1/core/packed/array/packed_fse_bitmap.hpp>
#include <memoria/v1/core/packed/array/packed_fse_array.hpp>
#include <memoria/v1/core/packed/array/packed_vle_dense_array.hpp>

#include <memoria/v1/core/config.hpp>


namespace memoria {
namespace v1 {
namespace louds     {

template <typename... List> struct StreamDescriptorsListHelper;
template <typename T>       struct LeafTypeTF;
template <typename T>       struct IdxListTF;



using LoudsStreamTF = StreamTF<
    PkdFSSeq<typename PkdFSSeqTF<1>::Type>,
    FSEBranchStructTF,
    TL<TL<>>
>;

//struct LoudsStreamTF {
//    typedef int64_t                                              Key;
//    typedef int64_t                                              Value;
//
//    typedef core::StaticVector<int64_t, 3>                       BranchNodeEntryPart;
//    typedef core::StaticVector<int64_t, 1>                       IteratorPrefixPart;
//
//    typedef PkdFQTreeT<Key, 3>             NonLeafType;
//    typedef TL<TL<>>                                          IdxRangeList;
//
//
//    static const int32_t BitsPerSymbol = 1;
//
//    typedef typename PkdFSSeqTF<BitsPerSymbol>::Type            SequenceTypes;
//
//    typedef PkdFSSeq<SequenceTypes>                             LeafType;
//
//};


template <typename... List>
class StreamDescriptorsListBuilder {
    typedef LoudsStreamTF                                                       LoudsStreamDescriptor;
public:
    typedef typename PrependToList<
            typename StreamDescriptorsListHelper<List...>::Type,
            LoudsStreamDescriptor
    >::Type                                                                     Type;
};

template <typename... List>
class StreamDescriptorsListBuilder<TypeList<List...>> {
    typedef LoudsStreamTF                                                       LoudsStreamDescriptor;
public:
    typedef typename PrependToList<
            typename StreamDescriptorsListHelper<List...>::Type,
            LoudsStreamDescriptor
    >::Type                                                                     Type;
};




template <int32_t Indexes>
struct LabelFTreeNodeTFBase {

    typedef core::StaticVector<int64_t, Indexes>                                 BranchNodeEntryPart;
    typedef core::StaticVector<int64_t, 1>                                       IteratorPrefixPart;


    typedef PkdFQTreeT<int64_t, Indexes>                                                 NonLeafType;
    typedef TL<TL<>>                                                            IdxRangeList;
};


template <typename Value>
struct LabelFTreeIndexedTF: LabelFTreeNodeTFBase<2> {


    typedef PkdFQTreeT<int64_t, 1, Value>                                                 LeafType;
};



template <typename Value>
struct LabelFTreeArrayTF: LabelFTreeNodeTFBase<1> {

    typedef PackedFSEArrayTypes<
                Value
    > ArrayTypes;

    typedef PackedFSEArray<ArrayTypes>                                          LeafType;
};



template <uint32_t BitsPerSymbol>
struct LabelFTreeBitmapTF: LabelFTreeNodeTFBase<1> {

    typedef PackedFSEBitmapTypes<
                BitsPerSymbol,
                uint64_t
    > ArrayTypes;

    typedef PackedFSEArray<ArrayTypes>                                          LeafType;
};





template <int32_t Indexes>
struct LabelVTreeNodeTFBase {

    typedef core::StaticVector<int64_t, Indexes>                                 BranchNodeEntryPart;
    typedef core::StaticVector<int64_t, 1>                                       IteratorPrefixPart;

//    typedef Packed2TreeTypes<
//            int64_t, int64_t, Indexes, UByteExintCodec,
//            PackedTreeBranchingFactor,
//            PackedTreeExintVPB
//    > TreeTypes;

    typedef PkdVQTreeT<int64_t, Indexes, UByteI7Codec>                           NonLeafType;
    typedef TL<TL<>>                                                            IdxRangeList;
};



template <typename Value, int32_t Indexes>
struct LabelVTreeByteTF: LabelVTreeNodeTFBase<Indexes> {

//    typedef Packed2TreeTypes<
//            Value,
//            typename bt::ExtendIntType<Value>::Type,
//            1,
//            UByteExintCodec,
//            PackedTreeBranchingFactor,
//            PackedTreeExintVPB
//    > TreeTypes;

    typedef PkdVQTreeT<int64_t, 1, UByteI7Codec, Value>                                                 LeafType;
};



template <typename Value, int32_t Indexes>
struct LabelVTreeBitTF: LabelVTreeNodeTFBase<Indexes> {

//    typedef Packed2TreeTypes<
//            Value,
//            typename bt::ExtendIntType<Value>::Type,
//            1,
//            UInt64I64Codec,
//            PackedTreeBranchingFactor,
//            PackedTreeEliasVPB
//    > TreeTypes;

    typedef PkdVDTreeT<int64_t, 1, UInt64I64Codec, Value>                                                 LeafType;
};



template <typename T>
struct LeafTypeTF<FLabel<T, Indexed::Yes>> {
    using Type = PkdFQTreeT<int64_t, 1, T>;

};


template <typename T>
struct LeafTypeTF<FLabel<T, Indexed::No>> {
    using Type = PkdFSQArrayT<T>;
};

template <int32_t BitsPerSymbol>
struct LeafTypeTF<FBLabel<BitsPerSymbol>> {
    using Type = PackedFSEBitmapT<BitsPerSymbol>;
};





template <typename T>
struct LeafTypeTF<VLabel<T, Granularity::Bit, Indexed::Yes>> {
    using Type = PkdVQTreeT<int64_t, 1, UInt64I64Codec, T>;
};

template <typename T>
struct LeafTypeTF<VLabel<T, Granularity::int8_t, Indexed::Yes>> {
    using Type = PkdVQTreeT<int64_t, 1, UByteI7Codec, T>;
};

template <typename T>
struct LeafTypeTF<VLabel<T, Granularity::Bit, Indexed::No>> {
    using Type = PkdVDArrayT<T, 1, UInt64I64Codec>;
};

template <typename T>
struct LeafTypeTF<VLabel<T, Granularity::int8_t, Indexed::No>> {
    using Type = PkdVDArrayT<T, 1, UByteI7Codec>;
};


template <typename T>
struct IdxListTF {
    using Type = TL<>;
};


template <typename Head, typename... Tail >
struct StreamDescriptorsListHelper<Head, Tail...> {

    using LeafType = MergeLists<
            typename LeafTypeTF<Head>::Type,
            typename StreamDescriptorsListHelper<Tail...>::LeafType
    >;

    using IdxList = MergeLists<
            TL<typename IdxListTF<Head>::Type>,
            typename StreamDescriptorsListHelper<Tail...>::IdxList
    >;

};

template <>
struct StreamDescriptorsListHelper<> {
    using LeafType = TypeList<>;
    using IdxList  = TypeList<>;
};




}
}}