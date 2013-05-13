
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_VECTOR_MAP_VECTOR_MAP_TEST_HPP_
#define MEMORIA_TESTS_VECTOR_MAP_VECTOR_MAP_TEST_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>



#include <vector>

namespace memoria {

using namespace memoria::vapi;
using namespace std;




class VectorMapTest: public SPTestTask {

    typedef VectorMapTest                                                       MyType;

public:
    struct Tripple {
    	BigInt id_;
    	BigInt size_;
    	BigInt data_;

    	Tripple(): id_(0), size_(0), data_(0) {}
    	Tripple(BigInt id, BigInt size, BigInt data): id_(id), size_(size), data_(data) {}

    	BigInt id() 	const {return id_;}
    	BigInt size()	const {return size_;}
    	BigInt data()	const {return data_;}
    };
protected:
    typedef Byte																Value;

    typedef vector<Tripple>                                                     VMapData;
    typedef SCtrTF<VectorMap<BigInt, Value>>::Type                              Ctr;
    typedef Ctr::Iterator                                                       Iterator;

    typedef std::function<void (MyType*, Allocator&, Ctr&)> 					TestFn;


    VMapData tripples_;

    Int 	max_block_size_ = 1024*40;

    Int 	iteration_;
    Int     data_;
    Int     data_size_;
    String  tripples_data_file_;
    BigInt  key_;
    BigInt  key_num_;

    BigInt  ctr_name_;
    String  dump_name_;

public:

    VectorMapTest(): SPTestTask("VectorMap")
    {
        MEMORIA_ADD_TEST_PARAM(max_block_size_);

        MEMORIA_ADD_TEST_PARAM(iteration_)->state();
        MEMORIA_ADD_TEST_PARAM(data_)->state();
        MEMORIA_ADD_TEST_PARAM(data_size_)->state();
        MEMORIA_ADD_TEST_PARAM(tripples_data_file_)->state();
        MEMORIA_ADD_TEST_PARAM(key_)->state();
        MEMORIA_ADD_TEST_PARAM(key_num_)->state();
        MEMORIA_ADD_TEST_PARAM(ctr_name_)->state();
        MEMORIA_ADD_TEST_PARAM(dump_name_)->state();

        MEMORIA_ADD_TEST_WITH_REPLAY(testOrderedCreation, replayOrderedCreation);
    }

    virtual ~VectorMapTest() throw() {}

    void storeTripples(const VMapData& tripples)
    {
        String basic_name = getResourcePath("Data." + getName());

        String tripples_name       = basic_name + ".triples.txt";

        tripples_data_file_ = tripples_name;

        StoreVector(tripples, tripples_name);
    }

    VMapData loadTripples()
    {
    	VMapData tripples;
        LoadVector(tripples, tripples_data_file_);

        return tripples;
    }

    void checkDataFw(const VMapData& tripples, Ctr& map)
    {
    	BigInt total_size = 0;
    	for (auto& tripple: tripples) total_size += tripple.size();

    	AssertEQ(MA_SRC, total_size, map.total_size());
    	AssertEQ(MA_SRC, (BigInt)tripples.size(), map.size());

    	Int idx = 0;
    	for (auto iter = map.Begin(); !iter.isEnd(); idx++)
    	{
    		auto& tripple = tripples[idx];

    		BigInt id 	= iter.id();
    		BigInt size	= iter.blob_size();

    		if (isReplayMode())
    		{
    			cout<<id<<" "<<size<<endl;
    		}

    		AssertEQ(MA_SRC, id, tripple.id());
    		AssertEQ(MA_SRC, size, tripple.size());

    		iter.findData();

    		for (BigInt c = 0; c < size; c++)
    		{
    			auto value = iter.value();

    			AssertEQ(MA_SRC, (Int)value, (Int)tripple.data());

    			iter.skipFw(1);
    		}

    		iter++;
    	}
    }


    void checkDataBw(const VMapData& tripples, Ctr& map)
    {
    	BigInt total_size = 0;
    	for (auto& tripple: tripples) total_size += tripple.size();

    	AssertEQ(MA_SRC, total_size, map.total_size());
    	AssertEQ(MA_SRC, (BigInt)tripples.size(), map.size());

    	Int idx = tripples.size() - 1;
    	for (auto iter = map.RBegin(); !iter.isBegin(); idx--)
    	{
    		auto& tripple = tripples[idx];

    		BigInt id 	= iter.id();
    		BigInt size	= iter.blob_size();

    		if (isReplayMode())
    		{
    			cout<<id<<" "<<size<<endl;
    		}

    		AssertEQ(MA_SRC, id, tripple.id());
    		AssertEQ(MA_SRC, size, tripple.size());

    		iter.findData();

    		for (BigInt c = 0; c < size; c++)
    		{
    			auto value = iter.value();

    			AssertEQ(MA_SRC, (Int)value, (Int)tripple.data());

    			iter.skipFw(1);
    		}

    		iter--;
    	}
    }



    void checkBlock(Iterator& iter, BigInt id, BigInt size, Value data)
    {
    	AssertEQ(MA_SRC, iter.id(), id);
    	AssertEQ(MA_SRC, iter.blob_size(), size);

    	iter.findData(0);

		for (BigInt c = 0; c < size; c++)
		{
			auto value = iter.value();

			AssertEQ(MA_SRC, (Int)value, (Int)data);

			iter.skipFw(1);
		}
    }

    virtual void setUp()
    {
        if (btree_random_branching_)
        {
            btree_branching_ = 8 + getRandom(100);
            out()<<"BTree Branching: "<<btree_branching_<<endl;
        }

        tripples_.clear();
    }

    void test(TestFn test_fn)
    {
    	DefaultLogHandlerImpl logHandler(out());

    	Allocator allocator;
    	allocator.getLogger()->setHandler(&logHandler);

    	Ctr map(&allocator);

    	ctr_name_ = map.name();

    	allocator.commit();

    	try {
    		for (iteration_ = 0; iteration_ < size_; iteration_++)
    		{
    			test_fn(this, allocator, map);

    			allocator.commit();
    		}
    	}
    	catch (...) {
    		dump_name_ = Store(allocator);
    		storeTripples(tripples_);
    		throw;
    	}
    }


    void replay(TestFn test_fn)
    {
    	Allocator allocator;
    	DefaultLogHandlerImpl logHandler(out());
    	allocator.getLogger()->setHandler(&logHandler);

    	LoadAllocator(allocator, dump_name_);

    	tripples_ = loadTripples();

    	Ctr ctr(&allocator, CTR_FIND, ctr_name_);

    	test_fn(this, allocator, ctr);

    	check(allocator, "Insert: Container Check Failed", MA_SRC);
    }

    void testOrderedCreation()
    {
    	test(&MyType::orderedCreationTest);
    }

    void replayOrderedCreation()
    {
    	replay(&MyType::orderedCreationTest);
    }

    void orderedCreationTest(Allocator& allocator, Ctr& map)
    {
    	if (!isReplayMode())
    	{
    		data_size_  = getRandom(max_block_size_);
    		data_		= iteration_ & 0xFF;
    	}

    	vector<Byte> data = createSimpleBuffer<Byte>(data_size_, data_);

    	MemBuffer<Byte> buf(data);

    	auto iter = map.create(buf);

    	tripples_.push_back(Tripple(iter.id(), iter.blob_size(), data_));

    	try {
    		checkBlock(iter, iter.id(), data_size_, data_);

    		checkDataFw(tripples_, map);
    		checkDataBw(tripples_, map);
    	}
    	catch(...) {
    		tripples_.pop_back();
    		throw;
    	}
    }
};


static ostream& operator<<(ostream& out, const VectorMapTest::Tripple& pair)
{
    out<<pair.id()<<" "<<pair.size()<<" "<<pair.data();
    return out;
}




static istream& operator>>(istream& in, VectorMapTest::Tripple& pair)
{
	BigInt id = 0, size = 0, data = 0;

    in>>skipws;
    in>>id;

    in>>skipws;
    in>>size;

    in>>skipws;
    in>>data;

    pair = VectorMapTest::Tripple(id, size, data);

    return in;
}


}


namespace std {



}

#endif

