
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

#include <memoria/prototypes/bt_ss/btss_factory.hpp>

//#include <memoria/core/packed/sseq/packed_fse_searchable_seq.hpp>

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

#include <memoria/core/packed/misc/packed_sized_struct.hpp>

namespace memoria {

template <typename Profile, int32_t BitsPerSymbol_>
struct BTTypes<Profile, Sequence<BitsPerSymbol_, true> >: public BTTypes<Profile, BTSingleStream> {

    typedef BTTypes<Profile, BTSingleStream>                           Base;

    typedef uint8_t                                                        Value;

    static const int32_t BitsPerSymbol                                     = BitsPerSymbol_;
    static const int32_t Symbols                                           = 1 << BitsPerSymbol;

    using SequenceTypes = typename PkdFSSeqTF<BitsPerSymbol>::Type;

    using SymbolsSubstreamPath = IntList<0, 1>;

    using SeqStreamTF = bt::StreamTF<
        TL<
            StreamSize,
            TL<PkdFSSeq<SequenceTypes>>
        >,
        bt::VLDBranchStructTF
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

}
