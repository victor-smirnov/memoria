
// Copyright Victor Smirnov 2014.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_MAPX_FACTORY_HPP
#define _MEMORIA_CONTAINERS_MAPX_FACTORY_HPP

#include <memoria/containers/map/container/map_c_insert.hpp>
#include <memoria/containers/map/container/mapm_c_insert.hpp>
#include <memoria/containers/map/container/map_c_remove.hpp>
#include <memoria/containers/map/iterator/map_i_nav.hpp>
#include <memoria/containers/map/iterator/mapm_i_nav.hpp>
#include <memoria/containers/map/map_iterator.hpp>
#include <memoria/containers/map/map_names.hpp>
#include <memoria/containers/map/map_tools.hpp>

#include <memoria/prototypes/bt_ss/btss_factory.hpp>
#include <memoria/prototypes/ctr_wrapper/ctrwrapper_factory.hpp>

#include <memoria/core/packed/tree/fse/packed_fse_quick_tree.hpp>
#include <memoria/core/packed/tree/fse_max/packed_fse_max_tree.hpp>
#include <memoria/core/packed/tree/vle/packed_vle_quick_tree.hpp>
#include <memoria/core/packed/tree/vle/packed_vle_dense_tree.hpp>
#include <memoria/core/packed/tree/vle_big/packed_vle_bigmax_tree.hpp>
#include <memoria/core/packed/misc/packed_sized_struct.hpp>

#include <memoria/core/tools/bignum/bigint.hpp>

#include <memoria/core/tools/uuid.hpp>

#include <tuple>

namespace memoria {


template <
    typename Profile,
    typename Key_,
    typename Value_
>
struct MapBTTypesBase: public BTTypes<Profile, memoria::BTSingleStream> {

    using Base = BTTypes<Profile, memoria::BTSingleStream>;

    static constexpr Int Indexes = 1;

    using Key 	= Key_;
    using Value = Value_;

    using MapStreamTF = StreamTF<
        	TL<
				TL<StreamSize>,
				TL<PkdFMTreeT<Key, Indexes>>,
				TL<PkdFSQArrayT<Value>>
    		>,
    		TL<
    			TL<TL<>>, TL<TL<>>, TL<TL<>>
    		>,

		FSEBranchStructTF
    >;


    using Entry = std::tuple<Key, Value>;

    using StreamDescriptors = TypeList<MapStreamTF>;

    using Metadata = BalancedTreeMetadata<
            typename Base::ID,
            ListSize<StreamDescriptors>::Value
    >;


    using CommonContainerPartsList = MergeLists<
                typename Base::CommonContainerPartsList,
                memoria::map::CtrInsertMaxName,
                memoria::map::CtrRemoveName
    >;


    using IteratorPartsList = MergeLists<
                typename Base::IteratorPartsList,
                memoria::map::ItrNavMaxName
    >;
};



template <
    typename Profile,
    typename Value_
>
struct MapBTTypesBase<Profile, BigInteger, Value_>: public BTTypes<Profile, memoria::BTSingleStream> {

    using Base = BTTypes<Profile, memoria::BTSingleStream>;

    using Key 	= BigInteger;
    using Value = Value_;

    using MapStreamTF = StreamTF<
        	TL<
				TL<StreamSize>,
				TL<PkdVBMTreeT<Key>>,
				TL<PkdFSQArrayT<Value>>
    		>,
    		TL<
    			TL<TL<>>, TL<TL<>>, TL<TL<>>
    		>,

			VLQBranchStructTF
    >;


    using Entry = std::tuple<Key, Value>;

    using StreamDescriptors = TypeList<MapStreamTF>;

    using Metadata = BalancedTreeMetadata<
            typename Base::ID,
            ListSize<StreamDescriptors>::Value
    >;


    using CommonContainerPartsList = MergeLists<
                typename Base::CommonContainerPartsList,
                memoria::map::CtrInsertMaxName,
                memoria::map::CtrRemoveName
    >;


    using IteratorPartsList = MergeLists<
                typename Base::IteratorPartsList,
                memoria::map::ItrNavMaxName
    >;
};





template <
    typename Profile,
    typename Key_,
    typename Value_
>
struct BTTypes<Profile, memoria::Map<Key_, Value_>>: public MapBTTypesBase<Profile, Key_, Value_>{};


template <typename Profile, typename Key, typename Value, typename T>
class CtrTF<Profile, memoria::Map<Key, Value>, T>: public CtrTF<Profile, memoria::BTSingleStream, T> {
};


//template <typename Profile, typename Value, typename T>
//class CtrTF<Profile, memoria::Map<UUID, Value>, T>: public CtrTF<Profile, memoria::BTSingleStream, T>
//{
//	using Base = CtrTF<Profile, memoria::BTSingleStream, T>;
//public:
//    struct Types: Base::Types
//    {
//    	using BaseTypes = typename Base::Types;
//
//    	typedef BTCtrTypes<Types>                                               CtrTypes;
//    	typedef BTIterTypes<Types>                                              IterTypes;
//
//        typedef PageUpdateManager<CtrTypes>                                     PageUpdateMgr;
//    };
//
//
//    typedef typename Types::CtrTypes                                            CtrTypes;
//    typedef Ctr<CtrTypes>                                                       Type;
//};

}

#endif
