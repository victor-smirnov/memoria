
// Copyright Victor Smirnov 2014+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_MAPX_FACTORY_HPP
#define _MEMORIA_CONTAINERS_MAPX_FACTORY_HPP

#include <memoria/core/tools/idata.hpp>

#include <memoria/prototypes/bt_ss/btss_factory.hpp>
#include <memoria/prototypes/ctr_wrapper/ctrwrapper_factory.hpp>


#include <memoria/containers/mapx/mapx_names.hpp>

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
struct MapXBTTypesBase: public BTTypes<Profile, memoria::BTSingleStream> {

    typedef BTTypes<Profile, memoria::BTSingleStream>                           Base;

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

    using MapStreamTF = StreamTF<
    	TL<TL<
			PkdFTree<Packed2TreeTypes<Key_, Key_, Indexes>>, //UByteExintCodec
			PackedFSEArray<PackedFSEArrayTypes<ValueType>>
		>>,
		TL<TL<TL<IndexRange<0, Indexes>>, TL<>>>,
		FSEBranchStructTF
    >;

    typedef Key_                                              					Key;
    typedef Value_                                            					Value;

    typedef std::tuple<Key, Value>                                              Entry;

    typedef IDataSource<Entry>                                                  DataSource;
    typedef IDataTarget<Entry>                                                  DataTarget;

    typedef std::tuple<DataSource*>                                             Source;
    typedef std::tuple<DataTarget*>                                             Target;

    typedef TypeList<MapStreamTF>                              					StreamDescriptors;

    typedef BalancedTreeMetadata<
            typename Base::ID,
            ListSize<StreamDescriptors>::Value
    >                                                                           Metadata;


    using CommonContainerPartsList = MergeLists<
                typename Base::CommonContainerPartsList,
                memoria::mapx::CtrInsertName,
                memoria::mapx::CtrRemoveName
    >;


    using IteratorPartsList = MergeLists<
                typename Base::IteratorPartsList,
                memoria::mapx::ItrNavName
    >;


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
                    mapx::CtrInsertName
    >;

    using IteratorPartsList = MergeLists<
                    typename Base::IteratorPartsList
    >;
};


template <typename Profile, typename Key, typename Value, typename T>
class CtrTF<Profile, memoria::MapX<Key, Value>, T>: public CtrTF<Profile, memoria::BTSingleStream, T> {
};


}

#endif
