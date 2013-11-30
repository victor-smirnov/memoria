
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef _MEMORIA_PROTOTYPES_MAP_PACKED_ADAPTORS_HPP
#define _MEMORIA_PROTOTYPES_MAP_PACKED_ADAPTORS_HPP

#include <memoria/prototypes/metamap/metamap_tools.hpp>

namespace memoria 	{
namespace metamap	{

template <typename Map, Int Indexes, typename Key, typename Value, typename HiddenLabels, typename Labels>
void InsertEntry(Map* map, Int idx, const MetaMapEntry<Indexes, Key, Value, HiddenLabels, Labels>& entry)
{
	map->insert(idx, entry.keys(), entry.value());
}


template <typename Value, typename Stream>
Value& GetValueRef(Stream* stream, Int idx)
{
	return stream->value(idx);
}

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
	Entry entry;

	stream->sums(idx, entry.keys());
	entry.value() = stream->value(idx);

	// Process entry labels here

	return entry;
}

template <typename Stream, typename Entry>
void SetEntry(const Stream* stream, Int idx, const Entry& entry)
{
	// Process entry keys here

	stream->value(idx) = entry.value();

	// Process entry labels here
}


}
}

#endif
