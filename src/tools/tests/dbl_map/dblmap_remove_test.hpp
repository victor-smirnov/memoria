
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_DBLMAP_REMOVE_TEST_HPP_
#define MEMORIA_TESTS_DBLMAP_REMOVE_TEST_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/profile_tests.hpp>

#include "dblmap_test_base.hpp"

namespace memoria {

using namespace memoria::vapi;
using namespace std;

template <typename CtrName>
class DblMapRemoveTest: public DblMapTestBase<CtrName> {

    typedef DblMapTestBase<CtrName>     	 									Base;

    typedef DblMapRemoveTest<CtrName>                                     		MyType;

protected:

    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::Ctr													Ctr;
    typedef typename Ctr::Iterator												Iterator;

    typedef typename Base::StdDblMap											StdDblMap;

    typedef typename Base::Key													Key;
    typedef typename Base::Value												Value;

    typedef StaticVector<Key, 1>												KeyV;
    typedef std::pair<KeyV, Value>												IOValue;

    bool map2_empty_;

    Int dblmap_check_step_ = 10;

public:

    DblMapRemoveTest(StringRef name): Base(name)
    {
    	MEMORIA_ADD_TEST_PARAM(dblmap_check_step_);

    	MEMORIA_ADD_TEST_PARAM(map2_empty_)->state();

    	MEMORIA_ADD_TEST_WITH_REPLAY(testRemove, replayRemove);
    }


    void testRemove()
    {
    	Allocator allocator;

    	Ctr ctr(&allocator, CTR_CREATE);

    	this->ctr_name_ = ctr.name();

    	StdDblMap std_map;


    	for (Int c = 0; c < this->size_; c++)
    	{
    		BigInt key1  = getRandom(50) + 1;
    		BigInt key2  = getRandom(100) + 1;
    		BigInt value = getRandom(100);

    		std_map[key1][key2] = value;
    	}

    	for (auto entry1: std_map)
    	{
    		auto& map = entry1.second;

    		MemBuffer<IOValue> buffer(map.size());

    		Key prefix = 0;

    		for (auto entry2: map)
    		{
    			KeyV key; key[0] = entry2.first - prefix;

    			IOValue io_value(key, entry2.second);

    			buffer.put(io_value);

    			prefix = entry2.first;
    		}

    		auto iter = ctr.create(entry1.first);

    		buffer.reset();

    		iter.insert(buffer);
    	}

    	this->checkMap(std_map, ctr);

    	allocator.commit();

    	this->StoreAllocator(allocator, this->getResourcePath("remove-full.dump"));

    	try {

    		Int check_cnt = 0;

    		while (std_map.size() > 0)
    		{
    			Key key1, key2;
    			BigInt value;
    			bool map2_empty = map2_empty_ = false;

    			Int idx1 = getRandom(std_map.size());

    			for (auto entry1: std_map)
    			{
    				if (idx1-- == 0)
    				{
    					this->key1_ = key1 = entry1.first;

    					auto& map2 = entry1.second;

    					if (map2.size() > 0)
    					{
    						Int idx2 = getRandom(map2.size());

    						for (auto entry2: map2)
    						{
    							if (idx2-- == 0)
    							{
    								this->key2_  = key2  = entry2.first;
    								this->value_ = value = entry2.second;
    							}
    						}
    					}
    					else {
    						map2_empty = map2_empty_ = true;
    					}
    				}
    			}

    			if (map2_empty)
    			{
    				this->out()<<"Remove from empty map: "<<key1<<std::endl;

    				std_map.erase(key1);

    				ctr.remove(key1);
    			}
    			else {
    				this->out()<<"Remove from map: "<<key1<<" "<<key2<<std::endl;

    				std_map[key1].erase(key2);

    				auto iter = ctr.find(key1);
    				MEMORIA_ASSERT_TRUE(iter.found());

    				iter.remove2nd(key2);
    			}

    			if (check_cnt % dblmap_check_step_ == 0) {
    				this->checkMap(std_map, ctr);
    			}

    			check_cnt++;

    			allocator.commit();
    		}

    		this->checkMap(std_map, ctr);
    	}
    	catch (...)
    	{
    		this->dump_name_ = this->Store(allocator);
    		throw;
    	}

    	allocator.commit();
    	this->StoreAllocator(allocator, this->getResourcePath("remove.dump"));
    }

    void replayRemove()
    {
    	Allocator allocator;

    	this->LoadAllocator(allocator, this->dump_name_);

    	Ctr ctr(&allocator, CTR_FIND, this->ctr_name_);

    	auto& key1 = this->key1_;
    	auto& key2 = this->key2_;

    	if (map2_empty_)
    	{
    		this->out()<<"Remove from empty map: "<<key1<<std::endl;

    		ctr.remove(key1);
    	}
    	else {
    		this->out()<<"Remove from map: "<<key1<<" "<<key2<<std::endl;

    		auto iter = ctr.find(key1);
    		MEMORIA_ASSERT_TRUE(iter.found());

    		iter.remove2nd(key2);
    	}


    	auto iter2 = ctr.find(50);

    	this->dumpEntries(iter2);

    	iter2.dump();
    }

    virtual ~DblMapRemoveTest() throw() {}

};




}



#endif

