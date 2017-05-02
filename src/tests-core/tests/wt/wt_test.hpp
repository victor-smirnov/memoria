
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

#include <memoria/v1/tools/profile_tests.hpp>
#include <memoria/v1/tools/tools.hpp>

#include <memoria/v1/containers/wt/wt_factory.hpp>

#include "../prototype/bt/bt_test_base.hpp"

#include <memory>
#include <map>

namespace memoria {
namespace v1 {

using namespace std;

class WTTest: public BTTestBase<WT, PersistentInMemAllocator<>, DefaultProfile<>> {

    using Base      = BTTestBase<WT, PersistentInMemAllocator<>, DefaultProfile<>>;
    using MyType    = WTTest;

    using typename Base::Ctr;
    using typename Base::Iterator;
    using typename Base::CtrName;

    using Base::commit;
    using Base::drop;
    using Base::branch;
    using Base::allocator;
    using Base::snapshot;
    using Base::check;
    using Base::out;
    using Base::size_;
    using Base::storeAllocator;
    using Base::isReplayMode;
    using Base::getResourcePath;


    typedef pair<uint32_t, int32_t>                                                 Pair;

    int32_t alphabet_size_  = 10;
    int32_t remove_check_   = 100;

public:

    WTTest(StringRef name): Base(name)
    {
        Ctr::initMetadata();

        size_ = 5000;

        MEMORIA_ADD_TEST_PARAM(alphabet_size_);
        MEMORIA_ADD_TEST_PARAM(remove_check_);

        MEMORIA_ADD_TEST(testCreate);
        MEMORIA_ADD_TEST(testRemove);

//        MEMORIA_ADD_TEST(testStore);
    }

    virtual ~WTTest() noexcept {}



    void testCreate()
    {
        auto snp = branch();
        auto ctr = create<CtrName>(snp);


        ctr->prepare();

        auto alphabet = createRandomAlphabet(alphabet_size_);
        auto text = createRandomText(size_, alphabet);

        for (uint32_t c = 0; c < text.size(); c++)
        {
            out()<<c<<" "<<hex<<text[c]<<dec<<std::endl;

            uint64_t value1 = text[c];

            ctr->insert(c, value1);

            check(MA_SRC);
        }


        assertText(*ctr.get(), text);

        auto ranks = getRankedSymbols(text);

        out()<<"Check ranks"<<std::endl;

        for (auto& rnk: ranks)
        {
            uint32_t sym = rnk.first;

            for (uint32_t c = 0; c < text.size(); c += 100)
            {
                int32_t rank1 = ctr->rank(c, sym);
                int32_t rank2 = rank(text, c, sym);

                AssertEQ(MA_SRC, rank1, rank2);
            }
        }

        out()<<"Check selects"<<std::endl;

        for (auto& rnk: ranks)
        {
            uint32_t sym = rnk.first;
            int32_t rank = rnk.second;

            for (int32_t r = 1; r <= rank; r++)
            {
                int32_t idx1 = ctr->select(r, sym);
                int32_t idx2 = select(text, r, sym);

                AssertEQ(MA_SRC, idx1, idx2);
            }
        }

        commit();

    }

    void testRemove()
    {
        auto snp = branch();
        auto ctr = create<CtrName>(snp);


        ctr->prepare();

        auto alphabet = createRandomAlphabet(alphabet_size_);
        auto text = createRandomText(size_, alphabet);

        for (uint32_t c = 0; c < text.size(); c++)
        {
            out()<<c<<" "<<hex<<text[c]<<dec<<std::endl;

            uint64_t value1 = text[c];

            ctr->insert(c, value1);
        }

        assertText(*ctr.get(), text);



        int32_t cnt = 0;
        while (ctr->size() > 0)
        {
            int32_t idx = getRandom(ctr->size());

            out()<<"Remove at "<<idx<<endl;

            ctr->remove(idx);
            text.erase(text.begin() + idx);

            if ((cnt++) % remove_check_ == 0)
            {
                assertText(*ctr.get(), text);
            }

            check(MA_SRC);
        }
    }

//    void testStore()
//    {
//      auto snp = branch();
//      auto ctr = create<CtrName>(snp);
//
//      ctr->prepare();
//
//      auto alphabet = createRandomAlphabet(alphabet_size_);
//      auto text = createRandomText(this->size_, alphabet);
//
//      for (uint32_t c = 0; c < text.size(); c++)
//      {
//          out()<<c<<" "<<hex<<text[c]<<dec<<std::endl;
//
//          uint64_t value1 = text[c];
//
//          ctr->insert(c, value1);
//      }
//
//      forceCheck(allocator, MA_SRC);
//
//      assertText(ctr, text);
//
//      allocator.commit();
//
//      StoreResource(allocator, "wts");
//
//      Allocator alloc2;
//
//      LoadResource(alloc2, "wts");
//
//      check(alloc2, MA_SRC);
//
//      Ctr wt2(&alloc2, CTR_FIND, ctr->name());
//
//      assertText(wt2, text);
//    }


    vector <uint32_t> createRandomAlphabet(int32_t size)
    {
        vector <uint32_t> text(size);

        for (auto& v: text)
        {
            v = getRandom();
        }

        return text;
    }

    vector <uint32_t> createRandomText(int32_t size, const vector<uint32_t>& alphabet)
    {
        vector <uint32_t> text(size);

        for (auto& v: text)
        {
            v = alphabet[getRandom(alphabet.size())];
        }

        return text;
    }

    vector<Pair> getRankedSymbols(const vector<uint32_t>& text)
    {
        vector<Pair> result;

        std::map<uint32_t, int32_t> ranks;

        for (uint32_t c = 0; c < text.size(); c++)
        {
            uint32_t letter = text[c];

            if (ranks.find(letter) == ranks.end())
            {
                ranks[letter] = 0;
            }

            ranks[letter]++;
        }

        for (auto pair: ranks)
        {
            result.push_back(pair);
        }

        std::sort(result.begin(), result.end(), [](const Pair& a, const Pair& b ) {
            return a.second > b.second;
        });

        return result;
    }

    int32_t rank(const vector<uint32_t>& text, int32_t idx, uint32_t symbol)
    {
        int32_t sum = 0;

        for (int32_t c = 0; c <= idx; c++)
        {
            sum += text[c] == symbol;
        }

        return sum;
    }

    int32_t select(const vector<uint32_t>& text, int32_t rank, uint32_t symbol)
    {
        for (uint32_t c = 0; c < text.size(); c++)
        {
            rank -= text[c] == symbol;

            if (rank == 0)
            {
                return c;
            }
        }

        return text.size();
    }

    void assertText(Ctr& tree, const vector<uint32_t>& text)
    {
        AssertEQ(MA_SRC, tree.size(), (int64_t)text.size());

        for (uint32_t c = 0; c < text.size(); c++)
        {
            uint32_t value = tree.value(c);

            AssertEQ(MA_SRC, value, text[c], SBuf()<<c);
        }
    }
};


}}