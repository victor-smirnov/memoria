
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

#include <memoria/v1/core/packed/sseq/packed_fse_searchable_seq.hpp>
#include <memoria/v1/core/packed/tree/vle/packed_vle_quick_tree.hpp>
#include <memoria/v1/core/packed/tree/vle/packed_vle_dense_tree.hpp>
#include <memoria/v1/core/packed/tree/fse/packed_fse_quick_tree.hpp>
#include <memoria/v1/core/tools/elias_codec.hpp>

#include <functional>

namespace memoria {
namespace v1 {

template <
    int32_t BitsPerSymbol_ = 8,
    typename IndexType = PkdVDTreeT<int64_t, 1<<BitsPerSymbol_, UInt64I64Codec>,
    template <typename> class ReindexFnType = VLEReindex8Fn,
    template <typename> class SelectFnType  = Seq8SelectFn,
    template <typename> class RankFnType    = Seq8RankFn,
    template <typename> class ToolsFnType   = Seq8ToolsFn
>
struct PackedCxMultiSequenceTypes {
    static const int32_t BitsPerSymbol = BitsPerSymbol_;

    using Index = IndexType;

    template <typename Seq>
    using ReindexFn = ReindexFnType<Seq>;

    template <typename Seq>
    using SelectFn  = SelectFnType<Seq>;

    template <typename Seq>
    using RankFn    = RankFnType<Seq>;

    template <typename Seq>
    using ToolsFn   = ToolsFnType<Seq>;

};

template <typename Types>
class PackedCxMultiSequence: public PackedAllocator {

    typedef PackedAllocator                                                     Base;
    typedef PackedCxMultiSequence<Types>                                        MyType;

public:

    typedef PkdFQTreeT<int32_t, 1>                                                  LabelArray;
    typedef typename LabelArray::Values                                         LabelArrayValues;

    typedef PkdFSSeqTypes<
            Types::BitsPerSymbol,
            512,
            typename Types::Index,
            Types::template ReindexFn,
            Types::template SelectFn,
            Types::template RankFn,
            Types::template ToolsFn
    >                                                                           SequenceTypes;
    typedef PkdFSSeq<SequenceTypes>                                             Sequence;

    typedef typename Sequence::SymbolAccessor                                   SymbolAccessor;
    typedef typename Sequence::ConstSymbolAccessor                              ConstSymbolAccessor;

    enum {
        LABELS, SYMBOLS
    };


public:
    PackedCxMultiSequence() {}

    LabelArray* labels() {
        return Base::template get<LabelArray>(LABELS);
    }

    const LabelArray* labels() const {
        return Base::template get<LabelArray>(LABELS);
    }

    Sequence* sequence() {
        return Base::template get<Sequence>(SYMBOLS);
    }

    const Sequence* sequence() const {
        return Base::template get<Sequence>(SYMBOLS);
    }

    static int32_t empty_size()
    {
        int32_t labels_block_size   = LabelArray::empty_size();
        int32_t symbols_block_size  = Sequence::empty_size();

        int32_t block_size = Base::block_size(labels_block_size + symbols_block_size, 2);

        return block_size;
    }

    OpStatus init()
    {
        int32_t block_size = MyType::empty_size();

        if(isFail(Base::init(block_size, 2))) {
            return OpStatus::FAIL;
        }

        if(isFail(Base::template allocateEmpty<LabelArray>(LABELS))) {
            return OpStatus::FAIL;
        }

        if(isFail(Base::template allocateEmpty<Sequence>(SYMBOLS))) {
            return OpStatus::FAIL;
        }

        return OpStatus::OK;
    }

    int32_t rank(int32_t subseq_num, int32_t to, int32_t symbol) const
    {
        const Sequence*     seq     = sequence();
        const LabelArray*   labels  = this->labels();

        MEMORIA_V1_ASSERT(to, <=, labels->value(0, subseq_num));

        int32_t seq_pefix   = labels->sum(0, subseq_num);

        return seq->rank(seq_pefix, seq_pefix + to, symbol);
    }

    SelectResult select(int32_t subseq_num, int32_t rank, int32_t symbol) const
    {
        const Sequence*     seq     = sequence();
        const LabelArray*   labels  = this->labels();

        int32_t seq_size    = labels->value(0, subseq_num);
        int32_t seq_prefix  = labels->sum(0, subseq_num);
        int32_t rank_prefix = seq->rank(seq_prefix, symbol);

        SelectResult result = seq->selectFw(symbol, rank_prefix + rank);
        if (result.idx() - seq_prefix < seq_size)
        {
            return SelectResult(result.idx() - seq_prefix, rank, true);
        }
        else {
            return SelectResult(seq_prefix + seq_size, seq->rank(seq_prefix, seq_prefix + seq_size), false);
        }
    }

    OpStatus insertSubsequence(int32_t idx)
    {
        if(isFail(labels()->insert(idx, LabelArrayValues()))) {
            return OpStatus::FAIL;
        }

        return labels()->reindex();
    }

    void appendSubsequence()
    {
        insertSubsequence(labels()->size());
    }

    OpStatus insertSymbol(int32_t subseq_num, int32_t idx, int32_t symbol)
    {
        Sequence* seq       = sequence();
        LabelArray* labels  = this->labels();

        int32_t seq_prefix  = labels->sum(0, subseq_num);

        MEMORIA_V1_ASSERT(idx, <=, labels->value(0, subseq_num));

        if(isFail(seq->insert(seq_prefix + idx, symbol))) {
            return OpStatus::FAIL;
        }

        labels->value(0, subseq_num)++;
//        labels->setValue(0, subseq_num)++;

        labels->reindex();

        return seq->reindex();
    }

    OpStatus removeSymbol(int32_t subseq_num, int32_t idx)
    {
        Sequence* seq       = sequence();
        LabelArray* labels  = this->labels();

        int32_t seq_prefix  = labels->sum(0, subseq_num);

        MEMORIA_V1_ASSERT(idx, <=, labels->value(0, subseq_num));

        if(isFail(seq->removeSymbol(seq_prefix + idx))) {
            return OpStatus::FAIL;
        }

        labels->value(0, subseq_num)--;

        labels->reindex();

        return seq->reindex();
    }

    OpStatus appendSymbol(int32_t subseq_num, int32_t symbol)
    {
        LabelArray* labels  = this->labels();
        int32_t size        = labels->value(0, subseq_num);

        return insertSymbol(subseq_num, size, symbol);
    }

    OpStatus remove(int32_t subseq_num)
    {
        LabelArray* labels  = this->labels();
        MEMORIA_V1_ASSERT(labels->value(0, subseq_num), ==, 0);

        return labels->removeSpace(subseq_num, subseq_num + 1);
    }

    int32_t subseq_size(int32_t seq_num) const
    {
        return labels()->value(0, seq_num);
    }

    int32_t length(int32_t seq_num) const
    {
        return subseq_size(seq_num);
    }

    static int32_t block_size(int32_t client_area)
    {
        return Base::block_size(client_area, 2);
    }


    ConstSymbolAccessor
    symbol(int32_t seq_num, int32_t idx) const
    {
        int32_t seq_prefix  = labels()->sum(0, seq_num);
        int32_t size        = labels()->value(0, seq_num);

        MEMORIA_V1_ASSERT(idx, <, size);

        return sequence()->symbol(seq_prefix + idx);
    }

    SymbolAccessor
    sumbol(int32_t seq_num, int32_t idx)
    {
        int32_t seq_prefix  = labels()->sum(0, seq_num);
        int32_t size        = labels()->value(seq_num);

        MEMORIA_V1_ASSERT(idx, <, size);

        return sequence()->symbol(seq_prefix + idx);
    }

    void dump(std::ostream& out = std::cout, bool multi = true, bool dump_index = true) const
    {
//      if (dump_index) {
//          Base::dump(out);
//      }

        out << "Sequence Labels: " << std::endl;
        labels()->dump(out, dump_index);
        out << std::endl;

        if (multi)
        {
            if (dump_index && sequence()->has_index())
            {
                sequence()->index()->dump(out);
            }

            auto values = sequence()->symbols();

            auto labels = this->labels();

            int32_t offset = 0;

            for (int32_t c = 0; c <labels->size(); c++)
            {
                int32_t size = labels->value(0, c);

                out << "seq: " << c << " offset: " << offset << " size: " << size << std::endl;

                dumpSymbols<typename Sequence::Value>(out, size, 8, [values, offset](int32_t idx) {
                    return values[idx + offset];
                });

                offset += size;

                out << std::endl << std::endl;
            }
        }
        else {
            out << "Sequence: " << std::endl;
            sequence()->dump(out, dump_index);
            out << std::endl;
        }
    }
};

}}
