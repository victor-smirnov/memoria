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

#include <memoria/v1/memoria.hpp>

#include <memoria/v1/tools/tests.hpp>
#include <memoria/v1/tools/tools.hpp>

#include "ltree_test_base.hpp"

#include <vector>
#include <algorithm>
#include <sstream>
#include <memory>

namespace memoria {
namespace v1 {

using namespace v1::louds;

class LabeledTreeIterTest: public LabeledTreeTestBase {

    using MyType = LabeledTreeIterTest;

    Int iterations_ = 1000;
    Int max_degree_ = 4;

public:

    LabeledTreeIterTest(): LabeledTreeTestBase("Iter")
    {
        size_ = 60000;

        MEMORIA_ADD_TEST_PARAM(iterations_);
        MEMORIA_ADD_TEST_PARAM(max_degree_);

        MEMORIA_ADD_TEST(testIteratorCache);
    }

    virtual ~LabeledTreeIterTest() throw () {}

    void testIteratorCache()
    {
        auto snp = branch();

        auto ctr = create<CtrName>(snp);

        fillRandom(*ctr.get(), size_, max_degree_);

        Int nodes = ctr->nodes();

        auto skip_iter = ctr->seek(0);

        assertIterator(MA_SRC, skip_iter);

        out()<<"Forward skip"<<std::endl;

        while (!skip_iter->isEof())
        {
            skip_iter->next();
            assertIterator(MA_SRC, skip_iter);
        }
        out()<<std::endl;


        out()<<"Forward skip"<<std::endl;
        while (!skip_iter->isEof())
        {
            skip_iter->next();
            assertIterator(MA_SRC, skip_iter);
        }
        out()<<std::endl;

        out()<<"Random forward select/skip"<<std::endl;
        for (Int c = 0; c < iterations_; c++)
        {
            out()<<"FW: "<<c<<std::endl;

            Int node = getRandom(nodes / 2) + 1;
            auto iter = ctr->select1(node);

            assertIterator(MA_SRC, iter);

            Int skip = getRandom(nodes / 2 - 1);

            auto iter_select0   = iter->clone();
            auto iter_select1   = iter->clone();

            auto iter_skip      = iter->clone();

            iter_select0->selectFw(skip, 0);
            assertIterator(MA_SRC, iter_select0);

            iter_select1->selectFw(skip, 1);
            assertIterator(MA_SRC, iter_select1);

            iter_skip->skipFw(skip * 2);
            assertIterator(MA_SRC, iter_skip);
        }
        out()<<std::endl;

        out()<<"Random backward select/skip"<<std::endl;
        for (Int c = 0; c < iterations_; c++)
        {
            out()<<"BW: "<<c<<std::endl;

            Int node = getRandom(nodes / 2) + nodes / 2 - 1;
            auto iter = ctr->select1(node);

            assertIterator(MA_SRC, iter);

            Int skip = getRandom(nodes / 2);

            auto iter_select0   = iter->clone();
            auto iter_select1   = iter->clone();

            auto iter_skip      = iter->clone();

            iter_select0->selectBw(skip, 0);

            assertIterator(MA_SRC, iter_select0);

            iter_select1->selectBw(skip, 1);
            assertIterator(MA_SRC, iter_select1);

            iter_skip->skipBw(skip * 2);
            assertIterator(MA_SRC, iter_skip);
        }
        out()<<std::endl;

        out()<<"Random forward/backward rank"<<std::endl;
        for (Int c = 0; c < iterations_; c++)
        {
            out()<<"Rank: "<<c<<std::endl;

            Int node = getRandom(ctr->size() / 2);
            auto iter = ctr->seek(node);

            assertIterator(MA_SRC, iter);

            Int skip = getRandom(nodes / 2 - 1);

            auto iter_rankfw0   = iter->clone();
            auto iter_rankfw1   = iter->clone();

            iter_rankfw0->rank(skip, 0);
            assertIterator(MA_SRC, iter_rankfw0);

            iter_rankfw0->rank(-skip, 0);
            assertIterator(MA_SRC, iter_rankfw0);

            iter_rankfw1->rank(skip, 1);
            assertIterator(MA_SRC, iter_rankfw1);

            iter_rankfw1->rank(-skip, 1);
            assertIterator(MA_SRC, iter_rankfw1);
        }

        commit();
    }
};

}}