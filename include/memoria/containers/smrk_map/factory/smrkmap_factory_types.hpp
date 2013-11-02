
// Copyright Victor Smirnov 2011-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_SMRKMAP_FACTORY_TYPES_HPP
#define _MEMORIA_CONTAINERS_SMRKMAP_FACTORY_TYPES_HPP

#include <memoria/prototypes/bt/bt_factory.hpp>
#include <memoria/prototypes/ctr_wrapper/ctrwrapper_factory.hpp>

#include <memoria/containers/smrk_map/smrkmap_walkers.hpp>
#include <memoria/containers/smrk_map/smrkmap_tools.hpp>

#include <memoria/containers/map/container/map_c_remove.hpp>
#include <memoria/containers/smrk_map/container/smrkmap_c_insert.hpp>
#include <memoria/containers/smrk_map/container/smrkmap_c_api.hpp>

#include <memoria/containers/smrk_map/smrkmap_iterator.hpp>

#include <memoria/containers/smrk_map/iterator/smrkmap_i_api.hpp>
#include <memoria/containers/smrk_map/iterator/smrkmap_i_nav.hpp>

#include <memoria/containers/map/iterator/map_i_nav.hpp>

#include <memoria/containers/map/map_names.hpp>
#include <memoria/containers/smrk_map/smrkmap_names.hpp>

#include <memoria/core/packed/map/packed_fse_smark_map.hpp>

namespace memoria {


template <typename Types, Int StreamIdx>
struct SearchableMarkableMapTF {

    typedef typename Types::Key                                                 Key;
    typedef typename Types::Value                                               Value;

    typedef typename SelectByIndexTool<
            StreamIdx,
            typename Types::StreamDescriptors
    >::Result                                                                   Descriptor;

    typedef PackedFSESearchableMarkableMapTypes<
    		Key,
    		Value,
            Descriptor::LeafIndexes,
            Types::BitsPerMark
    >                                                                           MapTypes;

    typedef PackedFSESearchableMarkableMap<MapTypes>                            Type;
};



template <typename Profile, typename Key_, typename Value_, Int BitsPerMark_>
struct BTTypes<Profile, SMrkMap<Key_, Value_, BitsPerMark_> >: public BTTypes<Profile, memoria::BT> {

    typedef BTTypes<Profile, memoria::BT>                                       Base;

    typedef Key_                                                              	Key;
    typedef Value_                                                              Value;

    static const Int BitsPerMark												= BitsPerMark_;


    typedef TypeList<BigInt>                                                    KeysList;


    typedef TypeList<
            LeafNodeTypes<LeafNode>,
            NonLeafNodeTypes<BranchNode>
    >                                                                           NodeTypesList;

    typedef TypeList<
            TreeNodeType<LeafNode>,
            TreeNodeType<BranchNode>
    >                                                                           DefaultNodeTypesList;

    typedef TypeList<
            StreamDescr<
                PkdFTreeTF,
                SearchableMarkableMapTF,
                (1 << BitsPerMark) + 2,
                1
            >
    >                                                                           StreamDescriptors;

    typedef BalancedTreeMetadata<
            typename Base::ID,
            ListSize<StreamDescriptors>::Value
    >                                                                           Metadata;


    typedef typename MergeLists<
                typename Base::ContainerPartsList,
                bt::NodeComprName,
                smrk_map::CtrInsertName,
                map::CtrRemoveName,
                smrk_map::CtrApiName
    >::Result                                                                   ContainerPartsList;


    typedef typename MergeLists<
                typename Base::IteratorPartsList,
                map::ItrNavName,
                smrk_map::ItrApiName,
                smrk_map::ItrNavName
    >::Result                                                                   IteratorPartsList;


    template <typename Iterator, typename Container>
    struct IteratorCacheFactory {
        typedef smrk_map::MapIteratorPrefixCache<Iterator, Container> Type;
    };



    template <typename Types>
    using FindLTWalker      = smrk_map::FindLTWalker<Types>;

    template <typename Types>
    using FindLEWalker      = smrk_map::FindLEWalker<Types>;


    template <typename Types>
    using SelectFwWalker    = smrk_map::SelectForwardWalker<Types>;

    template <typename Types>
    using FindBeginWalker   = smrk_map::FindBeginWalker<Types>;

    template <typename Types>
    using FindEndWalker     = smrk_map::FindEndWalker<Types>;

    template <typename Types>
    using FindRBeginWalker  = smrk_map::FindRBeginWalker<Types>;

    template <typename Types>
    using FindREndWalker    = smrk_map::FindREndWalker<Types>;
};


}

#endif
