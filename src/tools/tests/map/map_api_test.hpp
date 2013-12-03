// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_MAP_MAP_API_TEST_HPP_
#define MEMORIA_TESTS_MAP_MAP_API_TEST_HPP_

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
class MapApiTest: public MapTestBase<MapName> {

    typedef MapTestBase<MapName>                                                Base;
    typedef MapApiTest<MapName>                                                 MyType;

    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::Ctr                                                  Ctr;

    Int key_ 		= 0;
    Int skip_bw_  	= 0;
    Int select_bw_  = 0;

public:

    MapApiTest(String name): Base(name)
    {
        MEMORIA_ADD_TEST_PARAM(key_);
        MEMORIA_ADD_TEST_PARAM(skip_bw_);
        MEMORIA_ADD_TEST_PARAM(select_bw_);

    	MEMORIA_ADD_TEST(runTest);
    }

    virtual ~MapApiTest() throw () {}


    void runTest()
    {
    	DefaultLogHandlerImpl logHandler(Base::out());

        Allocator allocator;
        allocator.getLogger()->setHandler(&logHandler);

        Ctr map(&allocator);

        Base::ctr_name_ = map.name();

        BigInt last_key = 0;

        vector<Int> labels;

        for (Int c = 1; c <= 1000; c++)
        {
        	last_key += getRandom(5) + 1;

        	auto iter = map[last_key];
        	iter = std::make_tuple(1, 2);

        	if (getRandom(50) == 0)
        	{
        		iter.set_label(0, 2);

        		labels.push_back(iter.pos());
        	}
        }

        cout<<"Size: "<<map.size()<<endl;

        cout<<"Select Forward: "<<endl;

        auto iter = map.selectLabel(0, 2, 1);
        Int c = 0;

        while (!iter.isEnd())
        {
        	cout<<iter.pos()<<" "<<labels[c]<<endl;
        	iter.selectNextLabel(0, 2);
        	c++;
        }

        cout<<endl<<"Select Backward: "<<endl;

        iter = map.End();

        iter.selectLabelBw(0, 2, 1);

        c = labels.size() - 1;

        while (!iter.isBegin())
        {
        	cout<<iter.pos()<<" "<<labels[c]<<endl;

        	iter.selectLabelBw(0, 2, 1);
        	c--;
        }

        iter = map.RBegin();

        cout<<"Set Label="<<iter.set_hidden_label(1, 2)<<endl;
        cout<<"Set Label="<<iter.set_label(0, 2)<<endl;
        cout<<"Rank.2="<<iter.label_rank(0, 2)<<endl;
        cout<<"Rank.0="<<iter.label_rank(0, 0)<<endl;
        cout<<"Rank.0="<<iter.label_rank(0, 1)<<endl;

        cout<<iter.entry()<<endl;
        cout<<iter.key()<<endl;

        allocator.commit();

        this->StoreAllocator(allocator, this->getResourcePath("api.dump"));
    }


};

}

#endif
