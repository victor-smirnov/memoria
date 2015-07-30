
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



//#include <memoria/prototypes/bt_ss/container/btss_c_leaf_common.hpp>
//#include <memoria/prototypes/bt_ss/container/btss_c_leaf_fixed.hpp>
//#include <memoria/prototypes/bt_ss/container/btss_c_leaf_variable.hpp>
//
//#include <memoria/prototypes/bt_ss/iterator/btss_i_misc.hpp>

#include <tuple>

namespace memoria {

struct BTTreeLayout {};

template <
    typename Profile
>
struct BTTypes<Profile, memoria::BTTreeLayout>: public BTTypes<Profile, memoria::BT> {

    typedef BTTypes<Profile, memoria::BT>                                       Base;

//    using CommonContainerPartsList = MergeLists<
//                typename Base::CommonContainerPartsList,
//                memoria::btss::LeafCommonName
//    >;
//
//    using FixedLeafContainerPartsList = MergeLists<
//    			typename Base::FixedLeafContainerPartsList,
//    			memoria::btss::LeafFixedName
//    >;
//
//
//    using VariableLeafContainerPartsList = MergeLists<
//    			typename Base::VariableLeafContainerPartsList,
//    			memoria::btss::LeafVariableName
//    >;
//
//    using IteratorPartsList = MergeLists<
//                typename Base::IteratorPartsList,
//                memoria::btss::IteratorMiscName
//    >;

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
//    	template <Int StreamIdx>
//    	using LeafStreamSizeAccessor = bttl::LeafStreamSizeH<StreamIdx>;
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
