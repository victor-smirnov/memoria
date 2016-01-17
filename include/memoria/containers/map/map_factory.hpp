
// Copyright Victor Smirnov 2014+.
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
#include <memoria/core/packed/misc/packed_sized_struct.hpp>

#include <tuple>

namespace memoria {



template <
    typename Profile,
    Int Indexes_,
    typename Key_,
    typename Value_
>
struct MapBTTypesBase: public BTTypes<Profile, memoria::BTSingleStream> {

    typedef BTTypes<Profile, memoria::BTSingleStream>                           Base;

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
			PkdFQTreeT<Key_, Indexes>,
			PackedFSEArray<PackedFSEArrayTypes<ValueType>>
		>>,
		TL<TL<TL<SumRange<0, Indexes>>, TL<>>>,
		FSEBranchStructTF
    >;

    typedef Key_                                              					Key;
    typedef Value_                                            					Value;

    typedef std::tuple<Key, Value>                                              Entry;

    typedef TypeList<MapStreamTF>                              					StreamDescriptors;

    typedef BalancedTreeMetadata<
            typename Base::ID,
            ListSize<StreamDescriptors>::Value
    >                                                                           Metadata;


    using CommonContainerPartsList = MergeLists<
                typename Base::CommonContainerPartsList,
                memoria::map::CtrInsertName,
                memoria::map::CtrRemoveName
    >;


    using IteratorPartsList = MergeLists<
                typename Base::IteratorPartsList,
                memoria::map::ItrNavName
    >;
};




template <
    typename Profile,
    Int Indexes_,
    typename Value_
>
struct MapBTTypesBase<Profile, Indexes_, double, Value_>: public BTTypes<Profile, memoria::BTSingleStream> {

    typedef BTTypes<Profile, memoria::BTSingleStream>                           Base;

    static const Int Labels                                                     = 0;
    static const Int HiddenLabels                                               = 0;

    typedef typename IfThenElse<
                    IfTypesEqual<Value_, IDType>::Value,
                    typename Base::ID,
                    Value_
    >::Result                                                                   ValueType;

    static const Int Indexes                                                    = Indexes_;


    typedef double                                              				Key;
    typedef Value_                                            					Value;

    using MapStreamTF = StreamTF<
//    	TL<
//			TL<
//				PackedSizedStruct,
//				TL<
//					PkdFMTreeT<Key, Indexes>
//				>,
//				PackedFSEArray<PackedFSEArrayTypes<ValueType>>
//			>
//		>,
//		TL<
//			TL<
//				TL<>, TL<TL<>>, TL<>
//    		>
//		>,


        	TL<
					PackedFSEArray<PackedFSEArrayTypes<ValueType>>,
    				TL<PkdFMTreeT<Key, Indexes>>
    		>,
    		TL<
    				TL<>, TL<TL<>>
    		>,

		FSEBranchStructTF
    >;


    typedef std::tuple<Key, Value>                                              Entry;

    typedef TypeList<MapStreamTF>                              					StreamDescriptors;

    typedef BalancedTreeMetadata<
            typename Base::ID,
            ListSize<StreamDescriptors>::Value
    >                                                                           Metadata;


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
struct BTTypes<Profile, memoria::Map<Key_, Value_>>:
    public MapBTTypesBase<Profile, 1, Key_, Value_>
{

    using Base = MapBTTypesBase<Profile, 1, Key_, Value_>;


    using ContainerPartsList = MergeLists<
                    typename Base::ContainerPartsList,
                    memoria::map::CtrInsertName
    >;

    using IteratorPartsList = MergeLists<
                    typename Base::IteratorPartsList
    >;
};


template <typename Profile, typename Key, typename Value, typename T>
class CtrTF<Profile, memoria::Map<Key, Value>, T>: public CtrTF<Profile, memoria::BTSingleStream, T> {
};


template <typename Profile, typename Value, typename T>
class CtrTF<Profile, memoria::Map<double, Value>, T>: public CtrTF<Profile, memoria::BTSingleStream, T>
{
	using Base = CtrTF<Profile, memoria::BTSingleStream, T>;
public:
    struct Types: Base::Types
    {
    	using BaseTypes = typename Base::Types;

    	using LeafStreamsStructList 	= FailIf<typename BaseTypes::LeafStreamsStructList, false>;
//    	using StreamsInputTypeList 		= FailIf<typename BaseTypes::StreamsInputTypeList>;
//    	using InputBufferStructList		= FailIf<typename BaseTypes::InputBufferStructList>;
//    	using BranchStreamsStructList	= FailIf<typename BaseTypes::BranchStreamsStructList>;

//    	using Accumulator				= FailIf<typename BaseTypes::Accumulator>;

//    	using IteratorAccumulator		= FailIf<typename BaseTypes::IteratorAccumulator>;

//    	static constexpr Int Value1 = LeafToBranchIndexByValueTranslator1<LeafStreamsStructList, 1>::LeafOffset;

//    	using Boo = FailIf<IntValue<Value1>>;

    	using Boo = EmptyType;


    	typedef BTCtrTypes<Types>                                               CtrTypes;
    	typedef BTIterTypes<Types>                                              IterTypes;

        typedef PageUpdateManager<CtrTypes>                                     PageUpdateMgr;
    };


    typedef typename Types::CtrTypes                                            CtrTypes;
    typedef Ctr<CtrTypes>                                                       Type;
};

}

#endif
