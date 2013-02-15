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

class MapCreateTest: public MapTestBase {

    typedef MapCreateTest                                                       MyType;

    BigInt 	key_;
    BigInt 	value_;

public:

    MapCreateTest(): MapTestBase("Create")
    {
    	MEMORIA_ADD_TEST_PARAM(key_)->state();
    	MEMORIA_ADD_TEST_PARAM(value_)->state();

        MEMORIA_ADD_TEST_WITH_REPLAY(runCreateTest, replayCreateTest);
        MEMORIA_ADD_TEST_WITH_REPLAY(runIteratorTest, replayIteratorTest);
    }

    virtual ~MapCreateTest() throw () {}


    void runCreateTest()
    {
        DefaultLogHandlerImpl logHandler(out());

        Allocator allocator;
        allocator.getLogger()->setHandler(&logHandler);

        Ctr map(&allocator);

        map.setNewPageSize(8192);

        ctr_name_ = map.name();

        map.setBranchingFactor(btree_branching_);

        try {
            for (vector_idx_ = 0; vector_idx_ < size_; vector_idx_++)
            {
                auto iter = map[pairs[vector_idx_].key_];
                iter.setData(pairs[vector_idx_].value_);

                checkIterator(iter, MEMORIA_SOURCE);

                check(allocator, MEMORIA_SOURCE);

                PairVector tmp = pairs_sorted;

                appendToSortedVector(tmp, pairs[vector_idx_]);

                checkContainerData(map, tmp);

                allocator.commit();

                check(allocator, MEMORIA_SOURCE);

                pairs_sorted = tmp;
            }
        }
        catch (...) {
            StorePairs(pairs, pairs_sorted);
            dump_name_ = Store(allocator);
            throw;
        }
    }

    void replayCreateTest()
    {
        DefaultLogHandlerImpl logHandler(out());
        Allocator allocator;
        allocator.getLogger()->setHandler(&logHandler);

        LoadAllocator(allocator, dump_name_);

        LoadVector(pairs, pairs_data_file_);
        LoadVector(pairs_sorted, pairs_sorted_data_file_);

        Ctr map(&allocator, CTR_FIND, ctr_name_);

        auto iter = map[pairs[vector_idx_].key_];
        iter.setData(pairs[vector_idx_].value_);

        checkIterator(iter, MEMORIA_SOURCE);

        check(allocator, MEMORIA_SOURCE);

        appendToSortedVector(pairs_sorted, pairs[vector_idx_]);

        checkContainerData(map, pairs_sorted);

        allocator.commit();

        check(allocator, MEMORIA_SOURCE);
    }

    vector<Int> fillVector(Allocator& allocator, Ctr& ctr, Int size)
    {
    	vector<Int> v;

    	for (int c = 0; c < size; c++)
    	{
    		key_ = value_ = c;

    		ctr[c] = c;
    		v.push_back(c);

    		auto i = ctr[c];

    		AssertEQ(MA_SRC, i.key(), c);
    		AssertEQ(MA_SRC, i.value(), c);

    		allocator.commit();
    	}

    	return v;
    }


    void runIteratorTest()
    {
    	DefaultLogHandlerImpl logHandler(out());
    	Allocator allocator;
    	allocator.getLogger()->setHandler(&logHandler);

    	try {
    		Ctr ctr(&allocator);

    		ctr_name_ = ctr.name();

    		vector<Int> v = fillVector(allocator, ctr, 10000);

    		allocator.commit();

    		Iterator i1 = ctr.Begin();
    		AssertEQ(MA_SRC, i1.key(), v[0]);

    		Iterator i2 = ctr.RBegin();
    		AssertEQ(MA_SRC, i2.key(), v[v.size() - 1]);

    		Iterator i3 = ctr.End();
    		AssertTrue(MA_SRC, i3.isEnd());
    		AssertEQ(MA_SRC, i3.key_idx(), i3.page()->children_count());

    		Iterator i4 = ctr.REnd();
    		AssertTrue(MA_SRC, i4.isBegin());
    		AssertEQ(MA_SRC, i4.key_idx(), -1);
    	}
    	catch (...) {
    		dump_name_ = Store(allocator);
    		throw;
    	}
    }

    void replayIteratorTest()
    {
    	DefaultLogHandlerImpl logHandler(out());
    	Allocator allocator;
    	allocator.getLogger()->setHandler(&logHandler);

    	LoadAllocator(allocator, dump_name_);

    	Ctr ctr(&allocator, CTR_FIND, ctr_name_);

    	ctr[key_] = value_;

		auto i = ctr[key_];

		AssertEQ(MA_SRC, i.key(), key_);
		AssertEQ(MA_SRC, i.value(), value_);
    }

};

}

#endif
