
// Copyright Victor Smirnov 2011-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_CMAP_FACTORY_TYPES_HPP
#define _MEMORIA_CONTAINERS_CMAP_FACTORY_TYPES_HPP

#include <memoria/prototypes/bt/bt_factory.hpp>
#include <memoria/prototypes/ctr_wrapper/ctrwrapper_factory.hpp>

#include <memoria/prototypes/bt/walkers/bt_skip_walkers.hpp>
#include <memoria/prototypes/bt/walkers/bt_find_walkers.hpp>
#include <memoria/prototypes/bt/walkers/bt_edge_walkers.hpp>

#include <memoria/prototypes/bt/packed_adaptors/bt_tree_adaptor.hpp>

#include <memoria/containers/map/map_walkers.hpp>
#include <memoria/containers/map/map_tools.hpp>

#include <memoria/containers/map/container/cmap_c_insert.hpp>
#include <memoria/containers/map/container/cmap_c_nav.hpp>
#include <memoria/containers/map/container/cmap_c_remove.hpp>
#include <memoria/containers/map/container/cmap_c_api.hpp>

#include <memoria/containers/map/map_iterator.hpp>
#include <memoria/containers/map/iterator/cmap_i_api.hpp>
#include <memoria/containers/map/iterator/cmap_i_nav.hpp>
#include <memoria/containers/map/iterator/cmap_i_value.hpp>

#include <memoria/containers/map/map_names.hpp>

#include <memoria/prototypes/metamap/metamap_factory.hpp>

namespace memoria    {


template <typename Profile, Granularity gr>
struct BTTypes<Profile, memoria::CMap<gr> >: public BTTypes<Profile, MetaMap<1, VLen<gr, BigInt>, BigInt>> {

};


/*


template <typename Profile, Granularity gr>
struct BTTypes<Profile, memoria::CMap<gr> >: public BTTypes<Profile, memoria::BT> {

    typedef BTTypes<Profile, memoria::BT>                                       Base;

    typedef BigInt                                                              Value;
    typedef BigInt                                                    			Key;


    typedef TypeList<
            LeafNodeTypes<LeafNode>,
            NonLeafNodeTypes<BranchNode>
    >                                                                           NodeTypesList;

    typedef TypeList<
            TreeNodeType<LeafNode>,
            TreeNodeType<BranchNode>
    >                                                                           DefaultNodeTypesList;



    using StreamTF = map::CompressedMapTF<BigInt, gr, 1>;


    typedef TypeList<StreamTF>                                                  StreamDescriptors;

    typedef BalancedTreeMetadata<
            typename Base::ID,
            ListSize<StreamDescriptors>::Value
    >                                                                           Metadata;


    typedef typename MergeLists<
                typename Base::ContainerPartsList,
                bt::NodeComprName,

                map::CtrCNavName,
                map::CtrCInsertName,
                map::CtrCRemoveName,
                map::CtrCApiName
    >::Result                                                                   ContainerPartsList;


    typedef typename MergeLists<
                typename Base::IteratorPartsList,
                map::ItrCApiName,
                map::ItrCNavName,
                map::ItrCValueName
    >::Result                                                                   IteratorPartsList;


    template <typename Iterator, typename Container>
    struct IteratorCacheFactory {
        typedef map::CMapIteratorPrefixCache<Iterator, Container> Type;
    };



    template <typename Types>
    using FindGTWalker      = bt1::FindGTForwardWalker<Types, bt1::DefaultIteratorPrefixFn>;

    template <typename Types>
    using FindGEWalker      = bt1::FindGEForwardWalker<Types, bt1::DefaultIteratorPrefixFn>;

    template <typename Types>
    using SkipForwardWalker     = bt1::SkipForwardWalker<Types, bt1::DefaultIteratorPrefixFn>;

    template <typename Types>
    using SkipBackwardWalker    = bt1::SkipBackwardWalker<Types, bt1::DefaultIteratorPrefixFn>;

//    template <typename Types>
//    using NextLeafWalker    = bt::NextLeafWalker<Types>;

    template <typename Types>
    using FindBeginWalker   = bt1::FindBeginWalker<Types>;

    template <typename Types>
    using FindEndWalker     = map::FindEndWalker<Types>;

    template <typename Types>
    using FindRBeginWalker  = map::FindRBeginWalker<Types>;

    template <typename Types>
    using FindREndWalker    = map::FindREndWalker<Types>;
};
*/

}

#endif
