
// Copyright 2013 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


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

#include <memoria/v1/containers/seq_dense/factory/seqd_factory_misc.hpp>

#include <memoria/v1/core/packed/misc/packed_sized_struct.hpp>

namespace memoria {
namespace v1 {




template <typename Profile>
struct BTTypes<Profile, v1::Sequence<1, true> >: public BTTypes<Profile, v1::BTSingleStream> {

    typedef BTTypes<Profile, v1::BTSingleStream>                           Base;

    typedef UBigInt                                                             Value;
    typedef TypeList<BigInt>                                                    KeysList;

    static constexpr Int BitsPerSymbol                                          = 1;
    static constexpr Int Symbols                                                = 2;
    static constexpr Int BranchIndexes                                          = (1 << BitsPerSymbol) + 1;

    using SequenceTypes = typename PkdFSSeqTF<BitsPerSymbol>::Type;

    using SeqStreamTF = StreamTF<
        TL<TL<StreamSize, PkdFSSeq<SequenceTypes>>>,
        FSEBranchStructTF,
        TL<TL<TL<SumRange<0, BranchIndexes - 1>>>>
    >;

    typedef TypeList<
            SeqStreamTF
    >                                                                           StreamDescriptors;




    using CommonContainerPartsList = MergeLists<
                typename Base::CommonContainerPartsList,

                seq_dense::CtrFindName,
                seq_dense::CtrInsertName,
                seq_dense::CtrRemoveName
    >;


    using FixedLeafContainerPartsList = MergeLists<
                typename Base::FixedLeafContainerPartsList,

                seq_dense::CtrInsertFixedName
    >;

    using VariableLeafContainerPartsList = MergeLists<
                typename Base::VariableLeafContainerPartsList,

                seq_dense::CtrInsertVariableName
    >;



    using IteratorPartsList = MergeLists<
                typename Base::IteratorPartsList,

                seq_dense::IterSelectName,
                seq_dense::IterMiscName,
                seq_dense::IterCountName,
                seq_dense::IterRankName,
                seq_dense::IterSkipName
    >;

};


}}