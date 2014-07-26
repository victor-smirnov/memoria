// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_MAP_MAP_CREATE_TEST_HPP_
#define MEMORIA_TESTS_MAP_MAP_CREATE_TEST_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include "map_test_base.hpp"

#include <vector>
#include <algorithm>
#include <sstream>
#include <memory>

namespace memoria {

template <
    typename MapName
>
class MapCreateTest: public MapTestBase<MapName> {

    typedef MapCreateTest<MapName>                                              MyType;
    typedef MapTestBase<MapName>                                                Base;

    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::Iterator                                             Iterator;
    typedef typename Base::Ctr                                                  Ctr;
    typedef typename Base::PairVector                                           PairVector;

    BigInt  key_;
    BigInt  value_;

public:

    MapCreateTest(StringRef name): Base(name)
    {
        Base::size_         = 10000;
        Base::check_step    = 0;

        MEMORIA_ADD_TEST_PARAM(key_)->state();
        MEMORIA_ADD_TEST_PARAM(value_)->state();

        MEMORIA_ADD_TEST_WITH_REPLAY(runCreateTest, replayCreateTest);
        MEMORIA_ADD_TEST_WITH_REPLAY(runIteratorTest, replayIteratorTest);
    }

    virtual ~MapCreateTest() throw () {}


    void runCreateTest()
    {
        DefaultLogHandlerImpl logHandler(Base::out());

        Allocator allocator;
        allocator.getLogger()->setHandler(&logHandler);

        Ctr map(&allocator);

        allocator.commit();

        map.setNewPageSize(4096);

        Base::ctr_name_ = map.name();

        auto& vector_idx    = Base::vector_idx_;
        auto& pairs         = Base::pairs;
        auto& pairs_sorted  = Base::pairs_sorted;

        try {

            for (vector_idx = 0; vector_idx < Base::size_; vector_idx++)
            {
                this->out()<<vector_idx<<endl;

                auto iter = map[pairs[vector_idx].key_];

                iter.svalue() = pairs[vector_idx].value_;

                Base::checkIterator(iter, MEMORIA_SOURCE);

                Base::check(allocator, MEMORIA_SOURCE);

                PairVector tmp = pairs_sorted;

                appendToSortedVector(tmp, pairs[vector_idx]);

                Base::checkContainerData(map, tmp);

                allocator.commit();

                Base::check(allocator, MEMORIA_SOURCE);

                pairs_sorted = tmp;
            }
        }
        catch (...) {
            Base::StorePairs(pairs, pairs_sorted);
            Base::dump_name_ = Base::Store(allocator);
            throw;
        }

        this->StoreAllocator(allocator, this->getResourcePath("create.dump"));
    }

    void replayCreateTest()
    {
        DefaultLogHandlerImpl logHandler(Base::out());
        Allocator allocator;
        allocator.getLogger()->setHandler(&logHandler);

        auto& vector_idx_   = Base::vector_idx_;
        auto& pairs         = Base::pairs;
        auto& pairs_sorted  = Base::pairs_sorted;

        Base::LoadAllocator(allocator, Base::dump_name_);

        LoadVector(pairs, Base::pairs_data_file_);
        LoadVector(pairs_sorted, Base::pairs_sorted_data_file_);

        Ctr map(&allocator, CTR_FIND, Base::ctr_name_);

        auto iter = map[pairs[vector_idx_].key_];
        iter.svalue() = pairs[vector_idx_].value_;

        Base::checkIterator(iter, MEMORIA_SOURCE);

        Base::check(allocator, MEMORIA_SOURCE);

        appendToSortedVector(pairs_sorted, pairs[vector_idx_]);

        Base::checkContainerData(map, pairs_sorted);

        allocator.commit();

        Base::check(allocator, MEMORIA_SOURCE);
    }

    vector<Int> fillVector(Allocator& allocator, Ctr& ctr, Int size)
    {
        vector<Int> v;

        for (int c = 1; c <= size; c++)
        {
        	allocator.commit();

        	key_ = value_ = c;

            Base::out()<<c<<endl;

            ctr[c] = c;

            v.push_back(c);

            auto i = ctr[c];

            AssertEQ(MA_SRC, i.key(), c);
            AssertEQ(MA_SRC, i.value(), c);
        }

        return v;
    }


    void runIteratorTest()
    {
        DefaultLogHandlerImpl logHandler(Base::out());
        Allocator allocator;
        allocator.getLogger()->setHandler(&logHandler);

        allocator.commit();

        try {
            Ctr ctr(&allocator);

            Base::ctr_name_ = ctr.name();

            vector<Int> v = fillVector(allocator, ctr, Base::size_);

            Iterator i1 = ctr.Begin();
            AssertEQ(MA_SRC, i1.key(), v[0]);

            Iterator i2 = ctr.RBegin();
            AssertEQ(MA_SRC, i2.key(), v[v.size() - 1]);

            Iterator i3 = ctr.End();
            AssertTrue(MA_SRC, i3.isEnd());
            AssertEQ(MA_SRC, i3.idx(), ctr.getNodeSize(i3.leaf(), 0));

            Iterator i4 = ctr.REnd();
            AssertTrue(MA_SRC, i4.isBegin());
            AssertEQ(MA_SRC, i4.idx(), -1);

            allocator.commit();
        }
        catch (...) {
            Base::dump_name_ = Base::Store(allocator);
            throw;
        }

        this->StoreAllocator(allocator, this->getResourcePath("iterator.dump"));
    }

    void replayIteratorTest()
    {
        DefaultLogHandlerImpl logHandler(Base::out());
        Allocator allocator;
        allocator.getLogger()->setHandler(&logHandler);

        Base::LoadAllocator(allocator, Base::dump_name_);

        Base::check(allocator, MA_SRC);

        Ctr ctr(&allocator, CTR_FIND, Base::ctr_name_);

        ctr[key_] = value_;

        auto i = ctr[key_];

        AssertEQ(MA_SRC, i.key(), key_);
        AssertEQ(MA_SRC, i.value(), value_);
    }

};

}

#endif
