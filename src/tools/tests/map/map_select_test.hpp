// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_MAP_MAP_SELECT_TEST_HPP_
#define MEMORIA_TESTS_MAP_MAP_SELECT_TEST_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include "map_test_base.hpp"

#include <vector>
#include <algorithm>
#include <sstream>
#include <memory>
#include <map>

namespace memoria {

using namespace std;

template <
    typename MapName
>
class MapSelectTest: public MapTestBase<MapName> {

    typedef MapSelectTest<MapName>                                              MyType;
    typedef MapTestBase<MapName>                                                Base;

    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::Iterator                                             Iterator;
    typedef typename Base::Ctr                                                  Ctr;
    typedef typename Base::PairVector                                           PairVector;

    BigInt  key_;
    BigInt  value_;

public:

    MapSelectTest(StringRef name): Base(name)
    {
        Base::size_         = 10000;
        Base::check_step    = 0;
        Base::check_count   = 1;

        MEMORIA_ADD_TEST_PARAM(key_)->state();
        MEMORIA_ADD_TEST_PARAM(value_)->state();

        MEMORIA_ADD_TEST(runSelectTest);

    }

    virtual ~MapSelectTest() throw () {}


    void runSelectTest()
    {
        DefaultLogHandlerImpl logHandler(Base::out());

        Allocator allocator;
        allocator.getLogger()->setHandler(&logHandler);

        Ctr map(&allocator);

        allocator.commit();

        map.setNewPageSize(4096);

        Base::ctr_name_ = map.name();

        std::map<BigInt, BigInt> marked_keys;

        Int mark_step = 100;

        for (Int c = 1; c <= this->size_; c++)
        {
            Int key = getRandom();

            auto iter = map[key];

            iter.setData(c);

            if (c % mark_step == 0)
            {
                iter.setMark(1);

                marked_keys[key] = c;
            }

            this->check(allocator, MA_SRC);
        }

        allocator.commit();

        this->StoreAllocator(allocator, this->getResourcePath("select.dump"));

        assertMap(map, marked_keys);

        try {
            while(marked_keys.size() > 0)
            {
                auto tgt_pos = getRandom(marked_keys.size());

                auto i = marked_keys.begin();
                for (Int c = 0; c < tgt_pos; c++, i++);

                MEMORIA_ASSERT_TRUE(i != marked_keys.end());

                BigInt key = i->first;
                this->out()<<"Remove: "<<key<<" at "<<tgt_pos<<std::endl;
                marked_keys.erase(key);

                auto iter = map.find(key);

                AssertFalse(MA_SRC, iter.isEnd());
                AssertEQ(MA_SRC, iter.key(), key);
                AssertEQ(MA_SRC, iter.mark(), 1);

                iter.remove();

                this->check(allocator, MA_SRC);

                assertMap(map, marked_keys);

                allocator.commit();
            }
        }
        catch (...) {
            this->dump_name_ = this->Store(allocator);
            throw;
        }
    }


    void assertMap(Ctr& map, std::map<BigInt, BigInt>& marked_keys)
    {
        auto iter = map.Begin();

        UInt total_marks = 0;

        while (!iter.isEnd())
        {
            auto tmp = iter;

            iter.selectFw(1, 1);

            if (!iter.isEnd())
            {
                total_marks++;

                BigInt key      = iter.key();
                BigInt value    = iter.value();

                Int mark        = iter.mark();

                AssertEQ(MA_SRC, mark, 1);

                AssertTrue(MA_SRC, marked_keys.find(key) != marked_keys.end());

                AssertEQ(MA_SRC, value, marked_keys[key]);

                iter++;
            }
            else if (total_marks < marked_keys.size())
            {
                tmp.dumpPath();
                tmp.selectFw(1, 1);
                tmp.dumpPath();
            }
        }

        this->out()<<"Total marks: "<<total_marks<<std::endl;

        AssertEQ(MA_SRC, total_marks, marked_keys.size());
    }
};

}

#endif
