
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CONTAINERS_LBLTREE_TOOLS_STREAM_HPP_
#define MEMORIA_CONTAINERS_LBLTREE_TOOLS_STREAM_HPP_

#include <memoria/core/tools/isequencedata.hpp>
#include <memoria/core/packed/array/packed_fse_bitmap.hpp>
#include <memoria/core/types/types.hpp>


namespace memoria   {
namespace louds     {

template <typename... List> struct StreamDescriptorsListHelper;
template <typename T>       struct StreamDescrTF;


template <Int StreamIdx>
struct LoudsStreamTF {
    typedef BigInt                                              Key;
    typedef BigInt                                              Value;

    typedef core::StaticVector<BigInt, 3>						AccumulatorPart;
    typedef core::StaticVector<BigInt, 1>						IteratorPrefixPart;

    typedef PkdFTree<Packed2TreeTypes<Key, Key, 3>> 			NonLeafType;


    static const Int BitsPerSymbol = 1;

    typedef typename PkdFSSeqTF<BitsPerSymbol>::Type            SequenceTypes;

    typedef PkdFSSeq<SequenceTypes> 							LeafType;

};


template <typename... List>
class StreamDescriptorsListBuilder {
    typedef StreamDescr<LoudsStreamTF> 										LoudsStreamDescriptor;
public:
    typedef typename PrependToList<
            typename StreamDescriptorsListHelper<List...>::Type,
            LoudsStreamDescriptor
    >::Result                                                                   Type;
};

template <typename... List>
class StreamDescriptorsListBuilder<TypeList<List...>> {
	typedef StreamDescr<LoudsStreamTF> 										LoudsStreamDescriptor;
public:
    typedef typename PrependToList<
            typename StreamDescriptorsListHelper<List...>::Type,
            LoudsStreamDescriptor
    >::Result                                                                   Type;
};




template <Int Indexes>
struct LabelFTreeNodeTFBase {

	typedef core::StaticVector<BigInt, Indexes>									AccumulatorPart;
	typedef core::StaticVector<BigInt, 1>										IteratorPrefixPart;

    typedef Packed2TreeTypes<
            BigInt, BigInt, Indexes
    >                                                                           TreeTypes;

    typedef PkdFTree<TreeTypes> 												NonLeafType;
};


template <typename Value, Int StreamIdx>
struct LabelFTreeIndexedTF: LabelFTreeNodeTFBase<2> {

    typedef Packed2TreeTypes<
            Value,
            BigInt,
            1
    > TreeTypes;

    typedef PkdFTree<TreeTypes> 												LeafType;
};


template <typename Types, Int StreamIdx>
struct LabelFSEArrayTF {

    typedef typename SelectByIndexTool<
            StreamIdx - Types::StreamsIdxStart,
            typename Types::StreamDescriptors
    >::Result                                                                   Descriptor;

    typedef PackedFSEArrayTypes<
            typename Descriptor::Value
    >                                                                           ArrayTypes;

    typedef PackedFSEArray<ArrayTypes> Type;
};


template <typename Value, Int StreamIdx>
struct LabelFTreeArrayTF: LabelFTreeNodeTFBase<1> {

	typedef PackedFSEArrayTypes<
	            Value
	> ArrayTypes;

	typedef PackedFSEArray<ArrayTypes> 											LeafType;
};



template <UInt BitsPerSymbol, Int StreamIdx>
struct LabelFTreeBitmapTF: LabelFTreeNodeTFBase<1> {

	typedef PackedFSEBitmapTypes<
	            BitsPerSymbol,
	            UBigInt
	> ArrayTypes;

	typedef PackedFSEArray<ArrayTypes> 											LeafType;
};





template <Int Indexes>
struct LabelVTreeNodeTFBase {

	typedef core::StaticVector<BigInt, Indexes>									AccumulatorPart;
	typedef core::StaticVector<BigInt, 1>										IteratorPrefixPart;

    typedef Packed2TreeTypes<
            BigInt, BigInt, Indexes, UByteExintCodec,
            PackedTreeBranchingFactor,
            PackedTreeExintVPB
    > TreeTypes;

    typedef PkdVTree<TreeTypes> 												NonLeafType;
};



template <typename Value, Int Indexes, Int StreamIdx>
struct LabelVTreeByteTF: LabelVTreeNodeTFBase<Indexes> {

    typedef Packed2TreeTypes<
            Value,
            typename bt::ExtendIntType<Value>::Type,
            1,
            UByteExintCodec,
            PackedTreeBranchingFactor,
            PackedTreeExintVPB
    > TreeTypes;

    typedef PkdVTree<TreeTypes> 												LeafType;
};



template <typename Value, Int Indexes, Int StreamIdx>
struct LabelVTreeBitTF: LabelVTreeNodeTFBase<Indexes> {

    typedef Packed2TreeTypes<
            Value,
            typename bt::ExtendIntType<Value>::Type,
            1,
            UBigIntI64Codec,
            PackedTreeBranchingFactor,
            PackedTreeEliasVPB
    > TreeTypes;

    typedef PkdVTree<TreeTypes> 												LeafType;
};



template <typename T>
struct StreamDescrTF<FLabel<T, Indexed::Yes>> {

	template <Int StreamIdx>
	using StreamTF = LabelFTreeIndexedTF<T, StreamIdx>;

    typedef StreamDescr<StreamTF>                								Type;
};


template <typename T>
struct StreamDescrTF<FLabel<T, Indexed::No>> {

	template <Int StreamIdx>
	using StreamTF = LabelFTreeArrayTF<T, StreamIdx>;

    typedef StreamDescr<StreamTF>                								Type;
};

template <Int BitsPerSymbol>
struct StreamDescrTF<FBLabel<BitsPerSymbol>> {

	template <Int StreamIdx>
	using StreamTF = LabelFTreeBitmapTF<BitsPerSymbol, StreamIdx>;

    typedef StreamDescr<StreamTF>                                    			Type;
};





template <typename T>
struct StreamDescrTF<VLabel<T, Granularity::Bit, Indexed::Yes>> {

	template <Int StreamIdx>
	using StreamTF = LabelVTreeBitTF<T, 2, StreamIdx>;

    typedef StreamDescr<StreamTF>                								Type;
};

template <typename T>
struct StreamDescrTF<VLabel<T, Granularity::Byte, Indexed::Yes>> {

	template <Int StreamIdx>
	using StreamTF = LabelVTreeByteTF<T, 2, StreamIdx>;

    typedef StreamDescr<StreamTF>                								Type;
};

template <typename T>
struct StreamDescrTF<VLabel<T, Granularity::Bit, Indexed::No>> {

	template <Int StreamIdx>
	using StreamTF = LabelVTreeBitTF<T, 1, StreamIdx>;

    typedef StreamDescr<StreamTF>                								Type;
};

template <typename T>
struct StreamDescrTF<VLabel<T, Granularity::Byte, Indexed::No>> {
    template <Int StreamIdx>
	using StreamTF = LabelVTreeByteTF<T, 1, StreamIdx>;

    typedef StreamDescr<StreamTF>                								Type;
};





template <typename Head, typename... Tail >
struct StreamDescriptorsListHelper<Head, Tail...> {

    typedef typename StreamDescrTF<Head>::Type Descriptor;

    typedef typename AppendTool<
            Descriptor,
            typename StreamDescriptorsListHelper<Tail...>::Type
    >::Result                                                                   Type;

};

template <>
struct StreamDescriptorsListHelper<> {
    typedef TypeList<>                                                          Type;
};




}
}


#endif /* LBLTREE_TOOLS_HPP_ */
