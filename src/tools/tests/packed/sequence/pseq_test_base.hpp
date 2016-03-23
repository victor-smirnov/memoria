
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/tools/tests.hpp>
#include <memoria/v1/tools/tools.hpp>

#include <memoria/v1/core/packed/sseq/packed_fse_searchable_seq.hpp>
#include <memoria/v1/core/packed/tools/packed_struct_ptrs.hpp>

#include <memory>

namespace memoria {

using namespace std;

template <
    Int Bits,
    typename IndexType,
    template <typename> class ReindexFnType,
    template <typename> class SelectFnType,
    template <typename> class RankFnType,
    template <typename> class ToolsFnType
>
class PackedSearchableSequenceTestBase: public TestTask {

    typedef PackedSearchableSequenceTestBase<
            Bits,
            IndexType,
            ReindexFnType,
            SelectFnType,
            RankFnType,
            ToolsFnType
    >                                                                           MyType;

    typedef PkdFSSeqTypes<
                Bits,
                1024,
                IndexType,
                ReindexFnType,
                SelectFnType,
                RankFnType,
                ToolsFnType
    >                                                                           Types;

protected:

    using Seq = PkdFSSeq<Types>;
    using SeqPtr = PkdStructSPtr<Seq>;


    typedef typename Seq::Value                                                 Value;


    static const Int Blocks                 = Seq::Indexes;
    static const Int Symbols                = 1<<Bits;
    static const Int VPB                    = Seq::ValuesPerBranch;

    Int iterations_ = 100;

public:

    PackedSearchableSequenceTestBase(StringRef name): TestTask(name)
    {
        size_ = 32768;

        MEMORIA_ADD_TEST_PARAM(iterations_);
    }

    virtual ~PackedSearchableSequenceTestBase() throw() {}

    SeqPtr createEmptySequence(Int block_size = 1024*1024)
    {
        return MakeSharedPackedStructByBlock<Seq>(block_size);
    }



    vector<Int> populate(SeqPtr& seq, Int size, Value value = 0)
    {
        vector<Int> symbols(size);

        for (auto& s: symbols) s = value;

        seq->clear();
        seq->insert(0, size, [&](){
            return value;
        });

        assertEqual(seq, symbols);
        assertIndexCorrect(MA_SRC, seq);

        return symbols;
    }

    vector<Int> populateRandom(SeqPtr& seq, Int size)
    {
        seq->clear();
        return fillRandom(seq, size);
    }

    vector<Int> fillRandom(SeqPtr& seq, Int size)
    {
        vector<Int> symbols;

        seq->insert(0, size, [&]() {
            Int sym = getRandom(Blocks);
            symbols.push_back(sym);
            return sym;
        });

        seq->check();

        this->assertIndexCorrect(MA_SRC, seq);
        this->assertEqual(seq, symbols);

        return symbols;
    }

    Int rank(const SeqPtr& seq, Int start, Int end, Int symbol)
    {
        Int rank = 0;

        for (Int c = start; c < end; c++)
        {
            rank += seq->test(c, symbol);
        }

        return rank;
    }

    void assertIndexCorrect(const char* src, const SeqPtr& seq)
    {
        try {
            seq->check();
        }
        catch (Exception& e) {
            out()<<"Sequence structure check failed"<<std::endl;
            seq->dump(out());
            throw e;
        }
    }

    void assertEmpty(const SeqPtr& seq)
    {
        AssertEQ(MA_SRC, seq->size(), 0);
        AssertFalse(MA_SRC, seq->has_index());
    }

    void assertEqual(const SeqPtr& seq, const vector<Int>& symbols)
    {
        AssertEQ(MA_SRC, seq->size(), (Int)symbols.size());

        try {
            for (Int c = 0; c < seq->size(); c++)
            {
                AssertEQ(MA_SRC, (UBigInt)seq->symbol(c), (UBigInt)symbols[c], SBuf()<<"Index: "<<c);
            }
        }
        catch(...) {
            seq->dump(this->out());
            throw;
        }
    }
};


}
