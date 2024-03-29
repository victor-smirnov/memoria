
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

#include <memoria/tests/tests.hpp>
#include <memoria/tests/assertions.hpp>

#include <memoria/core/packed/sseq/packed_fse_searchable_seq.hpp>
#include <memoria/core/packed/tools/packed_struct_ptrs.hpp>

#include <memoria/reactor/reactor.hpp>

#include <memory>

namespace memoria {
namespace tests {

template <
    size_t Bits,
    typename IndexType,
    template <typename> class ReindexFnType,
    template <typename> class SelectFnType,
    template <typename> class RankFnType,
    template <typename> class ToolsFnType
>
class PackedSearchableSequenceTestBase: public TestState {

    using Base = TestState;

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


    static const size_t Blocks                 = Seq::Indexes;
    static const size_t Symbols                = 1 << Bits;
    static const size_t VPB                    = Seq::ValuesPerBranch;

    size_t iterations_ = 100;
    int64_t size_{32768};

public:

    MMA_STATE_FILEDS(size_, iterations_);



    SeqPtr createEmptySequence(size_t block_size = 1024*1024)
    {
        return MakeSharedPackedStructByBlock<Seq>(block_size);
    }



    std::vector<size_t> populate(SeqPtr& seq, size_t size, Value value = 0)
    {
        std::vector<size_t> symbols(size);

        for (auto& s: symbols) s = value;

        seq->clear();
        seq->insert(0, size, [&](){
            return value;
        });

        assertEqual(seq, symbols);
        assertIndexCorrect(MA_SRC, seq);

        return symbols;
    }

    std::vector<size_t> populateRandom(SeqPtr& seq, size_t size)
    {
        seq->clear();
        return fillRandom(seq, size);
    }

    std::vector<size_t> fillRandom(SeqPtr& seq, size_t size)
    {
        std::vector<size_t> symbols;

        seq->insert(0, size, [&]() {
            size_t sym = getRandom(Blocks);
            symbols.push_back(sym);
            return sym;
        });

        seq->check();

        this->assertIndexCorrect(MA_SRC, seq);
        this->assertEqual(seq, symbols);

        return symbols;
    }

    size_t rank(const SeqPtr& seq, size_t start, size_t end, size_t symbol)
    {
        size_t rank = 0;

        for (size_t c = start; c < end; c++)
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
            out() << "Sequence structure check failed" << std::endl;
            seq->dump(out());
            throw e;
        }
    }

    void assertEmpty(const SeqPtr& seq)
    {
        assert_equals(seq->size(), 0);
        assert_equals(false, seq->has_index());
    }

    void assertEqual(const SeqPtr& seq, const std::vector<size_t>& symbols)
    {
        assert_equals(seq->size(), (size_t)symbols.size());

        try {
            for (size_t c = 0; c < seq->size(); c++)
            {
                assert_equals((uint64_t)seq->symbol(c), (uint64_t)symbols[c], "Index: {}", c);
            }
        }
        catch(...) {
            seq->dump(this->out());
            throw;
        }
    }
};


}}
