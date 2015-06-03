
// Copyright Victor Smirnov 2013+.
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
#include <memoria/containers/seq_dense/container/seqd_c_insert_fixed.hpp>
#include <memoria/containers/seq_dense/container/seqd_c_insert_variable.hpp>
#include <memoria/containers/seq_dense/container/seqd_c_remove.hpp>

#include <memoria/containers/seq_dense/iterator/seqd_i_count.hpp>
#include <memoria/containers/seq_dense/iterator/seqd_i_misc.hpp>
#include <memoria/containers/seq_dense/iterator/seqd_i_rank.hpp>
#include <memoria/containers/seq_dense/iterator/seqd_i_select.hpp>
#include <memoria/containers/seq_dense/iterator/seqd_i_skip.hpp>

namespace memoria {


template <typename Profile>
struct BTTypes<Profile, memoria::Sequence<8, true> >: public BTTypes<Profile, memoria::BTSingleStream> {

    typedef BTTypes<Profile, memoria::BT>                                       Base;

    typedef UByte                                                             	Value;

    static const Int BitsPerSymbol                                              = 8;
    static const Int BranchIndexes                                                    = (1 << BitsPerSymbol) + 1;

    struct StreamTF {
        typedef BigInt                                                Key;

        typedef PkdVTree<Packed2TreeTypes<
                Key, Key, BranchIndexes, UByteExintCodec
        >>                                                          NonLeafType;
        typedef TL<TL<>>											IdxRangeList;


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


    typedef MergeLists<
                typename Base::CommonContainerPartsList,

                seq_dense::CtrFindName,
                seq_dense::CtrInsertName,
                seq_dense::CtrRemoveName
    >                                                                   		CommonContainerPartsList;


    using FixedLeafContainerPartsList = MergeLists<
                typename Base::FixedLeafContainerPartsList,

                seq_dense::CtrInsertFixedName
    >;

    using VariableLeafContainerPartsList = MergeLists<
                typename Base::VariableLeafContainerPartsList,

                seq_dense::CtrInsertVariableName
    >;



    typedef MergeLists<
                typename Base::IteratorPartsList,

                seq_dense::IterSelectName,
                seq_dense::IterMiscName,
                seq_dense::IterCountName,
                seq_dense::IterRankName,
                seq_dense::IterSkipName

    >                                                                   		IteratorPartsList;
};

}

#endif
