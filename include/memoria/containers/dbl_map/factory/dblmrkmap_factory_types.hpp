
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINERS_DBLMRKMAP_FACTORY_TYPES_HPP
#define _MEMORIA_CONTAINERS_DBLMRKMAP_FACTORY_TYPES_HPP

#include <memoria/containers/dbl_map/dblmap_walkers.hpp>
#include <memoria/containers/dbl_map/dblmap_tools.hpp>
#include <memoria/containers/dbl_map/dblmap_names.hpp>

#include <memoria/containers/dbl_map/container/dblmap_c_api.hpp>

#include <memoria/containers/vector_map/container/vmap_c_tools.hpp>
#include <memoria/containers/vector_map/container/vmap_c_insert.hpp>
#include <memoria/containers/vector_map/container/vmap_c_remove.hpp>
#include <memoria/containers/vector_map/container/vmap_c_update.hpp>


#include <memoria/containers/dbl_map/dblmap_iterator.hpp>
#include <memoria/containers/dbl_map/iterator/dblmap_i_crud.hpp>
#include <memoria/containers/vector_map/iterator/vmap_i_seek.hpp>

#include <memoria/containers/dbl_map/dblmap_names.hpp>
#include <memoria/containers/vector_map/vmap_names.hpp>

namespace memoria 	{


template <typename Profile, typename Key_, typename Value_, Int BitsPerMark_>
struct BTTypes<Profile, DblMrkMap<Key_, Value_, BitsPerMark_> >:
    public BTTypes<Profile, memoria::BT>
{

    typedef BTTypes<Profile, memoria::BT>                   					Base;

    typedef Key_																Key;
    typedef MarkedValue<Value_>                                                 Value;

    static const Int BitsPerMark												= BitsPerMark_;


    template <typename Iterator, typename Container>
    struct IteratorCacheFactory {
        typedef dblmap::DblMapIteratorPrefixCache<Iterator, Container>	Type;
    };

    typedef TypeList<
            NonLeafNodeTypes<BranchNode>,
            LeafNodeTypes<LeafNode>
    >                                                                           NodeTypesList;

    typedef TypeList<
            TreeNodeType<LeafNode>,
            TreeNodeType<BranchNode>
    >                                                                           DefaultNodeTypesList;

    typedef TypeList<
                // Outer Map
                StreamDescr<
                    PkdFTreeTF,
                    dblmap::PackedOuterMapLeafTF,
                    2 // node & leaf indexes
                >,

                // Inner Map
                StreamDescr<
                    PkdFTreeTF,
                    dblmap::PackedInnerMarkedMapLeafTF,
                    2, // node indexes
                    1  // leaf indexes
                >
    >                                                                           StreamDescriptors;

    typedef BalancedTreeMetadata<
                typename Base::ID,
                ListSize<StreamDescriptors>::Value
    >                                                                           Metadata;


    typedef typename MergeLists<
            typename Base::ContainerPartsList,
            memoria::bt::NodeNormName,

            memoria::vmap::CtrToolsName,
            memoria::vmap::CtrInsertName,
            memoria::vmap::CtrRemoveName,
            memoria::vmap::CtrUpdateName,

            memoria::dblmap::CtrApiName
    >::Result                                                                   ContainerPartsList;

    typedef typename MergeLists<
            typename Base::IteratorPartsList,

            memoria::vmap::ItrSeekName,

            memoria::dblmap::ItrCRUDName
    >::Result                                                                   IteratorPartsList;

    typedef std::pair<StaticVector<Key, 1>, Value>								IOValue;

    typedef IDataSource<IOValue>                                                DataSource;
    typedef IDataTarget<IOValue>                                                DataTarget;





    template <typename Types>
    using FindLEWalker              = dblmap::SecondMapFindWalker<Types>;

    template <typename Types>
    using SkipForwardWalker         = dblmap::SkipForwardWalker<Types>;

    template <typename Types>
    using SkipBackwardWalker        = dblmap::SkipBackwardWalker<Types>;


    template <typename Types>
    using NextLeafWalker            = bt::NextLeafWalker<Types>;

    template <typename Types>
    using PrevLeafWalker            = dblmap::PrevLeafWalker<Types>;

    template <typename Types>
    using NextLeafMutistreamWalker  = bt::NextLeafMultistreamWalker<Types>;


    template <typename Types>
    using PrevLeafMutistreamWalker  = bt::PrevLeafMultistreamWalker<Types>;



    template <typename Types>
    using FindBeginWalker           = vmap::FindVMapBeginWalker<Types>;

    template <typename Types>
    using FindEndWalker             = vmap::FindVMapEndWalker<Types>;

    template <typename Types>
    using FindRBeginWalker          = vmap::FindVMapRBeginWalker<Types>;

    template <typename Types>
    using FindREndWalker            = vmap::FindVMapREndWalker<Types>;
};


}

#endif
