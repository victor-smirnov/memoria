
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

#include <memoria/core/packed/sseq/packed_rle_searchable_seq.hpp>
#include <memoria/core/packed/tools/packed_struct_ptrs.hpp>

#include <memoria/reactor/reactor.hpp>

#include <memory>

namespace memoria {
namespace tests {

template <
    int32_t Symbols
>
class PackedRLESequenceTestBase: public TestState {

    using Base = TestState;
    using MyType = PackedRLESequenceTestBase<Symbols>;

protected:

    using Seq    = PkdRLESeqT<Symbols>;
    using SeqPtr = PkdStructSPtr<Seq>;

    using Value  = typename Seq::Value;


    static const int32_t Blocks                 = Seq::Indexes;
    static const int32_t VPB                    = Seq::ValuesPerBranch;

    int64_t size_{32768};
    int32_t iterations_{100};

public:

    MMA_STATE_FILEDS(size_, iterations_);


    SeqPtr createEmptySequence(int32_t block_size = 1024*1024)
    {
        return MakeSharedPackedStructByBlock<Seq>(block_size);
    }

    std::vector<int32_t> populate(SeqPtr& seq, int32_t size, Value value = 0)
    {
        std::vector<int32_t> symbols(size);

        for (auto& s: symbols) s = value;

        seq->clear();
        seq->append_and_reindex(value, size);

        assertEqual(seq, symbols);
        assertIndexCorrect(MA_SRC, seq);

        return symbols;
    }

    template <typename T>
    std::vector<int32_t> populateRandom(T& seq, int32_t size, bool compactify = true)
    {
        seq->clear().get_or_throw();
        return fillRandom(seq, size, compactify);
    }

    template <typename T>
    std::vector<int32_t> fillRandom(T& seq, int32_t size, bool compactify = true)
    {
        for (int32_t c = 0; c < size; c++)
        {
            int32_t sym = getRandom(Blocks);
            int32_t len = getRandom(100) + 1;
            seq->append(sym, len).get_or_throw();
        }

        seq->reindex().get_or_throw();
        seq->check().get_or_throw();

        if (compactify) {
            seq->compactify().get_or_throw();
            seq->check().get_or_throw();
        }

        std::vector<int32_t> symbols;

        auto iter = seq->begin();

        while (iter.has_data())
        {
            symbols.push_back(iter.symbol());
            iter.next();
        }

        this->assertEqual(seq, symbols);

        return symbols;
    }

    template <typename T>
    int32_t rank(const T& seq, int32_t start, int32_t end, int32_t symbol)
    {
        int32_t rank = 0;

        auto iter = seq->iterator(start);

        for (int32_t c = 0; c < end - start; c++)
        {
            rank += iter.symbol() == symbol;
            iter.next();
        }

        return rank;
    }

    template <typename T>
    void assertIndexCorrect(const char* src, const T& seq)
    {
        try {
            seq->check().get_or_throw();
        }
        catch (Exception& e) {
            out()<<"Sequence structure check failed"<<std::endl;
            seq->dump(out());
            throw e;
        }
    }

    template <typename T>
    void assertEmpty(const T& seq)
    {
        assert_equals(0, seq->size());
        assert_equals(false, seq->has_index());
    }

    template <typename T>
    void dumpAsSymbols(const std::vector<T>& symbols)
    {
        dumpSymbols<int8_t>(this->out(), symbols.size(), NumberOfBits(Symbols - 1), [&](int32_t idx){
            return symbols[idx];
        });
    }

    template <typename T>
    void assertEqual(const T& seq, const std::vector<int32_t>& symbols)
    {
        assert_equals(seq->size(), (int32_t)symbols.size());

        try {
            auto iter = seq->begin();

            for (int32_t c = 0; c < seq->size(); c++)
            {
                int32_t sym1 = iter.symbol();
                int32_t sym2 = symbols[c];

                assert_equals(sym1, sym2, "Index: {}", c);

                iter.next();
            }
        }
        catch(...) {
            seq->dump(this->out());
            dumpAsSymbols(symbols);
            throw;
        }
    }
};


}}
