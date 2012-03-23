
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_MODELS_IDX_MAP_FACTORY_HPP
#define _MEMORIA_MODELS_IDX_MAP_FACTORY_HPP

#include <memoria/containers/map/container/api.hpp>
#include <memoria/containers/map/iterator/api.hpp>

#include <memoria/prototypes/bstree/factory.hpp>

namespace memoria    {



template <typename Profile, Int Indexes_>
struct BTreeTypes<Profile, memoria::Map<Indexes_> >:
		public BTreeTypes<Profile, memoria::BSTree> {

	typedef BTreeTypes<Profile, memoria::BSTree> 							Base;

	typedef BigInt															Value;

	static const Int Indexes                                                = Indexes_;

	static const bool MapType                                               = MapTypes::Sum;

	typedef typename AppendLists<
			typename Base::ContainerPartsList,
			typename TLTool<
				memoria::models::idx_map::CtrApiName
			>::List
	>::Result                                                               ContainerPartsList;

	typedef typename AppendTool<
			typename Base::IteratorPartsList,
			typename TLTool<
				memoria::models::idx_map::ItrApiName
			>::List
	>::Result                                                               IteratorPartsList;

};

template <typename Profile, typename T, Int Indexes>
class CtrTF<Profile, memoria::Map<Indexes>, T>: public CtrTF<Profile, memoria::BSTree, T> {
};

}

#endif
