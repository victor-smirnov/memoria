
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_seqd_FACTORY_HPP
#define _MEMORIA_CONTAINERS_seqd_FACTORY_HPP

#include <memoria/prototypes/bt/bt_factory.hpp>

#include <memoria/core/packed/packed_fse_searchable_seq.hpp>
#include <memoria/core/packed/packed_fse_tree.hpp>
#include <memoria/core/packed/packed_vle_tree.hpp>

#include <memoria/containers/seq_dense/seqd_names.hpp>
#include <memoria/containers/seq_dense/seqd_tools.hpp>

//#include <memoria/containers/seq_dense/container/seqd_c_checks.hpp>
#include <memoria/containers/seq_dense/container/seqd_c_tools.hpp>
#include <memoria/containers/seq_dense/container/seqd_c_find.hpp>
#include <memoria/containers/seq_dense/container/seqd_c_insert.hpp>
#include <memoria/containers/seq_dense/container/seqd_c_remove.hpp>

#include <memoria/containers/seq_dense/iterator/seqd_i_api.hpp>

namespace memoria {


template <typename Types, Int StreamIdx>
struct PackedFSESeqTF {

	typedef typename Types::Value												Value;
	typedef typename Types::Key                                                 Key;

	typedef typename SelectByIndexTool<
			StreamIdx,
			typename Types::StreamDescriptors
	>::Result																	Descriptor;

	static const Int BitsPerSymbol = Types::BitsPerSymbol;

	typedef typename PackedFSESearchableSeqTF<BitsPerSymbol>::Type				SequenceTypes;

	typedef PackedFSESearchableSeq<SequenceTypes> Type;
};



template <typename Profile>
struct BalancedTreeTypes<Profile, memoria::Sequence<1, true> >:
	public BalancedTreeTypes<Profile, memoria::BalancedTree> {

    typedef BalancedTreeTypes<Profile, memoria::BalancedTree>                   Base;

    typedef UBigInt                                                          	Value;
    typedef TypeList<BigInt>                                                  	KeysList;

    static const Int BitsPerSymbol                                              = 1;
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
        		StreamDescr<PackedFSETreeTF, PackedFSESeqTF, Indexes>
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
				memoria::seq_dense::IterAPIName
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



template <typename Profile, Int BitsPerSymbol, bool Dense, typename T>
class CtrTF<Profile, memoria::Sequence<BitsPerSymbol, Dense>, T>: public CtrTF<Profile, memoria::BalancedTree, T> {
};

}

#endif
