
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_SEQD_FACTORY_FACTORY_8_HPP
#define _MEMORIA_CONTAINERS_SEQD_FACTORY_FACTORY_8_HPP

#include <memoria/prototypes/bt/bt_factory.hpp>

#include <memoria/core/packed/packed_fse_searchable_seq.hpp>
#include <memoria/core/packed/tree/packed_fse_tree.hpp>
#include <memoria/core/packed/tree/packed_vle_tree.hpp>

#include <memoria/containers/seq_dense/seqd_names.hpp>
#include <memoria/containers/seq_dense/seqd_tools.hpp>

//#include <memoria/containers/seq_dense/container/seqd_c_checks.hpp>
#include <memoria/containers/seq_dense/container/seqd_c_tools.hpp>
#include <memoria/containers/seq_dense/container/seqd_c_find.hpp>
#include <memoria/containers/seq_dense/container/seqd_c_insert.hpp>
#include <memoria/containers/seq_dense/container/seqd_c_remove.hpp>

#include <memoria/containers/seq_dense/iterator/seqd_i_count.hpp>
#include <memoria/containers/seq_dense/iterator/seqd_i_misc.hpp>
#include <memoria/containers/seq_dense/iterator/seqd_i_rank.hpp>
#include <memoria/containers/seq_dense/iterator/seqd_i_select.hpp>
#include <memoria/containers/seq_dense/iterator/seqd_i_skip.hpp>

namespace memoria {


template <typename Profile>
struct BTTypes<Profile, memoria::Sequence<8, true> >:
	public BTTypes<Profile, memoria::BT> {

    typedef BTTypes<Profile, memoria::BT>                   Base;

    typedef UBigInt                                                          	Value;
    typedef TypeList<BigInt>                                                  	KeysList;

    static const Int BitsPerSymbol                                              = 8;
    static const Int Indexes                                                	= (1 << BitsPerSymbol) + 1;



    typedef TypeList<
    			NonLeafNodeTypes<TreeMapNode>,
    			LeafNodeTypes<TreeLeafNode>
    >																			NodeTypesList;

    typedef TypeList<
    			LeafNodeType<TreeLeafNode>,
    		    InternalNodeType<TreeMapNode>,
    		    RootNodeType<TreeMapNode>,
    		    RootLeafNodeType<TreeLeafNode>
    >																			DefaultNodeTypesList;

    typedef TypeList<
        		StreamDescr<PkdFTreeTF, PackedFSESeqTF, Indexes>
    >																			StreamDescriptors;

    typedef BalancedTreeMetadata<
    		typename Base::ID,
    		ListSize<StreamDescriptors>::Value
    >        																	Metadata;


	typedef typename MergeLists<
				typename Base::ContainerPartsList,
				memoria::bt::NodeComprName,
				memoria::seq_dense::CtrToolsName,
				memoria::seq_dense::CtrFindName,
				memoria::seq_dense::CtrInsertName,
				memoria::seq_dense::CtrRemoveName
	>::Result                                           						ContainerPartsList;


	typedef typename MergeLists<
				typename Base::IteratorPartsList,

				memoria::seq_dense::IterSelectName,
				memoria::seq_dense::IterMiscName,
				memoria::seq_dense::IterCountName,
				memoria::seq_dense::IterRankName,
				memoria::seq_dense::IterSkipName

	>::Result                                           						IteratorPartsList;


	template <typename Iterator, typename Container>
	struct IteratorCacheFactory {
		typedef memoria::map::MapIteratorPrefixCache<Iterator, Container> Type;
	};



    template <typename Types>
    using FindLTWalker 		= ::memoria::seq_dense::SkipForwardWalker<Types>;


    template <typename Types>
    using RankFWWalker 		= ::memoria::seq_dense::RankFWWalker<Types>;

    template <typename Types>
    using RankBWWalker 		= ::memoria::seq_dense::RankBWWalker<Types>;


    template <typename Types>
    using SelectFwWalker 	= ::memoria::seq_dense::SelectForwardWalker<Types>;

    template <typename Types>
    using SelectBwWalker 	= ::memoria::seq_dense::SelectBackwardWalker<Types>;


    template <typename Types>
    using SkipForwardWalker 	= ::memoria::seq_dense::SkipForwardWalker<Types>;

    template <typename Types>
    using SkipBackwardWalker 	= ::memoria::seq_dense::SkipBackwardWalker<Types>;


    template <typename Types>
    using NextLeafWalker 	 	= ::memoria::bt::NextLeafWalker<Types>;

    template <typename Types>
    using PrevLeafWalker 		= ::memoria::bt::PrevLeafWalker<Types>;


    template <typename Types>
    using FindBeginWalker 	= ::memoria::seq_dense::FindBeginWalker<Types>;

    template <typename Types>
    using FindEndWalker 	= ::memoria::seq_dense::FindEndWalker<Types>;

    template <typename Types>
    using FindRBeginWalker 	= ::memoria::seq_dense::FindRBeginWalker<Types>;

    template <typename Types>
    using FindREndWalker 	= ::memoria::seq_dense::FindREndWalker<Types>;
};

}

#endif
