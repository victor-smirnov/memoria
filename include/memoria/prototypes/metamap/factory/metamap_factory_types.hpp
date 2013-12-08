
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_METAMAP_FACTORY_TYPES_HPP
#define _MEMORIA_PROTOTYPES_METAMAP_FACTORY_TYPES_HPP

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
#include <memoria/prototypes/metamap/container/metamap_c_nav.hpp>
#include <memoria/prototypes/metamap/container/metamap_c_remove.hpp>
#include <memoria/prototypes/metamap/container/metamap_c_find.hpp>

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



namespace memoria {


template <typename Profile, Int Indexes_, typename Key_, typename Value_, typename HiddenLabelsList, typename LabelsList>
struct BTTypes<
	Profile,
	memoria::MetaMap<Indexes_, Key_, Value_, LabelsList, HiddenLabelsList>
>:
public BTTypes<Profile, memoria::BT> {

    typedef BTTypes<Profile, memoria::BT>                                       Base;

    static const Int Indexes													= Indexes_;

    typedef typename IfThenElse<
    			IfTypesEqual<Value_, IDType>::Value,
    			typename Base::ID,
    			Value_
    >::Result																	Value;

    typedef Key_                                                    			Key;

    typedef typename TupleBuilder<
    		typename metamap::LabelTypeListBuilder<HiddenLabelsList>::Type
    >::Type																		HiddenLabelsTuple;

    typedef typename TupleBuilder<
    		typename metamap::LabelTypeListBuilder<LabelsList>::Type
    >::Type																		LabelsTuple;

    typedef metamap::MetaMapEntry<
    			Indexes,
    			Key,
    			Value,
    			HiddenLabelsTuple,
    			LabelsTuple
    >																			Entry;

    typedef TypeList<
            LeafNodeTypes<LeafNode>,
            NonLeafNodeTypes<BranchNode>
    >                                                                           NodeTypesList;

    typedef TypeList<
            TreeNodeType<LeafNode>,
            TreeNodeType<BranchNode>
    >                                                                           DefaultNodeTypesList;

    static const Int Labels														= ListSize<LabelsList>::Value;
    static const Int HiddenLabels												= ListSize<HiddenLabelsList>::Value;

    struct StreamTF {
    	typedef PackedFSEMap<
    	        			PackedFSEMapTypes<
    	        				Key,
    	        				Value,
    	        				Indexes,
    	        				HiddenLabelsList,
    	        				LabelsList
    	        			>
    	 	 	 > 														LeafType;


    	static const Int LeafIndexes 									= LeafType::SizedIndexes;

        typedef core::StaticVector<BigInt, LeafIndexes>					AccumulatorPart;
        typedef core::StaticVector<BigInt, Indexes + 1>					IteratorPrefixPart;

        typedef PkdFTree<Packed2TreeTypes<Key, Key, LeafIndexes>> 		NonLeafType;
    };

    typedef metamap::LabelOffsetProc<LabelsList> 								LabelsOffset;
    typedef metamap::LabelOffsetProc<HiddenLabelsList> 							HiddenLabelsOffset;


    typedef TypeList<StreamTF>                                                  StreamDescriptors;

    typedef BalancedTreeMetadata<
            typename Base::ID,
            ListSize<StreamDescriptors>::Value
    >                                                                           Metadata;


    typedef typename MergeLists<
                typename Base::ContainerPartsList,
                bt::NodeComprName,

                metamap::CtrNavName,
                metamap::CtrInsertName,
                metamap::CtrRemoveName,
                metamap::CtrFindName
    >::Result                                                                   ContainerPartsList;


    typedef typename MergeLists<
                typename Base::IteratorPartsList,

                metamap::ItrKeysName,
                metamap::ItrNavName,
                metamap::ItrValueByRefName,
                metamap::ItrEntryName,
                metamap::ItrLabelsName,
                metamap::ItrFindName,
                metamap::ItrMiscName
    >::Result                                                                   IteratorPartsList;


    template <typename Iterator, typename Container>
    struct IteratorCacheFactory {
        typedef metamap::MetaMapIteratorPrefixCache<Iterator, Container> Type;
    };



    template <typename Types>
    using FindGTWalker      	= bt1::FindGTForwardWalker<Types, bt1::DefaultIteratorPrefixFn>;

    template <typename Types>
    using FindGEWalker      	= bt1::FindGEForwardWalker<Types, bt1::DefaultIteratorPrefixFn>;

    template <typename Types>
    using FindBackwardWalker    = bt1::FindBackwardWalker<Types, bt1::DefaultIteratorPrefixFn>;


    template <typename Types>
    using SkipForwardWalker     = bt1::SkipForwardWalker<Types, bt1::DefaultIteratorPrefixFn>;

    template <typename Types>
    using SkipBackwardWalker    = bt1::SkipBackwardWalker<Types, bt1::DefaultIteratorPrefixFn>;

    template <typename Types>
    using SelectForwardWalker   = metamap::SelectForwardWalker<Types, bt1::DefaultIteratorPrefixFn>;

    template <typename Types>
    using SelectBackwardWalker  = metamap::SelectBackwardWalker<Types, bt1::DefaultIteratorPrefixFn>;


    template <typename Types>
    using FindBeginWalker   	= bt1::FindBeginWalker<Types>;
};


}

#endif
