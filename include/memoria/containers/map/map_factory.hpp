
// Copyright Victor Smirnov 2011-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_MAP_FACTORY_HPP
#define _MEMORIA_CONTAINERS_MAP_FACTORY_HPP

#include <memoria/prototypes/bt/bt_factory.hpp>
#include <memoria/prototypes/ctr_wrapper/ctrwrapper_factory.hpp>

#include <memoria/containers/map/map_walkers.hpp>
#include <memoria/containers/map/map_tools.hpp>

#include <memoria/containers/map/container/map_c_tools.hpp>
#include <memoria/containers/map/container/map_c_insert.hpp>
#include <memoria/containers/map/container/map_c_remove.hpp>
#include <memoria/containers/map/container/map_c_api.hpp>

#include <memoria/containers/map/map_iterator.hpp>
#include <memoria/containers/map/iterator/map_i_api.hpp>
#include <memoria/containers/map/iterator/map_i_nav.hpp>





#include <memoria/containers/map/map_names.hpp>

namespace memoria    {


template <typename Types, Int StreamIdx>
struct PackedFSEMapTF {

    typedef typename Types::Key                                                 Key;
    typedef typename Types::Value                                               Value;

    typedef typename SelectByIndexTool<
    		StreamIdx,
    		typename Types::StreamDescriptors
    >::Result																	Descriptor;

	typedef PackedFSEMapTypes<
			Key, Value, Descriptor::NodeIndexes
	>																			MapTypes;

	typedef PackedFSEMap<MapTypes> 												Type;
};


template <typename Profile, typename Key_, typename Value_>
struct BTTypes<Profile, memoria::Map<Key_, Value_> >: public BTTypes<Profile, memoria::BT> {

    typedef BTTypes<Profile, memoria::BT>                   Base;

    typedef Value_                                                          	Value;
    typedef TypeList<Key_>                                                  	KeysList;

    static const Int Indexes                                                	= 1;


    typedef TypeList<
    		LeafNodeTypes<LeafNode>,
    		NonLeafNodeTypes<BranchNode>
    >																			NodeTypesList;

    typedef TypeList<
        		LeafNodeType<LeafNode>,
        		BranchNodeType<BranchNode>
    >																			DefaultNodeTypesList;

    typedef TypeList<
        		StreamDescr<PkdFTreeTF, PackedFSEMapTF, 1>
    >																			StreamDescriptors;

    typedef BalancedTreeMetadata<
    		typename Base::ID,
    		ListSize<StreamDescriptors>::Value
    >        																	Metadata;


	typedef typename MergeLists<
				typename Base::ContainerPartsList,
				bt::NodeComprName,
				map::CtrToolsName,
				map::CtrInsert1Name,
				map::CtrRemoveName,
				map::CtrApiName
	>::Result                                           						ContainerPartsList;


	typedef typename MergeLists<
				typename Base::IteratorPartsList,
				map::ItrApiName,
				map::ItrNavName
	>::Result                                           						IteratorPartsList;


	template <typename Iterator, typename Container>
	struct IteratorCacheFactory {
		typedef memoria::map::MapIteratorPrefixCache<Iterator, Container> Type;
	};



    template <typename Types>
    using FindLTWalker 		= ::memoria::map::FindLTWalker<Types>;

    template <typename Types>
    using FindLEWalker 		= ::memoria::map::FindLEWalker<Types>;



    template <typename Types>
    using FindBeginWalker 	= ::memoria::map::FindBeginWalker<Types>;

    template <typename Types>
    using FindEndWalker 	= ::memoria::map::FindEndWalker<Types>;

    template <typename Types>
    using FindRBeginWalker 	= ::memoria::map::FindRBeginWalker<Types>;

    template <typename Types>
    using FindREndWalker 	= ::memoria::map::FindREndWalker<Types>;
};



template <typename Profile, typename Key, typename Value, typename T>
class CtrTF<Profile, memoria::Map<Key, Value>, T>: public CtrTF<Profile, memoria::BT, T> {
};






}

#endif
