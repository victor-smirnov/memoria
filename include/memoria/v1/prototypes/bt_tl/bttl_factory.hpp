
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#pragma once

#include <memoria/v1/prototypes/bt/bt_factory.hpp>

#include <memoria/v1/prototypes/bt_tl/bttl_input.hpp>

#include <memoria/v1/prototypes/bt_tl/bttl_names.hpp>
#include <memoria/v1/prototypes/bt_tl/bttl_iterator.hpp>

#include <memoria/v1/prototypes/bt_tl/bttl_tools.hpp>

#include <memoria/v1/prototypes/bt_tl/container/bttl_c_misc.hpp>
#include <memoria/v1/prototypes/bt_tl/container/bttl_c_insert.hpp>
#include <memoria/v1/prototypes/bt_tl/container/bttl_c_leaf_common.hpp>
#include <memoria/v1/prototypes/bt_tl/container/bttl_c_leaf_fixed.hpp>
#include <memoria/v1/prototypes/bt_tl/container/bttl_c_leaf_variable.hpp>
#include <memoria/v1/prototypes/bt_tl/container/bttl_c_branch_common.hpp>
#include <memoria/v1/prototypes/bt_tl/container/bttl_c_branch_fixed.hpp>
#include <memoria/v1/prototypes/bt_tl/container/bttl_c_branch_variable.hpp>
#include <memoria/v1/prototypes/bt_tl/container/bttl_c_ranks.hpp>
#include <memoria/v1/prototypes/bt_tl/container/bttl_c_checks.hpp>

#include <memoria/v1/prototypes/bt_tl/iterator/bttl_i_misc.hpp>
#include <memoria/v1/prototypes/bt_tl/iterator/bttl_i_srank.hpp>
#include <memoria/v1/prototypes/bt_tl/iterator/bttl_i_find.hpp>
#include <memoria/v1/prototypes/bt_tl/iterator/bttl_i_skip.hpp>
#include <memoria/v1/prototypes/bt_tl/iterator/bttl_i_update.hpp>
#include <memoria/v1/prototypes/bt_tl/iterator/bttl_i_remove.hpp>
#include <memoria/v1/prototypes/bt_tl/iterator/bttl_i_insert.hpp>

#include <memoria/v1/prototypes/bt_tl/tools/bttl_tools_random_gen.hpp>
#include <memoria/v1/prototypes/bt_tl/tools/bttl_tools_streamdescr.hpp>

#include <tuple>

namespace memoria {
namespace v1 {

struct BTTreeLayout {};

template <
    typename Profile
>
struct BTTypes<Profile, v1::BTTreeLayout>: public BTTypes<Profile, v1::BT> {

    typedef BTTypes<Profile, v1::BT>                                       Base;

    using CommonContainerPartsList = MergeLists<
                typename Base::CommonContainerPartsList,
                v1::bttl::MiscName,
                v1::bttl::InsertName,
                v1::bttl::BranchCommonName,
                v1::bttl::LeafCommonName,
                v1::bttl::RanksName,
                v1::bttl::ChecksName
    >;

    using FixedBranchContainerPartsList = MergeLists<
                typename Base::FixedBranchContainerPartsList,
                v1::bttl::BranchFixedName
    >;

    using VariableBranchContainerPartsList = MergeLists<
                typename Base::VariableBranchContainerPartsList,
                v1::bttl::BranchVariableName
    >;

    using FixedLeafContainerPartsList = MergeLists<
                    typename Base::FixedLeafContainerPartsList,
                    v1::bttl::LeafFixedName
    >;

    using VariableLeafContainerPartsList = MergeLists<
                    typename Base::VariableLeafContainerPartsList,
                    v1::bttl::LeafVariableName
    >;


    using IteratorPartsList = MergeLists<
                typename Base::IteratorPartsList,
                v1::bttl::IteratorMiscName,
                v1::bttl::IteratorStreamRankName,
                v1::bttl::IteratorFindName,
                v1::bttl::IteratorSkipName,
                v1::bttl::IteratorUpdateName,
                v1::bttl::IteratorRemoveName,
                v1::bttl::IteratorInsertName
    >;

    template <typename Iterator, typename Container>
    struct IteratorCacheFactory {
        typedef v1::bttl::BTTLIteratorPrefixCache<Iterator, Container>   Type;
    };
};




template <typename Profile, typename T>
class CtrTF<Profile, v1::BTTreeLayout, T>: public CtrTF<Profile, v1::BT, T> {

    using Base = CtrTF<Profile, v1::BT, T>;
public:

    struct Types: Base::Types
    {
        using CtrTypes          = BTTLCtrTypes<Types>;
        using IterTypes         = BTTLIterTypes<Types>;

        using PageUpdateMgr     = PageUpdateManager<CtrTypes>;

        using LeafPrefixRanks   = v1::core::StaticVector<typename Base::Types::Position, Base::Types::Streams>;

        template <Int StreamIdx>
        using LeafSizesSubstreamIdx = IntValue<
                v1::list_tree::LeafCountSup<
                    typename Base::Types::LeafStreamsStructList,
                    IntList<StreamIdx>>::Value - 1
        >;

        template <Int StreamIdx>
        using BranchSizesSubstreamIdx = IntValue<
                v1::list_tree::LeafCountSup<
                    typename Base::Types::BranchStreamsStructList,
                    IntList<StreamIdx>>::Value - 1
        >;

        template <Int StreamIdx>
        using LeafSizesSubstreamPath = typename Base::Types::template LeafPathT<LeafSizesSubstreamIdx<StreamIdx>::Value>;

        template <Int StreamIdx>
        using BranchSizesSubstreamPath = typename Base::Types::template BranchPathT<BranchSizesSubstreamIdx<StreamIdx>::Value>;

        static const Int SearchableStreams = Base::Types::Streams - 1;
    };

    using CtrTypes  = typename Types::CtrTypes;
    using Type      = Ctr<CtrTypes>;
};


}}