
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_PROTOTYPES_MAP_CONTAINER_TOOLS_HPP
#define _MEMORIA_PROTOTYPES_MAP_CONTAINER_TOOLS_HPP

#include <memoria/prototypes/bt/bt_tools.hpp>
#include <memoria/core/tools/static_array.hpp>
#include <memoria/core/container/container.hpp>

#include <memoria/core/packed/map/packed_fse_map.hpp>
#include <memoria/core/packed/map/packed_vle_map.hpp>

#include <memoria/core/tools/elias_codec.hpp>

namespace memoria       {
namespace metamap     	{

template <
	Int Indexes,
	typename Key,
	typename Value,
	typename HiddenLabels 	= std::tuple<>,
	typename Labels			= std::tuple<>
>
class MetaMapEntry {
	StaticVector<Key, Indexes> 	keys_;
	Value 						value_;
	HiddenLabels 				hidden_labels_;
	Labels 						labels_;
public:

	using HiddenLabelsType 	= HiddenLabels;
	using LabelsType 		= Labels;

	StaticVector<Key, Indexes>&	keys() 				{return keys_;}
	const StaticVector<Key, Indexes>& keys() const 	{return keys_;}

	Value& value() 									{return value_;}
	const Value& value() const 						{return value_;}

	HiddenLabels& hidden_labels() 					{return hidden_labels_;}
	const HiddenLabels& hidden_labels() const		{return hidden_labels_;}

	Labels& labels() 								{return labels_;}
	const Labels& labels() const 					{return labels_;}

	Key& key() 										{return keys_[0];}
	const Key& key() const							{return keys_[0];}
};


template <typename List> struct LabelTypeListBuilder;

template <Int Bits, typename... Tail>
struct LabelTypeListBuilder<TypeList<LabelDescr<Bits>, Tail...>> {
	typedef typename MergeLists<
				Int,
				typename LabelTypeListBuilder<
					TypeList<Tail...>
				>::Type
	>::Result                                                                   Type;
};


template <>
struct LabelTypeListBuilder<TypeList<>> {
	typedef TypeList<>                                                          Type;
};



template <typename Iterator, typename Container>
class MetaMapIteratorPrefixCache: public bt::BTreeIteratorCache<Iterator, Container> {
    typedef bt::BTreeIteratorCache<Iterator, Container> Base;

    typedef typename Container::Types::IteratorPrefix IteratorPrefix;

    IteratorPrefix prefix_;

public:

    MetaMapIteratorPrefixCache(): Base(), prefix_() {}

    const IteratorPrefix& prefixes() const
    {
        return prefix_;
    }

    IteratorPrefix& prefixes()
    {
    	return prefix_;
    }
};




template <typename Key, Granularity Gr, Int Indexes> struct CompressedMapTF;


template <typename Key, Int Indexes>
struct CompressedMapTF<Key, Granularity::Bit, Indexes> {

	typedef core::StaticVector<BigInt, Indexes + 1>								AccumulatorPart;
	typedef core::StaticVector<BigInt, Indexes + 1>								IteratorPrefixPart;

	typedef PkdFTree<Packed2TreeTypes<Key, Key, Indexes + 1>>					NonLeafType;

    typedef PackedVLEMapTypes<
            Indexes, UBigIntEliasCodec, PackedTreeBranchingFactor, PackedTreeEliasVPB
    > MapTypes;

    typedef PackedVLEMap<MapTypes>                                         		LeafType;
};


template <typename Key, Int Indexes>
struct CompressedMapTF<Key, Granularity::Byte, Indexes> {

	typedef core::StaticVector<BigInt, Indexes + 1>								AccumulatorPart;
	typedef core::StaticVector<BigInt, Indexes + 1>								IteratorPrefixPart;

	typedef PkdFTree<Packed2TreeTypes<Key, Key, Indexes + 1>>					NonLeafType;

    typedef PackedVLEMapTypes<
            Indexes, UByteExintCodec, PackedTreeBranchingFactor, PackedTreeExintVPB
    > MapTypes;

    typedef PackedVLEMap<MapTypes>                                              LeafType;
};



}
}

#endif
