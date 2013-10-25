
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINERS_DBLMAP_FACTORY_HPP
#define _MEMORIA_CONTAINERS_DBLMAP_FACTORY_HPP

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

namespace dblmap	{

template <typename Types, Int StreamIdx>
struct PackedOuterMapLeafTF {

    typedef typename Types::Key                                                 Key;

    typedef typename SelectByIndexTool<
            StreamIdx,
            typename Types::StreamDescriptors
    >::Result                                                                   Descriptor;

    typedef Packed2TreeTypes<
            Key, Key, Descriptor::LeafIndexes
    >                                                                           TreeTypes;

    typedef PkdFTree<TreeTypes> 												Type;
};

template <typename Types, Int StreamIdx>
struct PackedInnerMapLeafTF {

    typedef typename Types::Key                                                 Key;
    typedef typename Types::Value                                               Value;

    typedef typename SelectByIndexTool<
            StreamIdx,
            typename Types::StreamDescriptors
    >::Result                                                                   Descriptor;

    typedef PackedFSEMapTypes<
            Key, Value, Descriptor::LeafIndexes
    >                                                                           MapTypes;

    typedef PackedFSEMap<MapTypes> 												Type;
};


}


template <typename Profile, typename Key_, typename Value_>
struct BTTypes<Profile, memoria::Map<Key_, memoria::Map<Key_, Value_>> >:
    public BTTypes<Profile, memoria::BT>
{

    typedef BTTypes<Profile, memoria::BT>                   					Base;

    typedef Value_                                                              Value;
    typedef TypeList<BigInt>                                                    KeysList;

    template <typename Iterator, typename Container>
    struct IteratorCacheFactory {
        typedef dblmap::DblMapIteratorPrefixCache<Iterator, Container>       Type;
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
                    dblmap::PackedInnerMapLeafTF,
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

    typedef std::pair<StaticVector<BigInt, 1>, BigInt>								IOValue;

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


template <typename Profile, typename Key, typename Value, typename T>
class CtrTF<Profile, memoria::Map<Key, Map<Key, Value>>, T>: public CtrTF<Profile, memoria::BT, T> {
};




}

#endif
