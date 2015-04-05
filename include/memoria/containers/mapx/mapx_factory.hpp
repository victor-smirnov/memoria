
// Copyright Victor Smirnov 2014-2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_MAPX_FACTORY_HPP
#define _MEMORIA_CONTAINERS_MAPX_FACTORY_HPP

#include <memoria/core/tools/idata.hpp>

#include <memoria/prototypes/bt/bt_factory.hpp>
#include <memoria/prototypes/ctr_wrapper/ctrwrapper_factory.hpp>

#include <memoria/prototypes/bt/walkers/bt_skip_walkers.hpp>
#include <memoria/prototypes/bt/walkers/bt_find_walkers.hpp>
#include <memoria/prototypes/bt/walkers/bt_edge_walkers.hpp>
#include <memoria/prototypes/bt/walkers/bt_select_walkers.hpp>
#include <memoria/prototypes/bt/walkers/bt_rank_walkers.hpp>

#include <memoria/prototypes/metamap/walkers/metamap_rank_walkers.hpp>
#include <memoria/prototypes/metamap/walkers/metamap_select_walkers.hpp>


#include <memoria/prototypes/bt/packed_adaptors/bt_tree_adaptor.hpp>
#include <memoria/prototypes/metamap/packed_adaptors/metamap_packed_adaptors.hpp>

#include <memoria/prototypes/metamap/metamap_tools.hpp>

#include <memoria/prototypes/metamap/container/metamap_c_insert.hpp>
#include <memoria/prototypes/metamap/container/metamap_c_insert_compr.hpp>
#include <memoria/prototypes/metamap/container/metamap_c_nav.hpp>
#include <memoria/prototypes/metamap/container/metamap_c_remove.hpp>
#include <memoria/prototypes/metamap/container/metamap_c_find.hpp>
#include <memoria/prototypes/metamap/container/metamap_c_insbatch.hpp>
#include <memoria/prototypes/metamap/container/metamap_c_insbatch_compr.hpp>

#include <memoria/prototypes/metamap/metamap_iterator.hpp>
#include <memoria/prototypes/metamap/iterator/metamap_i_keys.hpp>
#include <memoria/prototypes/metamap/iterator/metamap_i_nav.hpp>
#include <memoria/prototypes/metamap/iterator/metamap_i_entry.hpp>
#include <memoria/prototypes/metamap/iterator/metamap_i_value.hpp>
#include <memoria/prototypes/metamap/iterator/metamap_i_value_byref.hpp>
#include <memoria/prototypes/metamap/iterator/metamap_i_labels.hpp>
#include <memoria/prototypes/metamap/iterator/metamap_i_find.hpp>
#include <memoria/prototypes/metamap/iterator/metamap_i_misc.hpp>

#include <memoria/prototypes/metamap/metamap_names.hpp>

#include <memoria/containers/mapx/container/mapx_c_insert.hpp>
#include <memoria/containers/mapx/container/mapx_c_remove.hpp>
#include <memoria/containers/mapx/iterator/mapx_i_nav.hpp>

#include <memoria/containers/mapx/mapx_tools.hpp>
#include <memoria/containers/mapx/mapx_iterator.hpp>


#include <tuple>

namespace memoria {



template <
    typename Profile,
    Int Indexes_,
    typename Key_,
    typename Value_
>
struct MapXBTTypesBase: public BTTypes<Profile, memoria::BT> {

    typedef BTTypes<Profile, memoria::BT>                                       Base;

    using HiddenLabelsTuple = std::tuple<>;

    using LabelsTuple = std::tuple<>;

    static const Int Labels                                                     = 0;
    static const Int HiddenLabels                                               = 0;

    typedef typename IfThenElse<
                    IfTypesEqual<Value_, IDType>::Value,
                    typename Base::ID,
                    Value_
    >::Result                                                                   ValueType;

    static const Int Indexes                                                    = Indexes_;

    using StreamTF = mapx::MapXStreamTF<Indexes, Key_, ValueType>;

    typedef typename StreamTF::Key                                              Key;
    typedef typename StreamTF::Value                                            Value;

    typedef std::tuple<Key, Value>                                              Entry;

    typedef IDataSource<Entry>                                                  DataSource;
    typedef IDataTarget<Entry>                                                  DataTarget;

    typedef std::tuple<DataSource*>                                             Source;
    typedef std::tuple<DataTarget*>                                             Target;

    typedef TypeList<StreamTF>                              					StreamDescriptors;

    typedef BalancedTreeMetadata<
            typename Base::ID,
            ListSize<StreamDescriptors>::Value
    >                                                                           Metadata;


    using ContainerPartsList = MergeLists<
                typename Base::ContainerPartsList,
                memoria::mapx::CtrInsertName,
                memoria::mapx::CtrRemoveName
    >;


    using IteratorPartsList = MergeLists<
                typename Base::IteratorPartsList,
                memoria::mapx::ItrNavName
    >;


    template <typename Iterator, typename Container>
    struct IteratorCacheFactory {
        typedef mapx::MapXIteratorPrefixCache<Iterator, Container> Type;
    };



    template <typename Types, typename LeafPath>
    using FindGTWalker          = bt1::FindGTForwardWalker2<WalkerTypes<Types, LeafPath>>;

    template <typename Types, typename LeafPath>
    using FindGEWalker          = bt1::FindGEForwardWalker2<WalkerTypes<Types, LeafPath>>;

    template <typename Types, typename LeafPath>
    using FindBackwardWalker    = bt1::FindBackwardWalker2<WalkerTypes<Types, LeafPath>>;


    template <typename Types, typename LeafPath>
    using SkipForwardWalker     = bt1::SkipForwardWalker2<WalkerTypes<Types, LeafPath>>;

    template <typename Types, typename LeafPath>
    using SkipBackwardWalker    = bt1::SkipBackwardWalker2<WalkerTypes<Types, LeafPath>>;

    template <typename Types, typename LeafPath>
    using SelectForwardWalker   = bt1::SelectForwardWalker2<WalkerTypes<Types, LeafPath>>;

    template <typename Types, typename LeafPath>
    using SelectBackwardWalker  = bt1::SelectBackwardWalker2<WalkerTypes<Types, LeafPath>>;


    template <typename Types>
    using FindBeginWalker       = bt1::FindBeginWalker<Types>;
};







template <
    typename Profile,
    typename Key_,
    typename Value_
>
struct BTTypes<Profile, memoria::MapX<Key_, Value_>>:
    public MapXBTTypesBase<Profile, 1, Key_, Value_>
{

    using Base = MapXBTTypesBase<Profile, 1, Key_, Value_>;


    using ContainerPartsList = MergeLists<
                    typename Base::ContainerPartsList,
                    bt::NodeNormName,
                    mapx::CtrInsertName
    >;

    using IteratorPartsList =MergeLists<
                    typename Base::IteratorPartsList
    >;
};





}

#endif
