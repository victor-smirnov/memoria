// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_SEQUENCE_SEQUENCE_CREATE_TEST_HPP_
#define MEMORIA_TESTS_SEQUENCE_SEQUENCE_CREATE_TEST_HPP_

#include <memoria/memoria.hpp>
#include <memoria/tools/tests.hpp>

#include "sequence_test_base.hpp"

#include <vector>

namespace memoria {

using namespace memoria::vapi;
using namespace std;

template<Int BitsPerSymbol, bool Dense = true>
class SequenceCreateTest: public SequenceTestBase<BitsPerSymbol, Dense> {

    typedef SequenceCreateTest<BitsPerSymbol, Dense>                            MyType;
    typedef SequenceTestBase<BitsPerSymbol, Dense>                              Base;

    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::Iterator                                             Iterator;
    typedef typename Base::Ctr                                                  Ctr;

    static const Int Symbols = Base::Symbols;

    Int seq_check_count_ = 100;
    Int seq_check_start_ = 0;

    Int last_symbol_;
    Int ctr_name_;
    Int remove_idx_;
    BigInt target_size_;

    String seq_file_name_;

public:

    SequenceCreateTest(StringRef name): Base(name)
    {
        this->size_ = BitsPerSymbol == 8 ? 50000 : 300000;

        MEMORIA_ADD_TEST_PARAM(seq_check_count_);
        MEMORIA_ADD_TEST_PARAM(seq_check_start_);

        MEMORIA_ADD_TEST_PARAM(seq_file_name_)->state();
        MEMORIA_ADD_TEST_PARAM(last_symbol_)->state();
        MEMORIA_ADD_TEST_PARAM(ctr_name_)->state();
        MEMORIA_ADD_TEST_PARAM(remove_idx_)->state();
        MEMORIA_ADD_TEST_PARAM(target_size_)->state();

        MEMORIA_ADD_TEST_WITH_REPLAY(testCreateRemoveRandom, replayCreateRemoveRandom);
        MEMORIA_ADD_TEST_WITH_REPLAY(testAppend, replayAppend);
    }

    void testCreateRemoveRandom()
    {
        DefaultLogHandlerImpl logHandler(Base::out());

        Allocator allocator;
        allocator.getLogger()->setHandler(&logHandler);

        Ctr ctr(&allocator);

        allocator.commit();

        try {
            for (Int c = 0; c < this->size_; c++)
            {
                Int bit1 = this->getRandom(Symbols);
                Int idx  = this->getRandom(c + 1);

                this->out() << c << " Insert: " << bit1 << " at " << idx << endl;

                ctr.insert(idx, bit1);

                this->check(allocator, MA_SRC);

                auto iter = ctr.seek(idx);

                Int bit2 = iter.symbol();

                AssertEQ(MA_SRC, iter.pos(), idx);
                AssertEQ(MA_SRC,bit1, bit2);

                allocator.commit();
            }

            AssertEQ(MA_SRC, ctr.size(), this->size_);

            this->StoreAllocator(allocator, this->getResourcePath("create.dump"));

            BigInt size = ctr.size();

            for (Int c = 0; c < size; c++)
            {
                remove_idx_ = this->getRandom(size - c);

                ctr.remove(remove_idx_);

                target_size_ = size - c - 1;

                AssertEQ(MA_SRC, ctr.size(), target_size_);

                allocator.commit();
            }

            this->StoreAllocator(allocator, this->getResourcePath("remove.dump"));
        }
        catch (...) {
            Base::dump_name_ = Base::Store(allocator);
            throw;
        }
    }

    void replayCreateRemoveRandom()
    {
        Allocator allocator;
        allocator.commit();

        this->LoadAllocator(allocator, Base::dump_name_);

        Base::check(allocator, MA_SRC);

        Ctr ctr(&allocator, CTR_FIND, ctr_name_);

        ctr.remove(remove_idx_);

        AssertEQ(MA_SRC, ctr.size(), target_size_);

        allocator.commit();

        this->StoreAllocator(allocator, this->getResourcePath("remove-replay.dump"));
    }

    void StoreSequenceData(const vector<UByte>& seq)
    {
        String basic_name = "Data." + this->getName();

        String pairs_name = basic_name + ".seq.txt";
        seq_file_name_ = this->getResourcePath(pairs_name);

        StoreVector(seq, seq_file_name_);
    }

    void checkSequence(Ctr& seq, const vector<UByte>& data, Int start = 0)
    {
        auto i = seq.begin();

        Int cnt = start;
        for (auto i = seq.seek(start); !i.isEof(); i++, cnt++)
        {
            Int symbol1 = i.symbol();
            Int symbol2 = data[cnt];

            AssertEQ(MA_SRC, cnt, i.pos());
            AssertEQ(MA_SRC, symbol1, symbol2, SBuf()<<cnt);
        }
    }



    void testAppend()
    {
        DefaultLogHandlerImpl logHandler(Base::out());

        Allocator allocator;
        allocator.getLogger()->setHandler(&logHandler);

        Ctr ctr(&allocator);

        ctr_name_ = ctr.name();

        allocator.commit();

        auto iter = ctr.Begin();

        vector<UByte> seq;

        try {
            Int counter = 0;

            for (Int c = 0; c < this->size_; c++)
            {
                this->out()<<"Append: "<<c<<" "<<ctr.size()<<std::endl;

                Int symbol = last_symbol_ = this->getRandom(Symbols);
                iter.insert(symbol);

                seq.push_back(symbol);

                Base::check(allocator, MA_SRC);

                counter++;

                if (c + 1 != iter.pos())
                {
                    AssertEQ(MA_SRC, c + 1, iter.pos());
                }

                allocator.commit();
            }

            this->StoreResource(allocator, "append", 0);
        }
        catch (...)
        {
            iter.dumpPath();
            cout<<"Sequence Size: "<<seq.size()<<endl;

            Base::dump_name_ = Base::Store(allocator);

            seq.erase(seq.end() - 1);
            StoreSequenceData(seq);
            throw;
        }
    }


    void replayAppend()
    {
        Allocator allocator;
        allocator.commit();

        this->LoadAllocator(allocator, Base::dump_name_);

        Base::check(allocator, MA_SRC);

        Ctr ctr(&allocator, CTR_FIND, ctr_name_);

        vector<UByte> seq;

        LoadVector(seq, seq_file_name_);

        checkSequence(ctr, seq, seq_check_start_);

        auto iter = ctr.seek(ctr.size());

        iter.insert(last_symbol_);
        seq.push_back(last_symbol_);

        Base::check(allocator, MA_SRC);
    }
};



}

#endif
