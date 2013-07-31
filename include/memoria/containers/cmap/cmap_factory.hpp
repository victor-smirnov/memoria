
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_CMAP_FACTORY_HPP
#define _MEMORIA_CONTAINERS_CMAP_FACTORY_HPP

#include <memoria/prototypes/bt/bt_factory.hpp>
#include <memoria/prototypes/ctr_wrapper/ctrwrapper_factory.hpp>

#include <memoria/containers/cmap/cmap_walkers.hpp>
#include <memoria/containers/cmap/cmap_tools.hpp>

#include <memoria/containers/cmap/container/cmap_c_tools.hpp>
#include <memoria/containers/cmap/container/cmap_c_insert.hpp>
#include <memoria/containers/cmap/container/cmap_c_remove.hpp>
#include <memoria/containers/cmap/container/cmap_c_api.hpp>

#include <memoria/containers/cmap/cmap_iterator.hpp>
#include <memoria/containers/cmap/iterator/cmap_i_api.hpp>
#include <memoria/containers/cmap/iterator/cmap_i_nav.hpp>




#include <memoria/containers/cmap/cmap_names.hpp>

namespace memoria    {



template <typename Profile, typename Key_, typename Value_>
struct BalancedTreeTypes<Profile, memoria::CMap<Key_, Value_> >: public BalancedTreeTypes<Profile, memoria::BalancedTree> {

    typedef BalancedTreeTypes<Profile, memoria::BalancedTree>                   Base;

    typedef Value_                                                          	Value;
    typedef TypeList<Key_>                                                  	KeysList;

    static const Int Indexes                                                	= 1;


    typedef TypeList<
    		AllNodeTypes<bt::TreeMapNode>
    >																			NodeTypesList;

    typedef TypeList<
        		LeafNodeType<TreeMapNode>,
        		InternalNodeType<TreeMapNode>,
        		RootNodeType<TreeMapNode>,
        		RootLeafNodeType<TreeMapNode>
    >																			DefaultNodeTypesList;

    typedef TypeList<
        		StreamDescr<PackedVLETreeTF, PackedVLETreeTF, 1>
    >																			StreamDescriptors;

    typedef BalancedTreeMetadata<
    		typename Base::ID,
    		ListSize<StreamDescriptors>::Value
    >        																	Metadata;


	typedef typename MergeLists<
				typename Base::ContainerPartsList,
				memoria::bt::NodeComprName,
				memoria::cmap::CtrToolsName,
				memoria::cmap::CtrInsertName,
				memoria::cmap::CtrRemoveName,
				memoria::cmap::CtrApiName
	>::Result                                           						ContainerPartsList;


	typedef typename MergeLists<
				typename Base::IteratorPartsList,
				memoria::cmap::ItrApiName,
				memoria::cmap::ItrNavName
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
class CtrTF<Profile, memoria::CMap<Key, Value>, T>: public CtrTF<Profile, memoria::BalancedTree, T> {
};





}

#endif
