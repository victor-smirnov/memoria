
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_VECTOR_MAP_VECTOR_MAP_TEST_HPP_
#define MEMORIA_TESTS_VECTOR_MAP_VECTOR_MAP_TEST_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>


#include "../shared/params.hpp"


#include <vector>

namespace memoria {

using namespace memoria::vapi;
using namespace std;




class VectorMapTest: public SPTestTask {

    typedef VectorMapTest                                               MyType;

    typedef KVPair<BigInt, BigInt>                                      Pair;
    typedef vector<Pair>                                                PairVector;
    typedef SmallCtrTypeFactory::Factory<VectorMapCtr>::Type            VectorMapCtrType;
    typedef VectorMapCtrType::Iterator                                  VMIterator;


    PairVector pairs_;

    Int max_block_size_ = 1024*40;

    Int     data_;
    Int     step_;
    Int     data_size_;
    String  pairs_data_file_;
    BigInt  key_;
    BigInt  key_num_;

    BigInt  ctr_name_;
    String  dump_name_;

public:

    VectorMapTest(): SPTestTask("VectorMap")
    {
        VectorMapCtrType::initMetadata();

        MEMORIA_ADD_TEST_PARAM(max_block_size_);

        MEMORIA_ADD_TEST_PARAM(step_)->state();
        MEMORIA_ADD_TEST_PARAM(data_)->state();
        MEMORIA_ADD_TEST_PARAM(data_size_)->state();
        MEMORIA_ADD_TEST_PARAM(pairs_data_file_)->state();
        MEMORIA_ADD_TEST_PARAM(key_)->state();
        MEMORIA_ADD_TEST_PARAM(key_num_)->state();
        MEMORIA_ADD_TEST_PARAM(ctr_name_)->state();
        MEMORIA_ADD_TEST_PARAM(dump_name_)->state();

        MEMORIA_ADD_TEST_WITH_REPLAY(runTest, runReplay);
    }

    virtual ~VectorMapTest() throw() {}

    void StorePairs(const PairVector& pairs)
    {
        String basic_name = getResourcePath("Data." + getName());

        String pairs_name       = basic_name + ".pairs.txt";

        pairs_data_file_ = pairs_name;

        StoreVector(pairs, pairs_name);
    }

    void checkIteratorFw(VectorMapCtrType& ctr)
    {
        MEMORIA_TEST_THROW_IF(ctr.count(), != , (BigInt)pairs_.size());

        auto iter = ctr.Begin();

        Int c = 0;
        for (Pair& pair: pairs_)
        {
            MEMORIA_TEST_THROW_IF_1(iter.getKey(), != , pair.key_, c);
            MEMORIA_TEST_THROW_IF_1(iter.size(),   != , pair.value_, c);
            MEMORIA_TEST_THROW_IF_1(iter.pos(),    != , 0, c);

            UByte value = pair.key_ & 0xFF;

            ArrayData<UByte> data = createBuffer(pair.value_, value);

            checkBufferWritten(iter, data, "Buffer written does not match", MEMORIA_SOURCE);

            iter.next();
            c++;
        }
    }

    void runReplay(ostream& out)
    {
        pairs_.clear();

        DefaultLogHandlerImpl logHandler(out);

        Allocator allocator;
        allocator.getLogger()->setHandler(&logHandler);

        LoadAllocator(allocator, dump_name_);

        VectorMapCtrType map(&allocator, ctr_name_);

        if (step_ == 0)
        {

        }
        else if (step_ == 1)
        {
            LoadVector(pairs_, pairs_data_file_);

            auto iter = map.create(key_);

            checkCtr(map, "insertion failed 1", MEMORIA_SOURCE);

            ArrayData<UByte> data = createBuffer(data_size_, key_ & 0xFF);

            iter.insert(data);

            appendToSortedVector(pairs_, Pair(key_, data_size_));

            checkCtr(map, "insertion failed 2", MEMORIA_SOURCE);

            MEMORIA_TEST_THROW_IF(iter.size(),      != , data.size());
            MEMORIA_TEST_THROW_IF(iter.getKey(),    != , key_);

            auto iter2 = map.find(iter.getKey());

            MEMORIA_TEST_THROW_IF(iter2.exists(), != , true);
            MEMORIA_TEST_THROW_IF(iter2.size(),   != , data.size());
            MEMORIA_TEST_THROW_IF(iter2.getKey(), != , iter.getKey());

            checkBufferWritten(iter2, data, "Buffer written does not match", MEMORIA_SOURCE);

            checkIteratorFw(map);
        }
        else {
            LoadVector(pairs_, pairs_data_file_);

            MEMORIA_TEST_THROW_IF(map.count(), != , (BigInt)pairs_.size());

            bool removed        = map.remove(key_);

            MEMORIA_TEST_THROW_IF(removed, != , true);

            checkCtr(map, "remove failed.",     MEMORIA_SOURCE);

            pairs_.erase(pairs_.begin() + key_num_);

            checkIteratorFw(map);
        }
    }

    virtual void setUp(ostream out)
    {
        if (btree_random_branching_)
        {
            btree_branching_ = 8 + getRandom(100);
            out<<"BTree Branching: "<<btree_branching_<<endl;
        }

        pairs_.clear();
    }

    void runTest(ostream& out)
    {
        TestOrderedCreation(out);
        TestRandomCreation(out);
        TestRandomDeletion(out);
    }

    void TestOrderedCreation(ostream& out)
    {
        out<<"OrderedCreation Test"<<endl;

        DefaultLogHandlerImpl logHandler(out);

        Allocator allocator;
        allocator.getLogger()->setHandler(&logHandler);

        VectorMapCtrType map(&allocator);

        ctr_name_ = map.name();

        allocator.commit();

        try {

            step_ = 0;

            UByte value = 0;

            Int total_size = 0;

            for (Int c = 0; c < size_; c++, value++)
            {
                auto iter = map.create();

                data_size_ = getRandom(max_block_size_);

                ArrayData<UByte> data = createBuffer(data_size_, c % 256);

                iter.insert(data);

                MEMORIA_TEST_THROW_IF(iter.size(),   != , data.size());
                MEMORIA_TEST_THROW_IF(iter.getKey(), != , c + 1);

                total_size += iter.size();

                checkCtr(map, "insertion failed.",  MEMORIA_SOURCE);

                MEMORIA_TEST_THROW_IF(map.array().size(), != , total_size);

                auto iter2 = map.find(iter.getKey());

                MEMORIA_TEST_THROW_IF(iter2.exists(), != , true);
                MEMORIA_TEST_THROW_IF(iter2.size(),   != , data.size());
                MEMORIA_TEST_THROW_IF(iter2.getKey(), != , iter.getKey());

                checkBufferWritten(iter2, data, "Buffer written does not match", MEMORIA_SOURCE);

                allocator.commit();
            }
        }
        catch (...) {
            dump_name_ = Store(allocator);
            throw;
        }
    }


    void TestRandomCreation(ostream& out)
    {
        out<<"RandomCreation test"<<endl;

        DefaultLogHandlerImpl logHandler(out);

        Allocator allocator;
        allocator.getLogger()->setHandler(&logHandler);

        VectorMapCtrType map(&allocator);

        ctr_name_ = map.name();

        PairVector pairs_tmp;

        pairs_.clear();

        try {
            step_ = 1;

            Int total_size = 0;

            for (Int c = 0; c < size_; c++)
            {
                key_ = getUniqueRandom(pairs_);



                auto iter = map.create(key_);
                data_size_ = getRandom(max_block_size_);

                ArrayData<UByte> data = createBuffer(data_size_, key_ & 0xFF);
                iter.insert(data);

                checkCtr(map, "insertion failed.",  MEMORIA_SOURCE);

                key_num_ = appendToSortedVector(pairs_, Pair(key_, data_size_));

                MEMORIA_TEST_THROW_IF(iter.size(),      != , data.size());
                MEMORIA_TEST_THROW_IF(iter.getKey(),    != , key_);

                total_size += iter.size();

                MEMORIA_TEST_THROW_IF(map.array().size(), != , total_size);

                auto iter2 = map.find(iter.getKey());

                MEMORIA_TEST_THROW_IF(iter2.exists(), != , true);
                MEMORIA_TEST_THROW_IF(iter2.size(),   != , data.size());
                MEMORIA_TEST_THROW_IF(iter2.getKey(), != , iter.getKey());

                checkBufferWritten(iter2, data, "Buffer written does not match", MEMORIA_SOURCE);

                checkIteratorFw(map);

                allocator.commit();

                appendToSortedVector(pairs_tmp, Pair(key_, data_size_));
            }

            MEMORIA_TEST_THROW_IF(map.count(), != , (BigInt)pairs_.size());
        }
        catch (...)
        {
            StorePairs(pairs_tmp);
            dump_name_ = Store(allocator);

            throw;
        }
    }


    void TestRandomDeletion(ostream& out)
    {
        out<<"RandomDeletion Test"<<endl;

        DefaultLogHandlerImpl logHandler(out);

        Allocator allocator;
        allocator.getLogger()->setHandler(&logHandler);

        VectorMapCtrType map(&allocator);

        ctr_name_ = map.name();

        PairVector pairs_tmp;

        pairs_.clear();

        try {
            step_ = 2;

            for (Int c = 0; c < size_; c++)
            {
                key_ = getUniqueRandom(pairs_);

                auto iter = map.create(key_);
                data_size_ = getRandom(max_block_size_);

                ArrayData<UByte> data = createBuffer(data_size_, key_ & 0xFF);
                iter.insert(data);

                appendToSortedVector(pairs_, Pair(key_, data_size_));
            }

            allocator.commit();

            while (map.count() > 0)
            {
                pairs_tmp = pairs_;

                Int idx = getRandom(map.count());

                key_num_    = idx;
                key_        = pairs_[idx].key_;

                bool removed = map.remove(pairs_[idx].key_);

                MEMORIA_TEST_THROW_IF(removed, != , true);

                checkCtr(map, "remove failed.",     MEMORIA_SOURCE);

                pairs_.erase(pairs_.begin() + idx);

                checkIteratorFw(map);
                allocator.commit();
            }

        }
        catch (...)
        {
            StorePairs(pairs_tmp);
            dump_name_ = Store(allocator);
            throw;
        }
    }
};


}


#endif

