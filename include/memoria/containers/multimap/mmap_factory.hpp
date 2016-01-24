
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_MULTIMAP_FACTORY_HPP
#define _MEMORIA_CONTAINERS_MULTIMAP_FACTORY_HPP

#include <memoria/prototypes/bt_tl/bttl_factory.hpp>
#include <memoria/containers/multimap/mmap_names.hpp>

#include <memoria/containers/multimap/container/mmap_c_api.hpp>
#include <memoria/containers/multimap/iterator/mmap_i_misc.hpp>

#include <memoria/containers/multimap/mmap_tools.hpp>
#include <memoria/containers/multimap/mmap_iterator.hpp>

#include <memoria/prototypes/bt/layouts/bt_input.hpp>

#include <tuple>

namespace memoria {



template <
    typename Profile,
    typename Key_,
    typename Value_
>
struct MultimapBTTypesBase: public BTTypes<Profile, memoria::BTTreeLayout> {

    using Base = BTTypes<Profile, memoria::BTTreeLayout>;

    using ValueType = IfThenElse<
                    IfTypesEqual<Value_, IDType>::Value,
                    typename Base::ID,
                    Value_
    >;

    static constexpr PackedSizeType SizeType = PackedSizeType::FIXED;

    using Key 	= Key_;
    using Value	= Value_;

    using CtrSizeT = BigInt;


    using FirstStreamVariableTF = StreamTF<
    	TL<
			//PkdVTree<Packed2TreeTypes<CtrSizeT, CtrSizeT, 1, UByteI7Codec>>
		>,
		TL<
			//IndexRange<0, 1>
    	>,
		FSEBranchStructTF
	>;



    using FirstStreamFixedTF = StreamTF<
        TL<
        	//PkdFQTree<CtrSizeT, 1>
        >,
        TL<
			//TL<IndexRange<0, 1>>
		>,
		FSEBranchStructTF
    >;


    using DataStreamTF  = StreamTF<
    	TL<TL<PkdFSQArrayT<Value, 1>>>,
    	TL<TL<TL<>>>,
		FSEBranchStructTF
    >;


    using RawStreamDescriptors = IfThenElse<
    		SizeType == PackedSizeType::FIXED,
			MergeLists<
				FirstStreamFixedTF,
				DataStreamTF
			>,
			MergeLists<
				FirstStreamVariableTF,
				DataStreamTF
			>
    >;

    using StreamDescriptors = typename bttl::BTTLAugmentStreamDescriptors<
    		RawStreamDescriptors,
			//PkdVDTreeT<CtrSizeT, 1, UByteI7Codec>
			PkdFQTreeT<CtrSizeT, 1>
	>::Type;


    using Metadata = BalancedTreeMetadata<
            typename Base::ID,
            ListSize<StreamDescriptors>::Value
    >;


    using CommonContainerPartsList = MergeLists<
                typename Base::CommonContainerPartsList,
                memoria::mmap::CtrApiName
    >;

    using IteratorPartsList = MergeLists<
                typename Base::IteratorPartsList,
                memoria::mmap::ItrMiscName
    >;



};







template <
    typename Profile,
    typename Key_,
    typename Value_
>
struct BTTypes<Profile, memoria::Map<Key_, Vector<Value_>>>: public MultimapBTTypesBase<Profile, Key_, Value_>
{
};


template <typename Profile, typename Key, typename Value, typename T>
class CtrTF<Profile, memoria::Map<Key, Vector<Value>>, T>: public CtrTF<Profile, memoria::BTTreeLayout, T> {
    using Base = CtrTF<Profile, memoria::BTTreeLayout, T>;
public:

    struct Types: Base::Types
    {
    	using CtrTypes 			= MultimapCtrTypes<Types>;
        using IterTypes 		= MultimapIterTypes<Types>;

        using PageUpdateMgr 	= PageUpdateManager<CtrTypes>;

        template <Int StreamIdx>
        using InputTupleSizeAccessor = mmap::InputTupleSizeH<StreamIdx>;
    };

    using CtrTypes 	= typename Types::CtrTypes;
    using Type 		= Ctr<CtrTypes>;
};


}

#endif
