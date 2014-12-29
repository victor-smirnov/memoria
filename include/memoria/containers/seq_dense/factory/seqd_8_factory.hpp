
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_SEQD_FACTORY_FACTORY_8_HPP
#define _MEMORIA_CONTAINERS_SEQD_FACTORY_FACTORY_8_HPP

#include <memoria/prototypes/bt/bt_factory.hpp>

#include <memoria/core/packed/sseq/packed_fse_searchable_seq.hpp>
#include <memoria/core/packed/tree/packed_fse_tree.hpp>
#include <memoria/core/packed/tree/packed_vle_tree.hpp>

#include <memoria/containers/seq_dense/seqd_names.hpp>
#include <memoria/containers/seq_dense/seqd_tools.hpp>

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

    typedef BTTypes<Profile, memoria::BT>                                       Base;

    typedef UBigInt                                                             Value;
    typedef TypeList<BigInt>                                                    KeysList;

    static const Int BitsPerSymbol                                              = 8;
    static const Int Indexes                                                    = (1 << BitsPerSymbol) + 1;



//    typedef TypeList<
//                NonLeafNodeTypes<BranchNode>,
//                LeafNodeTypes<LeafNode>
//    >                                                                           NodeTypesList;
//
//    typedef TypeList<
//                TreeNodeType<LeafNode>,
//                TreeNodeType<BranchNode>
//    >                                                                           DefaultNodeTypesList;



    struct StreamTF {
        typedef BigInt                                                Key;

        typedef core::StaticVector<BigInt, Indexes>                 AccumulatorPart;
        typedef core::StaticVector<BigInt, 1>                       IteratorPrefixPart;

        typedef PkdVTree<Packed2TreeTypes<
                Key, Key, Indexes, UByteExintCodec
        >>                                                          NonLeafType;


        typedef typename PkdFSSeqTF<BitsPerSymbol>::Type            SequenceTypes;

        typedef TL<PkdFSSeq<SequenceTypes>>                             LeafType;
    };


    typedef TypeList<
                StreamTF
    >                                                                           StreamDescriptors;

    typedef BalancedTreeMetadata<
                typename Base::ID,
                ListSize<StreamDescriptors>::Value
    >                                                                           Metadata;


    typedef typename MergeLists<
                typename Base::ContainerPartsList,
                bt::NodeComprName,

                seq_dense::CtrFindName,
                seq_dense::CtrInsertName,
                seq_dense::CtrRemoveName
    >::Result                                                                   ContainerPartsList;


    typedef typename MergeLists<
                typename Base::IteratorPartsList,

                seq_dense::IterSelectName,
                seq_dense::IterMiscName,
                seq_dense::IterCountName,
                seq_dense::IterRankName,
                seq_dense::IterSkipName

    >::Result                                                                   IteratorPartsList;


    template <typename Iterator, typename Container>
    struct IteratorCacheFactory {
        typedef memoria::seq_dense::SequenceIteratorCache<Iterator, Container>  Type;
    };

    typedef IDataSource<Value>                                                  DataSource;
    typedef IDataTarget<Value>                                                  DataTarget;


    template <typename Types, typename BranchPath, typename LeafPath>
    using FindGTWalker          = ::memoria::seq_dense::SkipForwardWalker<WalkerTypes<Types, BranchPath, LeafPath>>;


    template <typename Types, typename BranchPath, typename LeafPath>
    using RankFWWalker          = ::memoria::seq_dense::RankFWWalker<WalkerTypes<Types, BranchPath, LeafPath>>;

    template <typename Types, typename BranchPath, typename LeafPath>
    using RankBWWalker          = ::memoria::seq_dense::RankBWWalker<WalkerTypes<Types, BranchPath, LeafPath>>;


    template <typename Types, typename BranchPath, typename LeafPath>
    using SelectFwWalker        = ::memoria::seq_dense::SelectForwardWalker<WalkerTypes<Types, BranchPath, LeafPath>>;

    template <typename Types, typename BranchPath, typename LeafPath>
    using SelectBwWalker        = ::memoria::seq_dense::SelectBackwardWalker<WalkerTypes<Types, BranchPath, LeafPath>>;


    template <typename Types, typename BranchPath, typename LeafPath>
    using SkipForwardWalker     = ::memoria::seq_dense::SkipForwardWalker<WalkerTypes<Types, BranchPath, LeafPath>>;

    template <typename Types, typename BranchPath, typename LeafPath>
    using SkipBackwardWalker    = ::memoria::seq_dense::SkipBackwardWalker<WalkerTypes<Types, BranchPath, LeafPath>>;


    template <typename Types, typename BranchPath, typename LeafPath>
    using NextLeafWalker        = ::memoria::bt::NextLeafWalker<WalkerTypes<Types, BranchPath, LeafPath>>;

    template <typename Types, typename BranchPath, typename LeafPath>
    using PrevLeafWalker        = ::memoria::bt::PrevLeafWalker<WalkerTypes<Types, BranchPath, LeafPath>>;


    template <typename Types>
    using FindBeginWalker       = ::memoria::seq_dense::FindBeginWalker<Types>;

    template <typename Types>
    using FindEndWalker         = ::memoria::seq_dense::FindEndWalker<Types>;

    template <typename Types>
    using FindRBeginWalker      = ::memoria::seq_dense::FindRBeginWalker<Types>;

    template <typename Types>
    using FindREndWalker        = ::memoria::seq_dense::FindREndWalker<Types>;
};

}

#endif
