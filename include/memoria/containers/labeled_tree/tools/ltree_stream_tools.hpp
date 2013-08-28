
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

template <typename... List>
class StreamDescriptorsListBuilder {
    typedef StreamDescr<PkdFTreeTF, PackedFSESeqTF, 3, 1> LoudsStreamDescriptor;
public:
    typedef typename PrependToList<
            typename StreamDescriptorsListHelper<List...>::Type,
            LoudsStreamDescriptor
    >::Result                                                                   Type;
};

template <typename... List>
class StreamDescriptorsListBuilder<TypeList<List...>> {
    typedef StreamDescr<PkdFTreeTF, PackedFSESeqTF, 3, 1> LoudsStreamDescriptor;
public:
    typedef typename PrependToList<
            typename StreamDescriptorsListHelper<List...>::Type,
            LoudsStreamDescriptor
    >::Result                                                                   Type;
};




template <typename Types, Int StreamIdx>
struct PkdFTreeNodeTF {
    typedef typename SelectByIndexTool<
            StreamIdx - Types::StreamsIdxStart,
            typename Types::StreamDescriptors
    >::Result                                                                   Descriptor;

    typedef Packed2TreeTypes<
            BigInt, BigInt, Descriptor::NodeIndexes
    >                                                                           TreeTypes;

    typedef PkdFTree<TreeTypes> Type;
};


template <typename Types, Int StreamIdx>
struct PkdFTreeLeafTF {
    typedef typename SelectByIndexTool<
            StreamIdx - Types::StreamsIdxStart,
            typename Types::StreamDescriptors
    >::Result                                                                   Descriptor;

    typedef Packed2TreeTypes<
            typename Descriptor::Value,
            BigInt,
            Descriptor::LeafIndexes
    >                                                                           TreeTypes;

    typedef PkdFTree<TreeTypes> Type;
};


template <typename Types, Int StreamIdx>
struct PackedFSEArrayLeafTF {

    typedef typename SelectByIndexTool<
            StreamIdx - Types::StreamsIdxStart,
            typename Types::StreamDescriptors
    >::Result                                                                   Descriptor;

    typedef PackedFSEArrayTypes<
            typename Descriptor::Value
    >                                                                           ArrayTypes;

    typedef PackedFSEArray<ArrayTypes> Type;
};

template <typename Types, Int StreamIdx>
struct PackedFSEBitmapLeafTF {

    typedef typename SelectByIndexTool<
            StreamIdx - Types::StreamsIdxStart,
            typename Types::StreamDescriptors
    >::Result                                                                   Descriptor;

    typedef PackedFSEBitmapTypes<
            Descriptor::LeafIndexes,
            UBigInt
    >                                                                           ArrayTypes;

    typedef PackedFSEArray<ArrayTypes> Type;
};



template <typename Types, Int StreamIdx>
struct PkdVTreeNodeTF {

    typedef typename SelectByIndexTool<
            StreamIdx - Types::StreamsIdxStart,
            typename Types::StreamDescriptors
    >::Result                                                                   Descriptor;

    typedef Packed2TreeTypes<
            BigInt, BigInt, Descriptor::NodeIndexes, UByteExintCodec,
            PackedTreeBranchingFactor,
            PackedTreeExintVPB
    >                                                                           TreeTypes;

    typedef PkdVTree<TreeTypes> Type;
};



template <typename Types, Int StreamIdx>
struct PkdVTreeByteLeafTF {
    typedef typename SelectByIndexTool<
            StreamIdx - Types::StreamsIdxStart,
            typename Types::StreamDescriptors
    >::Result                                                                   Descriptor;

    typedef typename Descriptor::Value                                          Value;

    typedef Packed2TreeTypes<
            Value,
            typename bt::ExtendIntType<Value>::Type,
            Descriptor::LeafIndexes,
            UByteExintCodec,
            PackedTreeBranchingFactor,
            PackedTreeExintVPB
    >                                                                           TreeTypes;

    typedef PkdVTree<TreeTypes> Type;
};



template <typename Types, Int StreamIdx>
struct PkdVTreeBitLeafTF {
    typedef typename SelectByIndexTool<
            StreamIdx - Types::StreamsIdxStart,
            typename Types::StreamDescriptors
    >::Result                                                                   Descriptor;

    typedef typename Descriptor::Value                                          Value;

    typedef Packed2TreeTypes<
            Value,
            typename bt::ExtendIntType<Value>::Type,
            Descriptor::LeafIndexes,
            UBigIntI64Codec,
            PackedTreeBranchingFactor,
            PackedTreeEliasVPB
    >                                                                           TreeTypes;

    typedef PkdVTree<TreeTypes> Type;
};



template <typename T>
struct StreamDescrTF<FLabel<T, Indexed::Yes>> {
    typedef StreamDescr<PkdFTreeNodeTF, PkdFTreeLeafTF, 2, 1, T>                Type;
};


template <typename T>
struct StreamDescrTF<FLabel<T, Indexed::No>> {
    typedef StreamDescr<PkdFTreeNodeTF, PackedFSEArrayLeafTF, 1, 1, T>          Type;
};

template <Int BitsPerSymbol>
struct StreamDescrTF<FBLabel<BitsPerSymbol>> {
    typedef StreamDescr<
            PkdFTreeNodeTF,
            PackedFSEArrayLeafTF,
            1,
            BitsPerSymbol
    >                                                                           Type;
};





template <typename T>
struct StreamDescrTF<VLabel<T, Granularity::Bit, Indexed::Yes>> {
    typedef StreamDescr<PkdVTreeNodeTF, PkdVTreeBitLeafTF, 2, 1, T>             Type;
};

template <typename T>
struct StreamDescrTF<VLabel<T, Granularity::Byte, Indexed::Yes>> {
    typedef StreamDescr<PkdVTreeNodeTF, PkdVTreeByteLeafTF, 2, 1, T>            Type;
};

template <typename T>
struct StreamDescrTF<VLabel<T, Granularity::Bit, Indexed::No>> {
    typedef StreamDescr<PkdVTreeNodeTF, PkdVTreeBitLeafTF, 1, 1, T>             Type;
};

template <typename T>
struct StreamDescrTF<VLabel<T, Granularity::Byte, Indexed::No>> {
    typedef StreamDescr<PkdVTreeNodeTF, PkdVTreeByteLeafTF, 1, 1, T>            Type;
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
