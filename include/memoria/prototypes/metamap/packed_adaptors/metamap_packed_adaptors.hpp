
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef _MEMORIA_PROTOTYPES_MAP_PACKED_ADAPTORS_HPP
#define _MEMORIA_PROTOTYPES_MAP_PACKED_ADAPTORS_HPP

#include <memoria/prototypes/metamap/metamap_tools.hpp>
#include <memoria/core/packed/map/packed_vle_map.hpp>

#include <tuple>

namespace memoria 	{
namespace metamap	{

template <typename Map, Int Indexes, typename Key, typename Value, typename HiddenLabels, typename Labels, typename Accum>
void InsertEntry(
		Map* map,
		Int idx,
		const MetaMapEntry<Indexes, Key, Value, HiddenLabels, Labels>& entry,
		Accum& sums
	)
{
	map->insert(idx, entry, sums);
}


template <typename StreamTypes, Int Indexes, typename Key, typename Value, typename HiddenLabels, typename Labels, typename Accum>
void InsertEntry(
		PackedVLEMap<StreamTypes>* map,
		Int idx,
		const MetaMapEntry<Indexes, Key, Value, HiddenLabels, Labels>& entry,
		Accum& sums
	)
{
	sums[0]++;
	sums.sumAt(1, entry.indexes());

	map->insert(idx, entry.indexes(), entry.value());
}


template <typename Value, typename Stream>
Value& GetValueRef(Stream* stream, Int idx)
{
	return stream->value(idx);
}

//template <typename Value, typename StreamTypes>
//Value& GetValueRef(PackedVLEMap<StreamTypes>* stream, Int idx)
//{
//	return stream->value(idx).value();
//}

template <typename Value, typename Stream>
Value GetValue(const Stream* stream, Int idx)
{
	return stream->value(idx);
}

template <typename Stream, typename Value>
void SetValue(Stream* stream, Int idx, const Value& value)
{
	stream->value(idx) = value;
}


template <typename Entry, typename Stream>
Entry GetEntry(const Stream* stream, Int idx)
{
	return stream->template entry<Entry>(idx);
}

template <typename Stream, typename Entry>
void SetEntry(const Stream* stream, Int idx, const Entry& entry)
{
	// Process entry keys here

	stream->value(idx) = entry.value();

	// Process entry labels here
}

template <typename IndexType, typename Stream>
IndexType GetLeafIndex(const Stream* stream, Int idx, Int index_num)
{
	return stream->key(index_num, idx);
}


template <typename IndexType, typename StreamTypes>
IndexType GetLeafIndex(const PackedVLEMap<StreamTypes>* stream, Int idx, Int index_num)
{
	return stream->tree()->value(index_num, idx);
}



}
}

#endif
