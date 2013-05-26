
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_VECTOR_MAP_VECTORMAP_TEST_BASE_HPP_
#define MEMORIA_TESTS_VECTOR_MAP_VECTORMAP_TEST_BASE_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>



#include <vector>

namespace memoria {

using namespace memoria::vapi;
using namespace std;




class VectorMapTestBase: public SPTestTask {

    typedef VectorMapTestBase                                                   MyType;

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
    typedef BigInt																Value;

    typedef vector<Tripple>                                                     VMapData;
    typedef SCtrTF<VectorMap<BigInt, Value>>::Type                              Ctr;
    typedef Ctr::Iterator                                                       Iterator;

    typedef std::function<void (MyType*, Allocator&, Ctr&)> 					TestFn;


    VMapData tripples_;

    Int 	max_block_size_ 		= 1024*40;
    bool	check_data_				= false;
    Int 	iterator_check_count_	= 1;

    Int 	iteration_;
    Int     data_;
    Int     data_size_;
    String  tripples_data_file_;
    BigInt  key_;
    BigInt  key_num_;
    Int 	iterator_check_counter_	= 0;



    BigInt  ctr_name_;
    String  dump_name_;

public:

    VectorMapTestBase(StringRef name): SPTestTask(name)
    {
        MEMORIA_ADD_TEST_PARAM(max_block_size_);
        MEMORIA_ADD_TEST_PARAM(check_data_);
        MEMORIA_ADD_TEST_PARAM(iterator_check_count_);

        MEMORIA_ADD_TEST_PARAM(iteration_)->state();
        MEMORIA_ADD_TEST_PARAM(data_)->state();
        MEMORIA_ADD_TEST_PARAM(data_size_)->state();
        MEMORIA_ADD_TEST_PARAM(tripples_data_file_)->state();
        MEMORIA_ADD_TEST_PARAM(key_)->state();
        MEMORIA_ADD_TEST_PARAM(key_num_)->state();
        MEMORIA_ADD_TEST_PARAM(ctr_name_)->state();
        MEMORIA_ADD_TEST_PARAM(dump_name_)->state();
        MEMORIA_ADD_TEST_PARAM(iterator_check_counter_)->state();
    }

    virtual ~VectorMapTestBase() throw() {}

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

    VMapData createRandomVMap(Ctr& map, Int size)
    {
    	VMapData tripples;

    	for (Int c = 0; c < size; c++)
    	{
    		Int	data_size 	= getRandom(max_block_size_);
    		Int	data		= c & 0xFF;
    		Int	key 		= getNewRandomId(map);

    		vector<Value> vdata = createSimpleBuffer<Value>(data_size, data);

    		MemBuffer<Value> buf(vdata);

    		auto iter = map.create(key, buf);

    		UInt insertion_pos;
    		for (insertion_pos = 0; insertion_pos < tripples.size(); insertion_pos++)
    		{
    			if (key <= tripples[insertion_pos].id())
    			{
    				break;
    			}
    		}

    		tripples.insert(tripples.begin() + insertion_pos, Tripple(iter.id(), iter.blob_size(), data));
    	}

    	return tripples;
    }

    VMapData createZeroDataVMap(Ctr& map, Int size)
    {
    	VMapData tripples;
    	vector<Value> vdata;

    	for (Int c = 0; c < size; c++)
    	{
    		Int	key 		= getNewRandomId(map);

    		MemBuffer<Value> buf(vdata);

    		auto iter = map.create(key, buf);

    		UInt insertion_pos;
    		for (insertion_pos = 0; insertion_pos < tripples.size(); insertion_pos++)
    		{
    			if (key <= tripples[insertion_pos].id())
    			{
    				break;
    			}
    		}

    		tripples.insert(tripples.begin() + insertion_pos, Tripple(iter.id(), iter.blob_size(), c & 0xFF));
    	}

    	return tripples;
    }


    void checkDataFw(const VMapData& tripples, Ctr& map)
    {
    	if (isReplayMode())
    	{
    		cout<<endl<<"CheckDataFW"<<endl;
    	}

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
    			cout<<idx<<" "<<id<<" "<<size<<endl;
    		}

    		if (id != tripple.id()) {
    			iter.dump();
    		}

    		AssertEQ(MA_SRC, id, tripple.id());

    		if (size != tripple.size()) {
    			iter.dump();
    		}

    		AssertEQ(MA_SRC, size, tripple.size());

    		if (check_data_)
    		{
    			iter.seek(0);

    			BigInt size0;
    			for (size0 = 0; !iter.isEof(); size0++)
    			{
    				auto value = iter.value();

    				AssertEQ(MA_SRC, (BigInt)value, (BigInt)tripple.data());
    				AssertEQ(MA_SRC, iter.pos(), size0);

    				iter.skipFw(1);
    			}

    			AssertEQ(MA_SRC, size, size0);
    			AssertEQ(MA_SRC, iter.pos(), size0);
    		}
    		else if (size > 0)
    		{
    			iter.seek(0);
    			AssertEQ(MA_SRC, (BigInt)iter.value(), (BigInt)tripple.data());

    			iter.skipFw(size - 1);

    			if ((BigInt)iter.value() != (BigInt)tripple.data()) {
    				iter.dump();
    			}

    			AssertEQ(MA_SRC, (BigInt)iter.value(), (BigInt)tripple.data());

    			iter.skipFw(1);
    			AssertTrue(MA_SRC, iter.isEof());
    		}
    		else {
    			iter.seek(0);

    			AssertTrue(MA_SRC, iter.isEof());
    			AssertTrue(MA_SRC, iter.isBof());
    		}

    		iter++;

    		AssertLT(MA_SRC, idx, (Int)tripples.size());
    	}
    }


    void checkDataBw(const VMapData& tripples, Ctr& map)
    {
    	if (isReplayMode())
    	{
    		cout<<endl<<"CheckDataBW"<<endl;
    	}

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
    			cout<<id<<" "<<size<<" "<<idx<<endl;
    		}

    		AssertEQ(MA_SRC, id, tripple.id());
    		AssertEQ(MA_SRC, size, tripple.size());

    		if (check_data_)
    		{
    			iter.seek(size - 1);

    			BigInt size0;
    			for (size0 = 0; !iter.isBof(); size0++)
    			{
    				auto value = iter.value();

    				AssertEQ(MA_SRC, (Int)value, (Int)tripple.data());

    				iter.skipBw(1);
    			}

    			AssertEQ(MA_SRC, size, size0);
    		}
    		else if (size > 0)
    		{
    			AssertEQ(MA_SRC, iter.seek(size - 1), size - 1);
    			AssertEQ(MA_SRC, iter.pos(), size - 1);

    			AssertEQ(MA_SRC, (BigInt)iter.value(), (BigInt)tripple.data());


    			AssertFalse(MA_SRC, iter.isBof());
    			AssertFalse(MA_SRC, iter.isEof());

    			AssertEQ(MA_SRC, iter.skipFw(1), 1);
    			AssertTrue(MA_SRC, iter.isEof());
    			AssertFalse(MA_SRC, iter.isBof());

    			AssertEQ(MA_SRC, iter.skipBw(size), size);
    			AssertEQ(MA_SRC, iter.pos(), 0);
    			AssertEQ(MA_SRC, (BigInt)iter.value(), (BigInt)tripple.data());
    			AssertFalse(MA_SRC, iter.isBof());
    			AssertFalse(MA_SRC, iter.isEof());

    			iter.skipBw(1);

    			AssertTrue(MA_SRC, iter.isBof());
    			AssertFalse(MA_SRC, iter.isEof());
    		}
    		else {
    			iter.seek(0);
    			AssertTrue(MA_SRC, iter.isEof());
    			AssertTrue(MA_SRC, iter.isBof());
    		}

    		iter--;

    		AssertLT(MA_SRC, idx, (Int)tripples.size());
    	}
    }



    void checkBlock(Iterator& iter, BigInt id, BigInt size, Value data)
    {
    	AssertEQ(MA_SRC, iter.id(), id);
    	AssertEQ(MA_SRC, iter.blob_size(), size);

    	iter.seek(0);

    	AssertEQ(MA_SRC, iter.pos(), 0);

    	if (size > 0)
    	{
    		AssertFalse(MA_SRC, iter.isBof());
    	}

    	if (check_data_)
    	{
    		iter.seek(0);

    		BigInt size0;
    		for (size0 = 0; !iter.isEof(); size0++)
    		{
    			auto value = iter.value();

    			AssertEQ(MA_SRC, (BigInt)value, (BigInt)data);
    			AssertEQ(MA_SRC, iter.pos(), size0);

    			iter.skipFw(1);
    		}

    		AssertEQ(MA_SRC, size, size0);
    		AssertEQ(MA_SRC, iter.pos(), size0);
    	}
    	else if (size > 0)
    	{
    		iter.seek(0);
    		AssertEQ(MA_SRC, (BigInt)iter.value(), (BigInt)data);

    		iter.skipFw(size - 1);
    		AssertEQ(MA_SRC, (BigInt)iter.value(), (BigInt)data);

    		iter.skipFw(1);
    		AssertTrue(MA_SRC, iter.isEof());
    	}
    	else {
    		iter.seek(0);

    		AssertTrue(MA_SRC, iter.isEof());
    		AssertTrue(MA_SRC, iter.isBof());
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



    BigInt getNewRandomId(Ctr& map)
    {
    	BigInt id;

    	do {
    		id = getBIRandom(1000000);
    	}
    	while(map.contains(id));

    	return id;
    }


};


static ostream& operator<<(ostream& out, const VectorMapTestBase::Tripple& pair)
{
    out<<pair.id()<<" "<<pair.size()<<" "<<pair.data();
    return out;
}

static istream& operator>>(istream& in, VectorMapTestBase::Tripple& pair)
{
	BigInt id = 0, size = 0, data = 0;

    in>>skipws;
    in>>id;

    in>>skipws;
    in>>size;

    in>>skipws;
    in>>data;

    pair = VectorMapTestBase::Tripple(id, size, data);

    return in;
}


}

#endif

