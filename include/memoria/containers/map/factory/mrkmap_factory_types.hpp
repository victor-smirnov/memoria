
// Copyright Victor Smirnov 2011-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_MRKMAP_FACTORY_TYPES_HPP
#define _MEMORIA_CONTAINERS_MRKMAP_FACTORY_TYPES_HPP

#include <memoria/prototypes/bt/bt_factory.hpp>
#include <memoria/prototypes/ctr_wrapper/ctrwrapper_factory.hpp>

#include <memoria/containers/map/map_walkers.hpp>
#include <memoria/containers/map/map_tools.hpp>

#include <memoria/containers/map/container/map_c_insert.hpp>
#include <memoria/containers/map/container/map_c_remove.hpp>
#include <memoria/containers/map/container/map_c_api.hpp>

#include <memoria/containers/map/map_iterator.hpp>
#include <memoria/containers/map/iterator/map_i_api.hpp>
#include <memoria/containers/map/iterator/map_i_nav.hpp>
#include <memoria/containers/map/iterator/mrkmap_i_value.hpp>

#include <memoria/containers/map/map_names.hpp>

#include <memoria/core/packed/map/packed_fse_mark_map.hpp>

namespace memoria    {

template <typename Types, Int StreamIdx>
struct MarkableMapTF {

    typedef typename Types::Key                                                 Key;
    typedef typename Types::Value                                               Value;

    typedef typename SelectByIndexTool<
            StreamIdx,
            typename Types::StreamDescriptors
    >::Result                                                                   Descriptor;

    typedef PackedFSEMarkableMapTypes<
    		Key,
    		Value,
            Descriptor::LeafIndexes,
            Types::BitsPerMark
    >                                                                           MapTypes;

    typedef PackedFSEMarkableMap<MapTypes>                            			Type;
};



template <typename Profile, typename Key_, typename Value_, Int BitsPerMark_>
struct BTTypes<Profile, MrkMap<Key_, Value_, BitsPerMark_> >: public BTTypes<Profile, memoria::BT> {

    typedef BTTypes<Profile, memoria::BT>                                       Base;

    typedef Key_                                                              	Key;
    typedef MarkedValue<Value_>                                                 Value;

    static const Int BitsPerMark												= BitsPerMark_;

    typedef TypeList<Key_>                                                      KeysList;



    typedef TypeList<
                LeafNodeTypes<LeafNode>,
                NonLeafNodeTypes<BranchNode>
    >                                                                           NodeTypesList;

    typedef TypeList<
                TreeNodeType<LeafNode>,
                TreeNodeType<BranchNode>
    >                                                                           DefaultNodeTypesList;

    typedef TypeList<
                StreamDescr<PkdFTreeTF, MarkableMapTF, 1>
    >                                                                           StreamDescriptors;

    typedef BalancedTreeMetadata<
            typename Base::ID,
            ListSize<StreamDescriptors>::Value
    >                                                                           Metadata;


    typedef typename MergeLists<
                typename Base::ContainerPartsList,

                bt::NodeComprName,
                map::CtrInsertName,
                map::CtrRemoveName,
                map::CtrApiName
    >::Result                                                                   ContainerPartsList;


    typedef typename MergeLists<
                typename Base::IteratorPartsList,

                map::ItrApiName,
                map::ItrNavName,
                map::ItrMrkValueName
    >::Result                                                                   IteratorPartsList;


    template <typename Iterator, typename Container>
    struct IteratorCacheFactory {
        typedef map::MapIteratorPrefixCache<Iterator, Container> Type;
    };



    template <typename Types>
    using FindGTWalker      = map::FindGTWalker<Types>;

    template <typename Types>
    using FindGEWalker      = map::FindGEWalker<Types>;


    template <typename Types>
    using FindBeginWalker   = map::FindBeginWalker<Types>;

    template <typename Types>
    using FindEndWalker     = map::FindEndWalker<Types>;

    template <typename Types>
    using FindRBeginWalker  = map::FindRBeginWalker<Types>;

    template <typename Types>
    using FindREndWalker    = map::FindREndWalker<Types>;
};


}

#endif
