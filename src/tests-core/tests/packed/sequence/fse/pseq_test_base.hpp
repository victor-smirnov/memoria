
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

#include <memoria/v1/tests/tests.hpp>
#include <memoria/v1/tests/tools.hpp>

#include <memoria/v1/core/packed/sseq/packed_fse_searchable_seq.hpp>
#include <memoria/v1/core/packed/tools/packed_struct_ptrs.hpp>

#include <memory>

namespace memoria {
namespace v1 {

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

    virtual ~PackedSearchableSequenceTestBase() noexcept {}

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


}}
