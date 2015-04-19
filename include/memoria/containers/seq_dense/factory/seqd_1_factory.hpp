
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_SEQD_FACTORY_FACTORY_1_HPP
#define _MEMORIA_CONTAINERS_SEQD_FACTORY_FACTORY_1_HPP

#include <memoria/prototypes/bt/bt_factory.hpp>

#include <memoria/core/packed/sseq/packed_fse_searchable_seq.hpp>
#include <memoria/core/packed/tree/packed_fse_tree.hpp>
#include <memoria/core/packed/tree/packed_vle_tree.hpp>

#include <memoria/containers/seq_dense/seqd_names.hpp>
#include <memoria/containers/seq_dense/seqd_tools.hpp>

#include <memoria/containers/seq_dense/container/seqd_c_norm.hpp>
#include <memoria/containers/seq_dense/container/seqd_c_find.hpp>
#include <memoria/containers/seq_dense/container/seqd_c_insert.hpp>
#include <memoria/containers/seq_dense/container/seqd_c_remove.hpp>

#include <memoria/containers/seq_dense/iterator/seqd_i_count.hpp>
#include <memoria/containers/seq_dense/iterator/seqd_i_misc.hpp>
#include <memoria/containers/seq_dense/iterator/seqd_i_rank.hpp>
#include <memoria/containers/seq_dense/iterator/seqd_i_select.hpp>
#include <memoria/containers/seq_dense/iterator/seqd_i_skip.hpp>

#include <memoria/containers/seq_dense/factory/seqd_factory_misc.hpp>

namespace memoria {




template <typename Profile>
struct BTTypes<Profile, memoria::Sequence<1, true> >: public BTTypes<Profile, memoria::BT> {

    typedef BTTypes<Profile, memoria::BT>                                       Base;

    typedef UBigInt                                                             Value;
    typedef TypeList<BigInt>                                                    KeysList;

    static const Int BitsPerSymbol                                              = 1;
    static const Int Indexes                                                    = (1 << BitsPerSymbol) + 1;


    struct StreamTF {
        typedef BigInt                                              Key;

        typedef core::StaticVector<BigInt, Indexes>                 AccumulatorPart;
        typedef core::StaticVector<BigInt, 1>                       IteratorPrefixPart;

        typedef PkdFTree<Packed2TreeTypes<Key, Key, Indexes>>       NonLeafType;
        typedef TL<TL<>>					IdxRangeList;

        typedef typename PkdFSSeqTF<BitsPerSymbol>::Type            SequenceTypes;

        typedef TL<PkdFSSeq<SequenceTypes>>                         LeafType;
    };

    typedef TypeList<
                StreamTF
    >                                                                           StreamDescriptors;

    typedef BalancedTreeMetadata<
            typename Base::ID,
            ListSize<StreamDescriptors>::Value
    >                                                                           Metadata;


    using ContainerPartsList = MergeLists<
                typename Base::ContainerPartsList,
                bt::NodeComprName,

                seq_dense::CtrNormName,
                seq_dense::CtrFindName,
                seq_dense::CtrInsertName,
                seq_dense::CtrRemoveName
    >;


    using IteratorPartsList = MergeLists<
                typename Base::IteratorPartsList,

                seq_dense::IterSelectName,
                seq_dense::IterMiscName,
                seq_dense::IterCountName,
                seq_dense::IterRankName,
                seq_dense::IterSkipName

    >;


    template <typename Iterator, typename Container>
    struct IteratorCacheFactory {
    	typedef ::memoria::bt::BTree2IteratorPrefixCache<Iterator, Container>   Type;
    };

    typedef IDataSource<Value>                                                  DataSource;
    typedef IDataTarget<Value>                                                  DataTarget;



    template <typename Types, typename LeafPath>
    using FindGTWalker          = ::memoria::bt1::SkipForwardWalker2<WalkerTypes<Types, LeafPath>>;


    template <typename Types, typename LeafPath>
    using RankFWWalker          = ::memoria::bt1::RankForwardWalker2<WalkerTypes<Types, LeafPath>>;

    template <typename Types, typename LeafPath>
    using RankBWWalker          = ::memoria::bt1::RankBackwardWalker2<WalkerTypes<Types, LeafPath>>;


    template <typename Types, typename LeafPath>
    using SelectFwWalker        = ::memoria::bt1::SelectForwardWalker2<WalkerTypes<Types, LeafPath>>;

    template <typename Types, typename LeafPath>
    using SelectBwWalker        = ::memoria::bt1::SelectBackwardWalker2<WalkerTypes<Types, LeafPath>>;


    template <typename Types, typename LeafPath>
    using SkipForwardWalker     = ::memoria::bt1::SkipForwardWalker2<WalkerTypes<Types, LeafPath>>;

    template <typename Types, typename LeafPath>
    using SkipBackwardWalker    = ::memoria::bt1::SkipBackwardWalker2<WalkerTypes<Types, LeafPath>>;


    template <typename Types, typename LeafPath>
    using NextLeafWalker        = ::memoria::bt1::ForwardLeafWalker<WalkerTypes<Types, LeafPath>>;

    template <typename Types, typename LeafPath>
    using PrevLeafWalker        = ::memoria::bt1::BackwardLeafWalker<WalkerTypes<Types, LeafPath>>;


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
