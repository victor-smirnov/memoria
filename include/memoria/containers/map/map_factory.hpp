
// Copyright Victor Smirnov 2011-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_MAP_FACTORY_HPP
#define _MEMORIA_CONTAINERS_MAP_FACTORY_HPP

#include <memoria/prototypes/balanced_tree/bt_factory.hpp>
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



template <typename Profile, typename Key_, typename Value_>
struct BalancedTreeTypes<Profile, memoria::Map<Key_, Value_> >: public BalancedTreeTypes<Profile, memoria::BalancedTree> {

    typedef BalancedTreeTypes<Profile, memoria::BalancedTree>                   Base;

    typedef Value_                                                          	Value;
    typedef TypeList<Key_>                                                  	KeysList;

    static const Int Indexes                                                	= 1;


    typedef TypeList<
    		AllNodeTypes<balanced_tree::TreeMapNode>
    >																			NodeTypesList;

    typedef TypeList<
        		LeafNodeType<TreeMapNode>,
        		InternalNodeType<TreeMapNode>,
        		RootNodeType<TreeMapNode>,
        		RootLeafNodeType<TreeMapNode>
    >																			DefaultNodeTypesList;

    typedef TypeList<
        		StreamDescr<PackedFSETreeTF, PackedFSETreeTF, 1>
    >																			StreamDescriptors;

    typedef BalancedTreeMetadata<
    		typename Base::ID,
    		ListSize<StreamDescriptors>::Value
    >        																	Metadata;


	typedef typename MergeLists<
				typename Base::ContainerPartsList,
				memoria::balanced_tree::NodeNormName,
				memoria::map::CtrToolsName,
				memoria::map::CtrInsert1Name,
				memoria::map::CtrRemoveName,
				memoria::map::CtrApiName
	>::Result                                           						ContainerPartsList;


	typedef typename MergeLists<
				typename Base::IteratorPartsList,
				memoria::map::ItrApiName,
				memoria::map::ItrNavName
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
class CtrTF<Profile, memoria::Map<Key, Value>, T>: public CtrTF<Profile, memoria::BalancedTree, T> {
};






}

#endif
