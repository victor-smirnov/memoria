
// Copyright Victor Smirnov 2014.
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


#include <memoria/containers/mapx/mapx_tools.hpp>
#include <memoria/containers/mapx/mapx_iterator.hpp>


#include <tuple>

namespace memoria {



template <
    typename Profile,
    Int Indexes_,
    typename Key_,
    typename Value_,
    typename LabelsList,
    typename HiddenLabelsList
>
struct MapXBTTypesBase: public BTTypes<Profile, memoria::BT> {

    typedef BTTypes<Profile, memoria::BT>                                       Base;

    typedef typename TupleBuilder<
            typename metamap::LabelTypeListBuilder<HiddenLabelsList>::Type
    >::Type                                                                     HiddenLabelsTuple;

    typedef typename TupleBuilder<
            typename metamap::LabelTypeListBuilder<LabelsList>::Type
    >::Type                                                                     LabelsTuple;


    static const Int Labels                                                     = ListSize<LabelsList>::Value;
    static const Int HiddenLabels                                               = ListSize<HiddenLabelsList>::Value;

    typedef typename IfThenElse<
                    IfTypesEqual<Value_, IDType>::Value,
                    typename Base::ID,
                    Value_
    >::Result                                                                   ValueType;

    static const Int Indexes                                                    = Indexes_;

    using StreamTF = metamap::MetaMapStreamTF<Indexes, Key_, ValueType, HiddenLabelsList, LabelsList>;

    typedef typename StreamTF::Key                                              Key;
    typedef typename StreamTF::Value                                            Value;

    typedef metamap::MetaMapEntry<
                    Indexes,
                    Key,
                    Value,
                    HiddenLabelsTuple,
                    LabelsTuple
    >                                                                           Entry;

    typedef IDataSource<Entry>                                                  DataSource;
    typedef IDataTarget<Entry>                                                  DataTarget;

    typedef std::tuple<DataSource*>                                             Source;
    typedef std::tuple<DataTarget*>                                             Target;

    typedef std::tuple <
                std::pair<float, float>
    >                                                                           Entropy;

    typedef typename bt::TupleBuilder<
        typename bt::SameTypeListBuilder<Int, 2 + Labels + HiddenLabels>::Type
    >::Type                                                                     EntrySizes;

    typedef metamap::LabelOffsetProc<LabelsList>                                LabelsOffset;
    typedef metamap::LabelOffsetProc<HiddenLabelsList>                          HiddenLabelsOffset;


    typedef TypeList<StreamTF>                                                  StreamDescriptors;

    typedef BalancedTreeMetadata<
            typename Base::ID,
            ListSize<StreamDescriptors>::Value
    >                                                                           Metadata;


    using ContainerPartsList = MergeLists<
                typename Base::ContainerPartsList,

                metamap::CtrNavName,
                metamap::CtrRemoveName,
                metamap::CtrFindName
    >;


    using IteratorPartsList = MergeLists<
                typename Base::IteratorPartsList,

                metamap::ItrKeysName,
                metamap::ItrNavName,
                metamap::ItrEntryName,
                metamap::ItrLabelsName,
                metamap::ItrFindName,
                metamap::ItrMiscName
    >;


    template <typename Iterator, typename Container>
    struct IteratorCacheFactory {
        typedef metamap::MetaMapIteratorPrefixCache<Iterator, Container> Type;
    };



    template <typename Types>
    using FindGTWalker          = bt1::FindGTForwardWalker<Types, 0, bt1::DefaultIteratorPrefixFn>;

    template <typename Types>
    using FindGEWalker          = bt1::FindGEForwardWalker<Types, 0, bt1::DefaultIteratorPrefixFn>;

    template <typename Types>
    using FindBackwardWalker    = bt1::FindBackwardWalker<Types, 0, bt1::DefaultIteratorPrefixFn>;


    template <typename Types>
    using SkipForwardWalker     = bt1::SkipForwardWalker<Types, 0, bt1::DefaultIteratorPrefixFn>;

    template <typename Types>
    using SkipBackwardWalker    = bt1::SkipBackwardWalker<Types, 0, bt1::DefaultIteratorPrefixFn>;

    template <typename Types>
    using SelectForwardWalker   = metamap::SelectForwardWalker<Types, 0, bt1::DefaultIteratorPrefixFn>;

    template <typename Types>
    using SelectBackwardWalker  = metamap::SelectBackwardWalker<Types, 0, bt1::DefaultIteratorPrefixFn>;


    template <typename Types>
    using FindBeginWalker       = bt1::FindBeginWalker<Types>;
};







template <
    typename Profile,
    typename Key_,
    typename Value_
>
struct BTTypes<Profile, memoria::MapX<Key_, Value_>>:
    public MapXBTTypesBase<Profile, 1, Key_, Value_, TL<>, TL<>> {

    using Base = MapXBTTypesBase<Profile, 1, Key_, Value_, TL<>, TL<>>;


    using ContainerPartsList = MergeLists<
                    typename Base::ContainerPartsList,
                    bt::NodeNormName,
                    metamap::CtrInsertName,
                    metamap::CtrInsBatchName
    >;


    using IteratorPartsList =MergeLists<
                    typename Base::IteratorPartsList,
                    metamap::ItrValueByRefName
    >;
};





}

#endif
