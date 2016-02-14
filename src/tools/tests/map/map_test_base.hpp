// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_MAP_MAP_TEST_BASE_HPP_
#define MEMORIA_TESTS_MAP_MAP_TEST_BASE_HPP_

#include <memoria/memoria.hpp>

#include <memoria/containers/map/map_factory.hpp>

#include <memoria/tools/profile_tests.hpp>

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include "../prototype/btss/btss_test_base.hpp"

#include <vector>
#include <algorithm>
#include <sstream>
#include <memory>
#include <tuple>

namespace memoria {

namespace {

template <typename T>
struct RNGTool {
	template <typename Test>
	static T next(const Test* test) {
		return test->getBIRandom();
	}
};


template <>
struct RNGTool<UUID> {
	template <typename Test>
	static UUID next(const Test* test) {
		return UUID::make_random();
	}
};

}


template<typename MapName>
class MapTestBase: public BTSSTestBase<MapName, PersistentInMemAllocator<>, DefaultProfile<>> {
    using MyType = MapTestBase<MapName>;
    using Base = BTSSTestBase<MapName, PersistentInMemAllocator<>, DefaultProfile<>>;

public:
    using typename Base::Ctr;
    using typename Base::Iterator;

    using Base::out;

    using Key 		= typename Ctr::Types::Key;
    using Value 	= typename Ctr::Types::Value;

    using Pair = KVPair<Key, Value>;



protected:
    typedef vector<Pair> PairVector;

    PairVector pairs;
    PairVector pairs_sorted;

    Int vector_idx_;
    Int step_;

    UUID ctr_name_;

    String pairs_data_file_;
    String pairs_sorted_data_file_;

    Int iterator_check_count_ = 100;
    Int iterator_check_counter_ = 0;

    Int data_check_count_ = 100;
    Int data_check_counter_ = 0;

    BigInt size_;

    bool throw_ex_ = false;

public:

    MapTestBase(StringRef name): Base(name)
    {
        Ctr::initMetadata();

        size_ = 10000;


        MEMORIA_ADD_TEST_PARAM(size_);
        MEMORIA_ADD_TEST_PARAM(throw_ex_);

        MEMORIA_ADD_TEST_PARAM(vector_idx_)->state();
        MEMORIA_ADD_TEST_PARAM(step_)->state();

        MEMORIA_ADD_TEST_PARAM(ctr_name_)->state();
        MEMORIA_ADD_TEST_PARAM(pairs_data_file_)->state();
        MEMORIA_ADD_TEST_PARAM(pairs_sorted_data_file_)->state();

        MEMORIA_ADD_TEST_PARAM(iterator_check_counter_)->state();
        MEMORIA_ADD_TEST_PARAM(iterator_check_count_)->state();

        MEMORIA_ADD_TEST_PARAM(data_check_counter_)->state();
        MEMORIA_ADD_TEST_PARAM(data_check_count_)->state();
    }

    virtual ~MapTestBase() throw () {
    }

    Key getRandomKey()
    {
    	return RNGTool<Key>::next(this);
    }

    template <typename CtrT>
    void checkContainerData(const std::shared_ptr<CtrT>& map, PairVector& pairs)
    {
        if (data_check_counter_ % data_check_count_ == 0)
        {
            Int pairs_size = (Int) pairs.size();

            Int idx = 0;
            for (auto iter = map->begin(); !iter.is_end();)
            {
                auto key = iter.key();

                auto value = iter.value();

                if (pairs[idx].key_ != key) {
                    iter.dump();
                }

                if (pairs[idx].value_ != value) {
                    iter.dump();
                }

                AssertEQ(MA_SRC, pairs[idx].key_, key);
                AssertEQ(MA_SRC, pairs[idx].value_, value);

                iter++;
                idx++;
            }

            AssertEQ(MA_SRC, idx, pairs_size);
        }

        data_check_counter_++;
    }

    virtual void checkIterator(Iterator& iter, const char* source)
    {
    	auto cache1 = iter.cache();

    	auto tmp = iter;
    	tmp.refresh();

    	auto cache2 = tmp.cache();

        if (cache1 != cache2)
        {
            iter.dump(out());
            throw TestException(source, SBuf()<<"Invalid iterator cache. Iterator: "<<cache1<<" Actual: "<<cache2);
        }
    }

    virtual Key makeRandomKey() = 0;
    virtual Value makeRandomValue() = 0;


    virtual void setUp()
    {
    	Base::setUp();

    	if (!this->isReplayMode())
    	{
            pairs.clear();
            pairs_sorted.clear();

            for (Int c = 0; c < size_; c++)
            {
                pairs.push_back(Pair(makeRandomKey(), makeRandomValue()));
            }
    	}
    	else {
    		LoadVector(pairs, pairs_data_file_);
    		LoadVector(pairs_sorted, pairs_sorted_data_file_);
    	}
    }

    virtual void onException() noexcept
    {
    	Base::onException();

    	String basic_name = "Data." + this->getName();

    	String pairs_name = basic_name + ".pairs.txt";
    	pairs_data_file_ = this->getResourcePath(pairs_name);

    	StoreVector(pairs, pairs_data_file_);

    	String pairs_sorted_name = basic_name + ".pairs_sorted.txt";
    	pairs_sorted_data_file_ = this->getResourcePath(pairs_sorted_name);

    	StoreVector(pairs_sorted, pairs_sorted_data_file_);
    }
};

}

#endif
