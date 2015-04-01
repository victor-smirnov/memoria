
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINERS_VECTORMAP_FACTORY_HPP
#define _MEMORIA_CONTAINERS_VECTORMAP_FACTORY_HPP

#include <memoria/containers/vector_map/vmap_walkers.hpp>
#include <memoria/containers/vector_map/vmap_tools.hpp>
#include <memoria/containers/vector_map/vmap_names.hpp>


#include <memoria/containers/vector_map/container/vmap_c_tools.hpp>
#include <memoria/containers/vector_map/container/vmap_c_insert.hpp>
#include <memoria/containers/vector_map/container/vmap_c_remove.hpp>
#include <memoria/containers/vector_map/container/vmap_c_api.hpp>
#include <memoria/containers/vector_map/container/vmap_c_find.hpp>
#include <memoria/containers/vector_map/container/vmap_c_update.hpp>

#include <memoria/containers/vector_map/vmap_iterator.hpp>
#include <memoria/containers/vector_map/iterator/vmap_i_crud.hpp>
#include <memoria/containers/vector_map/iterator/vmap_i_seek.hpp>

#include <memoria/containers/vector_map/vmap_names.hpp>

namespace memoria    {




template <typename Profile, typename Key_, typename Value_>
struct BTTypes<Profile, memoria::VectorMap<Key_, Value_> >:
    public BTTypes<Profile, memoria::BT>
{

    typedef BTTypes<Profile, memoria::BT>                                       Base;

    typedef Value_                                                              Value;
    typedef Key_                                                                Key;


    template <typename Iterator, typename Container>
    struct IteratorCacheFactory {
        typedef vmap::VectorMapIteratorPrefixCache<Iterator, Container>         Type;
    };

    typedef TypeList<
            BranchNodeTypes<BranchNode>,
            LeafNodeTypes<LeafNode>
    >                                                                           NodeTypesList;

    typedef TypeList<
            TreeNodeType<LeafNode>,
            TreeNodeType<BranchNode>
    >                                                                           DefaultNodeTypesList;



    struct MapStreamTF {
        typedef Key_                                                Key;
        typedef BigInt                                              Value;

        typedef core::StaticVector<BigInt, 2>                       AccumulatorPart;
        typedef core::StaticVector<BigInt, 2>                       IteratorPrefixPart;

        typedef PkdFTree<Packed2TreeTypes<Key, Key, 2>>             NonLeafType;
        typedef PkdFTree<Packed2TreeTypes<Key, Key, 2>>             LeafType;
        typedef TL<TL<>>											IdxRangeList;
    };



    struct DataStreamTF {
        typedef BigInt                                              Key;
        typedef Value_                                              Value;

        typedef core::StaticVector<BigInt, 1>                       AccumulatorPart;
        typedef core::StaticVector<BigInt, 1>                       IteratorPrefixPart;

        typedef PkdFTree<Packed2TreeTypes<Key, Key, 1>>             NonLeafType;
        typedef PackedFSEArray<PackedFSEArrayTypes<Value>>          LeafType;
        typedef TL<TL<>>											IdxRangeList;
    };


    typedef TypeList<
                // Map
                MapStreamTF
                ,
                // Vector
                DataStreamTF
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
            memoria::vmap::CtrFindName,
            memoria::vmap::CtrApiName
    >::Result                                                                   ContainerPartsList;

    typedef typename MergeLists<
            typename Base::IteratorPartsList,

            memoria::vmap::ItrCRUDName,
            memoria::vmap::ItrSeekName
    >::Result                                                                   IteratorPartsList;


    typedef Value                                                               IOValue;

    typedef IDataSource<IOValue>                                                DataSource;
    typedef IDataTarget<IOValue>                                                DataTarget;



//    template <typename Types>
//    using FindGTWalker            = ::memoria::vmap::FindLTForwardWalker<Types>;
//
//    template <typename Types>
//    using FindGEWalker            = ::memoria::vmap::FindLTForwardWalker<Types>;

    template <typename Types>
    using SkipForwardWalker         = vmap::SkipForwardWalker<Types, 0>;

    template <typename Types>
    using SkipBackwardWalker        = vmap::SkipBackwardWalker<Types, 0>;


    template <typename Types>
    using NextLeafWalker            = bt::NextLeafWalker<Types, 0>;

    template <typename Types>
    using PrevLeafWalker            = vmap::PrevLeafWalker<Types, 0>;

    template <typename Types>
    using NextLeafMutistreamWalker  = bt::NextLeafMultistreamWalker<Types, 0>;


    template <typename Types>
    using PrevLeafMutistreamWalker  = bt::PrevLeafMultistreamWalker<Types, 0>;



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
class CtrTF<Profile, memoria::VectorMap<Key, Value>, T>: public CtrTF<Profile, memoria::BT, T> {
};




}

#endif
