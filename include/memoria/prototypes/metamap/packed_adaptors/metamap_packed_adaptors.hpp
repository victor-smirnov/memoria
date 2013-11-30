
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


}
}

#endif
