
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_DBLMAP_TEST_BASE_HPP_
#define MEMORIA_TESTS_DBLMAP_TEST_BASE_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/profile_tests.hpp>


namespace memoria {

using namespace memoria::vapi;
using namespace std;

template <typename CtrName>
class DblMapTestBase: public SPTestTask {

    typedef SPTestTask      													Base;

    typedef DblMapTestBase<CtrName>                                     		MyType;

protected:

    typedef typename Base::Allocator                                            Allocator;
    typedef typename SCtrTF<CtrName>::Type										Ctr;
    typedef typename Ctr::Iterator												Iterator;

    typedef typename Ctr::Types::Key 											Key;
    typedef typename Ctr::Types::Value											Value;

    typedef std::map<Key, std::map<Key, Value>>									StdDblMap;

    BigInt 	key_ = 9;

    BigInt 	ctr_name_;
    Key 	key1_;
    Key 	key2_;
    BigInt 	value_;

    String 	dump_name_;

public:

    DblMapTestBase(StringRef name): Base(name)
    {
    	this->size_ = 1024;

    	Ctr::initMetadata();

    	MEMORIA_ADD_TEST_PARAM(key_);

    	MEMORIA_ADD_TEST_PARAM(ctr_name_)->state();
    	MEMORIA_ADD_TEST_PARAM(key1_)->state();
    	MEMORIA_ADD_TEST_PARAM(key2_)->state();
    	MEMORIA_ADD_TEST_PARAM(value_)->state();
    	MEMORIA_ADD_TEST_PARAM(dump_name_)->state();
    }

    void dumpMap(StdDblMap& std_map)
    {
    	for (auto entry: std_map)
    	{
    		this->out()<<"Entry: "<<entry.first<<" "<<entry.second.size()<<std::endl;

    		auto& map2	= entry.second;

    		for (auto entry2: map2)
    		{
    			this->out()<<entry2.first<<" "<<entry2.second<<std::endl;
    		}
    	}
    }

    void dumpMap(Ctr& ctr)
    {
    	auto iter = ctr.Begin();

    	while (!iter.isEnd())
    	{
    		this->out()<<"Maps: "<<iter.id()<<" "<<iter.blob_size()<<std::endl;

    		iter.findData();

    		while (!iter.isEof())
    		{
    			this->out()<<"Entry: "<<iter.key2()<<" "<<iter.value()<<std::endl;

    			iter.skipFw(1);
    		}

    		this->out()<<std::endl;

    		iter++;
    	}
    }

    void dumpEntries(Iterator& iter)
    {
    	iter.findData();

    	int c = 0;

    	while (!iter.isEof())
    	{
    		this->out()<<"Entry: "<<iter.key2()<<" "<<iter.value()<<" "<<iter.cache().second_prefix()<<" "<<iter.pos()<<std::endl;

    		if (c >= 5) {
    			int a = 0; a++;
    		}

    		iter.skipFw(1);

    		c++;
    	}

    	this->out()<<std::endl;
    }


    void dumpMapSizes(StdDblMap& std_map, Ctr& ctr)
    {
    	auto iter = ctr.Begin();

    	for (auto entry: std_map)
    	{
    		auto key 	= entry.first;
    		auto& map2	= entry.second;

    		this->out()<<"Maps: "<<key<<" "<<map2.size()<<" "<<iter.id()<<" "<<iter.blob_size()<<std::endl;

    		MEMORIA_ASSERT(key, ==, iter.id());
    		MEMORIA_ASSERT(map2.size(), ==, iter.blob_size());

    		iter.nextEntry();
    	}
    }

    void dumpMapSizes(Ctr& ctr)
    {
    	auto iter = ctr.Begin();

    	while (!iter.isEnd())
    	{
    		this->out()<<"Map: "<<iter.id()<<" "<<iter.blob_size()<<std::endl;
    		iter.nextEntry();
    	}
    }


    void checkMap(StdDblMap& std_map, Ctr& ctr)
    {
    	auto iter = ctr.Begin();

    	for (auto entry: std_map)
    	{
    		auto key 	= entry.first;
    		auto& map2	= entry.second;

    		this->out()<<"Maps: "<<key<<" "<<map2.size()<<" "<<iter.id()<<" "<<iter.blob_size()<<std::endl;

    		auto iter1 = iter;

    		iter1.findData();

    		AssertEQ(MA_SRC, iter1.id(), key);
    		AssertEQ(MA_SRC, iter1.blob_size(), map2.size());

    		auto m_iter = map2.begin();

    		while (!iter1.isEof())
    		{
    			auto entry2 = *m_iter;

    			this->out()<<"Entry: "<<entry2.first<<" "<<entry2.second<<" "<<iter1.key2()<<" "<<iter1.value()<<std::endl;

    			AssertEQ(MA_SRC, iter1.key2(), entry2.first);
    			AssertEQ(MA_SRC, iter1.value(), entry2.second);

    			iter1.skipFw(1);
    			m_iter++;
    		}

    		this->out()<<std::endl;

    		iter++;
    	}
    }

    virtual ~DblMapTestBase() throw() {}

};




}



#endif

