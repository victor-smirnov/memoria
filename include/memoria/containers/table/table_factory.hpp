
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_TABLE_FACTORY_HPP
#define _MEMORIA_CONTAINERS_TABLE_FACTORY_HPP

#include <memoria/prototypes/bt_tl/bttl_factory.hpp>
#include <memoria/prototypes/ctr_wrapper/ctrwrapper_factory.hpp>

#include <memoria/containers/table/table_names.hpp>

#include <memoria/containers/table/container/table_c_api.hpp>
#include <memoria/containers/table/iterator/table_i_misc.hpp>

#include <memoria/containers/table/table_tools.hpp>
#include <memoria/containers/table/table_iterator.hpp>

#include <memoria/prototypes/bt/layouts/bt_input.hpp>

#include <memoria/core/tools/i7_codec.hpp>

#include <tuple>

namespace memoria {



template <
    typename Profile,
    typename Key_,
    typename Value_
>
struct TableBTTypesBase: public BTTypes<Profile, memoria::BTTreeLayout> {

    using Base = BTTypes<Profile, memoria::BTTreeLayout>;

    using ValueType = typename IfThenElse<
                    IfTypesEqual<Value_, IDType>::Value,
                    typename Base::ID,
                    Value_
    >::Result;


    using Key 	= Key_;
    using Value	= Value_;

    using CtrSizeT = BigInt;

    template <Int Indexes>
    using Stream1TF = StreamTF<
    		TL<TL<
				PkdVTree<Packed2TreeTypes<Key, Key, Indexes, UByteI7Codec>>
//    			PkdVTree<Packed2TreeTypes<CtrSizeT, CtrSizeT, 1, UByteI7Codec>>
    		>>,
			TL<TL<TL<IndexRange<0, Indexes>>>>,
			FSEBranchStructTF
    >;



    using Stream2TF = StreamTF<
    		TL<>,
			TL<>,
			FSEBranchStructTF
    >;



    using DataStreamTF = StreamTF<
    		TL<TL<PackedFSEArray<PackedFSEArrayTypes<Value>>>>,
			TL<TL<TL<>>>,
			FSEBranchStructTF
    >;

    using StreamDescriptors = typename bttl::BTTLAugmentStreamDescriptors<
    		TypeList<
				Stream1TF<1>,
				Stream2TF,
				DataStreamTF
			>
	>::Type;

    using Metadata = BalancedTreeMetadata<
            typename Base::ID,
            ListSize<StreamDescriptors>::Value
    >;


    using CommonContainerPartsList = MergeLists<
                typename Base::CommonContainerPartsList,
                memoria::table::CtrApiName
    >;

    using IteratorPartsList = MergeLists<
                typename Base::IteratorPartsList,
                memoria::table::ItrMiscName
    >;
};







template <
    typename Profile,
    typename Key_,
    typename Value_
>
struct BTTypes<Profile, memoria::Table<Key_, Value_>>: public TableBTTypesBase<Profile, Key_, Value_>
{
};


template <typename Profile, typename Key, typename Value, typename T>
class CtrTF<Profile, memoria::Table<Key, Value>, T>: public CtrTF<Profile, memoria::BTTreeLayout, T> {
    using Base = CtrTF<Profile, memoria::BTTreeLayout, T>;
public:

    struct Types: Base::Types
    {
    	using CtrTypes 			= TableCtrTypes<Types>;
        using IterTypes 		= TableIterTypes<Types>;

        using PageUpdateMgr 	= PageUpdateManager<CtrTypes>;
    };

    using CtrTypes 	= typename Types::CtrTypes;
    using Type 		= Ctr<CtrTypes>;
};


}

#endif
