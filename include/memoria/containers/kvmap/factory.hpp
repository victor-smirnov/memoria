
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_MODELS_KVMAP_FACTORY_HPP
#define _MEMORIA_MODELS_KVMAP_FACTORY_HPP

#include <memoria/core/types/types.hpp>

#include <memoria/prototypes/templates/map.hpp>
#include <memoria/prototypes/templates/tree_map.hpp>

#include <memoria/prototypes/btree/btree.hpp>

#include <memoria/containers/kvmap/names.hpp>
#include <memoria/containers/kvmap/container/find.hpp>
#include <memoria/containers/kvmap/container/model_api.hpp>

#include <memoria/containers/kvmap/container/checks.hpp>
#include <memoria/containers/kvmap/container/insert.hpp>
#include <memoria/containers/kvmap/container/remove.hpp>

namespace memoria    {

using namespace memoria::btree;


template <typename Profile, typename Key, typename Value_>
struct BTreeTypes<Profile, KVMap<Key, Value_> >: public BTreeTypes<Profile, BTree> {
	typedef BTreeTypes<Profile, BTree> 											Base;

	typedef TL<Key>																KeysList;
	typedef Value_																Value;

	typedef typename AppendLists<
			typename Base::ContainerPartsList,
			typename TLTool<
				memoria::models::TreeMapName,
				memoria::models::MapName,
				memoria::models::kvmap::FindName,
				memoria::models::kvmap::ChecksName,
				memoria::models::kvmap::InsertName,
				memoria::models::kvmap::RemoveName,
				memoria::models::kvmap::MapApiName
			>::List
	>::Result                                                               	ContainerPartsList;
};




template <typename Profile, typename KeyType, typename ValueType, typename T>
class CtrTF<Profile, memoria::KVMap<KeyType, ValueType>, T>: public CtrTF<Profile, memoria::BTree, T> {


};


}

#endif
