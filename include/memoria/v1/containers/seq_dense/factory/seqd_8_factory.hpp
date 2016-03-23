
// Copyright Victor Smirnov 2013+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/prototypes/bt/bt_factory.hpp>

#include <memoria/v1/core/packed/sseq/packed_fse_searchable_seq.hpp>

#include <memoria/v1/containers/seq_dense/seqd_names.hpp>
#include <memoria/v1/containers/seq_dense/seqd_tools.hpp>

#include <memoria/v1/containers/seq_dense/container/seqd_c_find.hpp>
#include <memoria/v1/containers/seq_dense/container/seqd_c_insert.hpp>
#include <memoria/v1/containers/seq_dense/container/seqd_c_insert_fixed.hpp>
#include <memoria/v1/containers/seq_dense/container/seqd_c_insert_variable.hpp>
#include <memoria/v1/containers/seq_dense/container/seqd_c_remove.hpp>

#include <memoria/v1/containers/seq_dense/iterator/seqd_i_count.hpp>
#include <memoria/v1/containers/seq_dense/iterator/seqd_i_misc.hpp>
#include <memoria/v1/containers/seq_dense/iterator/seqd_i_rank.hpp>
#include <memoria/v1/containers/seq_dense/iterator/seqd_i_select.hpp>
#include <memoria/v1/containers/seq_dense/iterator/seqd_i_skip.hpp>

#include <memoria/v1/core/packed/misc/packed_sized_struct.hpp>

namespace memoria {
namespace v1 {


template <typename Profile>
struct BTTypes<Profile, v1::Sequence<8, true> >: public BTTypes<Profile, v1::BTSingleStream> {

    typedef BTTypes<Profile, v1::BTSingleStream>                           Base;

    typedef UByte                                                               Value;

    static const Int BitsPerSymbol                                              = 8;
    static const Int Symbols                                                    = 256;
    static const Int BranchIndexes                                              = (1 << BitsPerSymbol) + 1;

    using SequenceTypes = typename PkdFSSeqTF<BitsPerSymbol>::Type;


    using SeqStreamTF = StreamTF<
        TL<TL<StreamSize, PkdFSSeq<SequenceTypes>>>,
        VLDBranchStructTF,
        TL<TL<TL<>>>
    >;


    typedef TypeList<
                SeqStreamTF
    >                                                                           StreamDescriptors;



    typedef MergeLists<
                typename Base::CommonContainerPartsList,

                seq_dense::CtrFindName,
                seq_dense::CtrInsertName,
                seq_dense::CtrRemoveName
    >                                                                           CommonContainerPartsList;


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

    >                                                                           IteratorPartsList;
};

}}