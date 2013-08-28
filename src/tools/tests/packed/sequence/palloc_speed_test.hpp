
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PACKED_SEQUENCE_SPEED_TEST_HPP_
#define MEMORIA_TESTS_PACKED_SEQUENCE_SPEED_TEST_HPP_

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include "palloc_test_base.hpp"

#include <memory>

namespace memoria {

using namespace std;

template <
    Int Bits,
    template <typename> class IndexType     = PkdFTree,
    template <typename> class CodecType     = ValueFSECodec,
    template <typename> class ReindexFnType = BitmapReindexFn,
    template <typename> class SelectFnType  = BitmapSelectFn,
    template <typename> class RankFnType    = BitmapRankFn,
    template <typename> class ToolsFnType   = BitmapToolsFn
>
class PackedSearchableSequenceSpeedTest: public PackedSearchableSequenceTestBase<
    Bits,
    IndexType,
    CodecType,
    ReindexFnType,
    SelectFnType,
    RankFnType,
    ToolsFnType
> {

    typedef PackedSearchableSequenceSpeedTest<
            Bits,
            IndexType,
            CodecType,
            ReindexFnType,
            SelectFnType,
            RankFnType,
            ToolsFnType
    >                                                                           MyType;

    typedef PackedSearchableSequenceTestBase<
            Bits,
            IndexType,
            CodecType,
            ReindexFnType,
            SelectFnType,
            RankFnType,
            ToolsFnType
    >                                                                           Base;

    typedef typename Base::Seq                                                  Seq;

    typedef typename Seq::Value                                                 Value;


    static const Int Blocks                 = Seq::Indexes;
    static const Int Symbols                = 1<<Bits;
    static const Int VPB                    = Seq::ValuesPerBranch;

public:

    PackedSearchableSequenceSpeedTest(StringRef name): Base(name)
    {
        this->iterations_   = 4096;
        this->size_         = 4096;

        MEMORIA_ADD_TEST(testInsertRemove);
    }

    virtual ~PackedSearchableSequenceSpeedTest() throw() {}

    void testInsertRemove()
    {
        Seq* seq = this->createEmptySequence();
        PARemover remover(seq);

        BigInt t0 = getTimeInMillis();

        this->fillRandom(seq, this->size_);

        Int rs = seq->index()->raw_size();
        Int ds = seq->index()->data_size();
        Base::out()<<"BPE: "<<ds/(float)rs<<" BS: "<<seq->index()->block_size()<<endl;



        BigInt t1 = getTimeInMillis();

        for (Int c = 0; c < this->iterations_; c++)
        {
            Int idx1 = getRandom(this->size_);
            Int idx2 = getRandom(this->size_);

            Int symbol = getRandom(Symbols);

            seq->remove(idx2, idx2 + 1);
            seq->insert(idx1, symbol);
        }

        BigInt t2 = getTimeInMillis();

        Base::out()<<FormatTime(t1 - t0)<<" "<<FormatTime(t2 - t1)<<endl;
    }



};


}


#endif
