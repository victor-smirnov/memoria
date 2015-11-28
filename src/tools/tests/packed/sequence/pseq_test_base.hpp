
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PACKED_SEQUENCE_TEST_BASE_HPP_
#define MEMORIA_TESTS_PACKED_SEQUENCE_TEST_BASE_HPP_

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/core/packed/sseq/packed_fse_searchable_seq.hpp>

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

    typedef PkdFSSeq<Types>                                     Seq;

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

    Seq* createEmptySequence(Int block_size = 1024*1024)
    {
        void* memory_block = malloc(block_size);
        memset(memory_block, 0, block_size);

        PackedAllocator* allocator = T2T<PackedAllocator*>(memory_block);

        allocator->init(block_size, 1);
        allocator->setTopLevelAllocator();

        allocator->allocateEmpty<Seq>(0);

        Seq* seq = allocator->template get<Seq>(0);

        return seq;
    }



    vector<Int> populate(Seq* seq, Int size, Value value = 0)
    {
        vector<Int> symbols(size);

        for (auto& s: symbols) s = value;

        seq->clear();
        seq->insert(0, size, [&](){
            return value;
        });

        this->assertEqual(seq, symbols);
        this->assertIndexCorrect(MA_SRC, seq);

        return symbols;
    }

    vector<Int> populateRandom(Seq* seq, Int size)
    {
        seq->clear();
        return fillRandom(seq, size);
    }

    vector<Int> fillRandom(Seq* seq, Int size)
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

    Int rank(const Seq* seq, Int start, Int end, Int symbol)
    {
        Int rank = 0;

        for (Int c = start; c < end; c++)
        {
            rank += seq->test(c, symbol);
        }

        return rank;
    }

    void assertIndexCorrect(const char* src, const Seq* seq)
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

    void assertEmpty(const Seq* seq)
    {
        AssertEQ(MA_SRC, seq->size(), 0);
        AssertEQ(MA_SRC, seq->block_size(), Seq::empty_size());
        AssertFalse(MA_SRC, seq->has_index());
    }

    void assertEqual(const Seq* seq, const vector<Int>& symbols)
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


#endif
