
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_TL_FACTORY_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_TL_FACTORY_HPP

#include <memoria/prototypes/bt/bt_factory.hpp>

#include <memoria/prototypes/bt_tl/bttl_input.hpp>

#include <memoria/prototypes/bt_tl/bttl_names.hpp>
#include <memoria/prototypes/bt_tl/bttl_iterator.hpp>

#include <memoria/prototypes/bt_tl/bttl_tools.hpp>

#include <memoria/prototypes/bt_tl/container/bttl_c_misc.hpp>

#include <memoria/prototypes/bt_tl/iterator/bttl_i_misc.hpp>
#include <memoria/prototypes/bt_tl/iterator/bttl_i_srank.hpp>
#include <memoria/prototypes/bt_tl/iterator/bttl_i_find.hpp>

#include <tuple>

namespace memoria {

struct BTTreeLayout {};

template <
    typename Profile
>
struct BTTypes<Profile, memoria::BTTreeLayout>: public BTTypes<Profile, memoria::BT> {

    typedef BTTypes<Profile, memoria::BT>                                       Base;

    using CommonContainerPartsList = MergeLists<
                typename Base::CommonContainerPartsList,
                memoria::bttl::MiscName
    >;

    using IteratorPartsList = MergeLists<
                typename Base::IteratorPartsList,
                memoria::bttl::IteratorMiscName,
				memoria::bttl::IteratorStreamRankName,
				memoria::bttl::IteratorFindName
    >;

    template <typename Iterator, typename Container>
    struct IteratorCacheFactory {
        typedef ::memoria::bttl::BTTLIteratorPrefixCache<Iterator, Container>   Type;
    };
};




template <typename Profile, typename T>
class CtrTF<Profile, memoria::BTTreeLayout, T>: public CtrTF<Profile, memoria::BT, T> {

    using Base = CtrTF<Profile, memoria::BT, T>;
public:

    struct Types: Base::Types
    {
//    	template <Int StreamIdx>
//    	using InputTupleSizeAccessor = bttl::InputTupleSizeH<StreamIdx>;
//
//    	using StreamsSizes = TL<>;

    	using CtrTypes 			= BTTLCtrTypes<Types>;
        using IterTypes 		= BTTLIterTypes<Types>;

        using PageUpdateMgr 	= PageUpdateManager<CtrTypes>;
    };

    using CtrTypes 	= typename Types::CtrTypes;
    using Type 		= Ctr<CtrTypes>;
};


}

#endif
