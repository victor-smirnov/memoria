// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_MAP_MAP_TEST_BASE_HPP_
#define MEMORIA_TESTS_MAP_MAP_TEST_BASE_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/profile_tests.hpp>

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include <vector>
#include <algorithm>
#include <sstream>
#include <memory>
#include <tuple>

namespace memoria {

template<typename MapName>
class MapTestBase: public SPTestTask {
    typedef MapTestBase MyType;

public:
    typedef KVPair<BigInt, BigInt> Pair;

protected:
    typedef vector<Pair> PairVector;
    typedef typename DCtrTF<MapName>::Type Ctr;
    typedef typename Ctr::Iterator Iterator;
    typedef typename Ctr::ID ID;
    typedef typename Ctr::Accumulator Accumulator;

    PairVector pairs;
    PairVector pairs_sorted;

    Int vector_idx_;
    Int step_;

    BigInt ctr_name_;
    String dump_name_;
    String pairs_data_file_;
    String pairs_sorted_data_file_;

    Int iterator_check_count_ = 100;
    Int iterator_check_counter_ = 0;

    Int data_check_count_ = 100;
    Int data_check_counter_ = 0;

    bool throw_ex_ = false;

public:

    MapTestBase(StringRef name): SPTestTask(name)
    {
        Ctr::initMetadata();

        size_ = 10000;

        MEMORIA_ADD_TEST_PARAM(throw_ex_);

        MEMORIA_ADD_TEST_PARAM(vector_idx_)->state();
        MEMORIA_ADD_TEST_PARAM(step_)->state();

        MEMORIA_ADD_TEST_PARAM(ctr_name_)->state();
        MEMORIA_ADD_TEST_PARAM(dump_name_)->state();
        MEMORIA_ADD_TEST_PARAM(pairs_data_file_)->state();
        MEMORIA_ADD_TEST_PARAM(pairs_sorted_data_file_)->state();

        MEMORIA_ADD_TEST_PARAM(iterator_check_counter_)->state();
        MEMORIA_ADD_TEST_PARAM(iterator_check_count_)->state();

        MEMORIA_ADD_TEST_PARAM(data_check_counter_)->state();
        MEMORIA_ADD_TEST_PARAM(data_check_count_)->state();
    }

    virtual ~MapTestBase() throw () {
    }

    void checkContainerData(Ctr& map, PairVector& pairs) {
        if (data_check_counter_ % data_check_count_ == 0) {

            Int pairs_size = (Int) pairs.size();

            Int idx = 0;
            for (auto iter = map.Begin(); !iter.isEnd();) {
                BigInt key = iter.key();

                BigInt value = iter.value();

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

//            idx = pairs_size - 1;
//            for (auto iter = map.RBegin(); !iter.isBegin();)
//            {
//                BigInt key = iter.key();
//                BigInt value = iter.value();
//
//                AssertEQ(MA_SRC, pairs[idx].key_, key, SBuf()<<idx);
//                AssertEQ(MA_SRC, pairs[idx].value_, value, SBuf()<<idx);
//
//                iter--;
//
//                idx--;
//            }
//
//            AssertEQ(MA_SRC,idx, -1, SBuf()<<"pairs_size="<<pairs_size);
        }

        data_check_counter_++;
    }

    void StorePairs(const PairVector& pairs, const PairVector& pairs_sorted)
    {
        String basic_name = "Data." + getName();

        String pairs_name = basic_name + ".pairs.txt";
        pairs_data_file_ = getResourcePath(pairs_name);

        StoreVector(pairs, pairs_data_file_);

        String pairs_sorted_name = basic_name + ".pairs_sorted.txt";
        pairs_sorted_data_file_ = getResourcePath(pairs_sorted_name);

        StoreVector(pairs_sorted, pairs_sorted_data_file_);
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


    virtual void setUp()
    {
        pairs.clear();
        pairs_sorted.clear();

        for (Int c = 0; c < size_; c++)
        {
            pairs.push_back(Pair(getUniqueBIRandom(pairs, 1000000), getBIRandom(100000)));
        }
    }
};

}

#endif
