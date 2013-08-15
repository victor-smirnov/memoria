
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_CONTAINERS_LBLTREE_FACTORY_HPP_
#define MEMORIA_CONTAINERS_LBLTREE_FACTORY_HPP_

#include <memoria/core/types/types.hpp>

#include <memoria/containers/labeled_tree/ltree_names.hpp>

#include <memoria/containers/labeled_tree/container/ltree_c_api.hpp>
#include <memoria/containers/labeled_tree/container/ltree_c_find.hpp>
#include <memoria/containers/labeled_tree/container/ltree_c_insert.hpp>
#include <memoria/containers/labeled_tree/container/ltree_c_update.hpp>
#include <memoria/containers/labeled_tree/container/ltree_c_remove.hpp>

#include <memoria/containers/labeled_tree/iterator/ltree_i_api.hpp>


#include <memoria/containers/seq_dense/seqd_factory.hpp>
#include <memoria/containers/labeled_tree/ltree_walkers.hpp>
#include <memoria/containers/labeled_tree/ltree_tools.hpp>

namespace memoria {



template <typename Profile, typename... LabelDescriptors>
struct BTTypes<Profile, memoria::LabeledTree<LabelDescriptors...>>: BTTypes<Profile, memoria::Sequence<1, true>> {

    typedef BTTypes<Profile, memoria::Sequence<1, true>>                   		Base;

    typedef UBigInt                                                          	Value;
    typedef TypeList<BigInt>                                                  	KeysList;

    static const Int Indexes                                                	= 3;

    typedef TypeList<
    			NonLeafNodeTypes<BranchNode>,
    			LeafNodeTypes<LeafNode>
    >																			NodeTypesList;

    typedef TypeList<
    			LeafNodeType<LeafNode>,
    		    BranchNodeType<BranchNode>
    >																			DefaultNodeTypesList;


    typedef typename louds::StreamDescriptorsListBuilder<
    		LabelDescriptors...
    >::Type																		StreamDescriptors;

    typedef BalancedTreeMetadata<
    		typename Base::ID,
    		ListSize<StreamDescriptors>::Value
    >        																	Metadata;


	typedef typename MergeLists<
				typename Base::ContainerPartsList,
				bt::NodeComprName,
				louds::CtrApiName,
				louds::CtrFindName,
				louds::CtrInsertName,
				louds::CtrUpdateName,
				louds::CtrRemoveName
	>::Result                                           						ContainerPartsList;


	typedef typename MergeLists<
				typename Base::IteratorPartsList,
				louds::ItrApiName
	>::Result                                           						IteratorPartsList;

	typedef typename louds::LabelsTupleTF<LabelDescriptors...>::Type			LabelsTuple;


	template <typename Iterator, typename Container>
	struct IteratorCacheFactory {
		typedef memoria::louds::LOUDSIteratorCache<Iterator, Container> Type;
	};



    template <typename Types>
    using FindLTWalker 		= ::memoria::louds::SkipForwardWalker<Types>;


    template <typename Types>
    using RankFWWalker 		= ::memoria::louds::RankFWWalker<Types>;

    template <typename Types>
    using RankBWWalker 		= ::memoria::louds::RankBWWalker<Types>;


    template <typename Types>
    using SelectFwWalker 	= ::memoria::louds::SelectForwardWalker<Types>;

    template <typename Types>
    using SelectBwWalker 	= ::memoria::louds::SelectBackwardWalker<Types>;


    template <typename Types>
    using SkipForwardWalker 	= ::memoria::louds::SkipForwardWalker<Types>;

    template <typename Types>
    using SkipBackwardWalker 	= ::memoria::louds::SkipBackwardWalker<Types>;


    template <typename Types>
    using NextLeafWalker 	 	= ::memoria::bt::NextLeafWalker<Types>;

    template <typename Types>
    using PrevLeafWalker 		= ::memoria::bt::PrevLeafWalker<Types>;

    template <typename Types>
    using FindBeginWalker 		= ::memoria::louds::FindBeginWalker<Types>;

    template <typename Types>
    using FindEndWalker 		= ::memoria::louds::FindEndWalker<Types>;

    template <typename Types>
    using FindRBeginWalker 		= ::memoria::louds::FindRBeginWalker<Types>;

    template <typename Types>
    using FindREndWalker 		= ::memoria::louds::FindREndWalker<Types>;
};



template <typename Profile, typename... LabelDescriptors, typename T>
class CtrTF<Profile, memoria::LabeledTree<LabelDescriptors...>, T>: public CtrTF<Profile, memoria::Sequence<1, true>, T> {
};


}


#endif