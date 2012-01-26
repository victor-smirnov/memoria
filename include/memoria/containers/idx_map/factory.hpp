
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_MODELS_IDX_MAP_FACTORY_HPP
#define _MEMORIA_MODELS_IDX_MAP_FACTORY_HPP

#include <memoria/prototypes/templates/map.hpp>
#include <memoria/prototypes/templates/tree_map.hpp>


#include <memoria/containers/idx_map/container/insert.hpp>
#include <memoria/containers/idx_map/container/remove.hpp>
#include <memoria/containers/idx_map/container/model_api.hpp>

//#include <memoria/containers/idx_map/pages/parts.hpp>

#include <memoria/containers/idx_map/iterator/model_api.hpp>

#include <memoria/prototypes/bstree/factory.hpp>

namespace memoria    {



template <typename Profile, Int Indexes_>
struct BTreeTypes<Profile, memoria::SumMap<Indexes_> >:
		public BTreeTypes<Profile, memoria::BSTree> {

	typedef BTreeTypes<Profile, memoria::BSTree> 							Base;

	typedef BigInt															Value;

	static const Int Indexes                                                = Indexes_;

	static const bool MapType                                               = MapTypes::Sum;

	typedef typename AppendLists<
			typename Base::ContainerPartsList,
			typename TLTool<
				memoria::models::idx_map::RemoveName,
				memoria::models::idx_map::InsertName,
				memoria::models::idx_map::ContainerApiName,

				memoria::models::TreeMapName,
				memoria::models::MapName
			>::List
	>::Result                                                               ContainerPartsList;

	typedef typename AppendTool<
			typename Base::IteratorPartsList,
			typename TLTool<
				memoria::models::idx_map::ItrAPIName
			>::List
	>::Result                                                               IteratorPartsList;

};

template <typename Profile, typename T, Int Indexes>
class CtrTF<Profile, memoria::SumMap<Indexes>, T>: public CtrTF<Profile, memoria::BSTree, T> {
};

}

#endif
