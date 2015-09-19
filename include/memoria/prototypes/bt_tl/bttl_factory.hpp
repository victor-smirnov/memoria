
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
#include <memoria/prototypes/bt_tl/container/bttl_c_insert.hpp>
#include <memoria/prototypes/bt_tl/container/bttl_c_ranks.hpp>

#include <memoria/prototypes/bt_tl/iterator/bttl_i_misc.hpp>
#include <memoria/prototypes/bt_tl/iterator/bttl_i_srank.hpp>
#include <memoria/prototypes/bt_tl/iterator/bttl_i_find.hpp>
#include <memoria/prototypes/bt_tl/iterator/bttl_i_skip.hpp>
#include <memoria/prototypes/bt_tl/iterator/bttl_i_update.hpp>
#include <memoria/prototypes/bt_tl/iterator/bttl_i_remove.hpp>
#include <memoria/prototypes/bt_tl/iterator/bttl_i_insert.hpp>

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
                memoria::bttl::MiscName,
				memoria::bttl::InsertName,
				memoria::bttl::RanksName
    >;

    using IteratorPartsList = MergeLists<
                typename Base::IteratorPartsList,
                memoria::bttl::IteratorMiscName,
				memoria::bttl::IteratorStreamRankName,
				memoria::bttl::IteratorFindName,
				memoria::bttl::IteratorSkipName,
				memoria::bttl::IteratorUpdateName,
				memoria::bttl::IteratorRemoveName,
				memoria::bttl::IteratorInsertName
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
    	using CtrTypes 			= BTTLCtrTypes<Types>;
        using IterTypes 		= BTTLIterTypes<Types>;

        using PageUpdateMgr 	= PageUpdateManager<CtrTypes>;

        using LeafPrefixRanks   = typename Base::Types::Position[Base::Types::Streams];

        template <Int StreamIdx>
        using LeafSizesSubstreamIdx = IntValue<
        		memoria::list_tree::LeafCountSup<
					typename Base::Types::LeafStreamsStructList,
					IntList<StreamIdx>>::Value - 1
		>;

        template <Int StreamIdx>
        using BranchSizesSubstreamIdx = IntValue<
        		memoria::list_tree::LeafCountSup<
					typename Base::Types::BranchStreamsStructList,
					IntList<StreamIdx>>::Value - 1
		>;

        template <Int StreamIdx>
        using LeafSizesSubstreamPath = typename Base::Types::template LeafPathT<LeafSizesSubstreamIdx<StreamIdx>::Value>;

        template <Int StreamIdx>
        using BranchSizesSubstreamPath = typename Base::Types::template BranchPathT<BranchSizesSubstreamIdx<StreamIdx>::Value>;

        static const Int SearchableStreams = Base::Types::Streams - 1;
    };

    using CtrTypes 	= typename Types::CtrTypes;
    using Type 		= Ctr<CtrTypes>;
};


}

#endif
