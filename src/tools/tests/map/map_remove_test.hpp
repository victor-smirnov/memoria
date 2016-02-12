// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_MAP_MAP_REMOVE_TEST_HPP_
#define MEMORIA_TESTS_MAP_MAP_REMOVE_TEST_HPP_

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
class MapRemoveTest: public MapTestBase<MapName> {

    using MyType = MapRemoveTest<MapName>;
    using Base = MapTestBase<MapName>;

    using typename Base::Allocator;
    using typename Base::Iterator;
    using typename Base::Pair;
    using typename Base::PairVector;
    using typename Base::Key;
    using typename Base::Value;

    using Base::size_;
    using Base::ctr_name_;
    using Base::vector_idx_;
    using Base::pairs;
    using Base::pairs_sorted;

    using Base::commit;
    using Base::drop;
    using Base::branch;
    using Base::allocator;
    using Base::snapshot;
    using Base::check;
    using Base::checkContainerData;
    using Base::out;


    bool map_creation_ = false;

public:

    MapRemoveTest(StringRef name): Base(name)
    {
        Base::size_      = 10000;
        Base::check_step = 1;

        MEMORIA_ADD_TEST_PARAM(map_creation_)->state();

        MEMORIA_ADD_TEST_WITH_REPLAY(runRemoveTest, replayRemoveTest);
    }

    virtual ~MapRemoveTest() throw () {}

    virtual Key makeRandomKey() {
    	return Key::make_random();
    }

    virtual Value makeRandomValue() {
    	return this->getBIRandom();
    }

    void runRemoveTest()
    {
        auto snp = branch();

        auto map = create<MapName>(snp);

        ctr_name_ = map->name();

        map_creation_ = true;

        PairVector tmp = pairs;

        for (vector_idx_ = 0; vector_idx_ < size_; vector_idx_++)
        {
        	auto key 	= pairs[vector_idx_].key_;
        	auto value	= pairs[vector_idx_].value_;

        	map->assign(key, value);
        }

        check(snapshot(), MEMORIA_SOURCE);

        commit();
        snp = branch();

        map = find<MapName>(snp, ctr_name_);

        std::sort(tmp.begin(), tmp.end());
        checkContainerData(map, tmp);

        for (auto iter = map->begin(); iter != map->endm(); iter++)
        {
        	pairs_sorted.push_back(Pair(iter.key(), iter.value()));
        }

        map_creation_ = false;

        for (vector_idx_ = 0; vector_idx_ < size_; vector_idx_++)
        {
        	bool result = map->remove(pairs[vector_idx_].key_);

        	AssertTrue(MA_SRC, result);

        	check(snp, MA_SRC);

        	out()<<vector_idx_<<endl;

        	BigInt size = size_ - vector_idx_ - 1;

        	AssertEQ(MA_SRC, size, map->size());

        	PairVector pairs_sorted_tmp = pairs_sorted;

        	for (UInt x = 0; x < pairs_sorted_tmp.size(); x++)
        	{
        		if (pairs_sorted_tmp[x].key_ == pairs[vector_idx_].key_)
        		{
        			pairs_sorted_tmp.erase(pairs_sorted_tmp.begin() + x);
        		}
        	}

        	checkContainerData(map, pairs_sorted_tmp);

        	commit();
        	snp = branch();
        	map = find<MapName>(snp, ctr_name_);

        	check(snp, MA_SRC);

        	pairs_sorted = pairs_sorted_tmp;
        }

        if (snapshot()->is_active())
        {
        	commit();
        }
    }

    void replayRemoveTest()
    {
    	auto snp = branch();

    	auto map = find<MapName>(snp, ctr_name_);

    	checkContainerData(map, pairs_sorted);

        auto key = pairs[vector_idx_].key_;
        bool result = map->remove(key);

        AssertTrue(MA_SRC, result);

        check(snp, MA_SRC);

        BigInt size = size_ - vector_idx_ - 1;

        AssertEQ(MA_SRC, size, map->size());

        for (UInt x = 0; x < pairs_sorted.size(); x++)
        {
        	if (pairs_sorted[x].key_ == pairs[vector_idx_].key_)
        	{
        		pairs_sorted.erase(pairs_sorted.begin() + x);
        	}
        }

        commit();
    }

};

}

#endif
