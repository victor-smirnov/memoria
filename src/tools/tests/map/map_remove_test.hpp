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
	template <typename, typename> class MapType
>
class MapRemoveTest: public MapTestBase<MapType> {

	typedef MapRemoveTest<MapType>                                              MyType;
	typedef MapTestBase<MapType>												Base;

	typedef typename Base::Allocator											Allocator;
	typedef typename Base::Iterator												Iterator;
	typedef typename Base::Ctr													Ctr;
	typedef typename Base::PairVector											PairVector;
	typedef typename Base::Pair													Pair;


	BigInt&  		ctr_name_		= Base::ctr_name_;
	Int&  			size_			= Base::size_;
	Int&  			vector_idx_ 	= Base::vector_idx_;
	PairVector& 	pairs 			= Base::pairs;
	PairVector& 	pairs_sorted 	= Base::pairs_sorted;
	String& 		dump_name_ 		= Base::dump_name_;

public:

    MapRemoveTest(StringRef name): Base(name)
    {
        size_ = 10000;

    	MEMORIA_ADD_TEST_WITH_REPLAY(runRemoveTest, replayRemoveTest);
    }

    virtual ~MapRemoveTest() throw () {}

    void runRemoveTest()
    {
        DefaultLogHandlerImpl logHandler(Base::out());

        Allocator allocator;
        allocator.getLogger()->setHandler(&logHandler);

        Ctr map(&allocator);

        ctr_name_ = map.name();

        try {
            for (vector_idx_ = 0; vector_idx_ < size_; vector_idx_++)
            {
                auto iter = map[pairs[vector_idx_].key_];
                iter.value() = pairs[vector_idx_].value_;
            }

            allocator.commit();

            Base::check(allocator, MEMORIA_SOURCE);

            for (auto iter = map.begin(); iter != map.endm(); iter++)
            {
                pairs_sorted.push_back(Pair(iter.key(), iter.value()));
            }

            for (vector_idx_ = 0; vector_idx_ < size_; vector_idx_++)
            {
            	bool result = map.remove(pairs[vector_idx_].key_);

                AssertTrue(MA_SRC, result);

                Base::check(allocator, MEMORIA_SOURCE);

                Base::out()<<vector_idx_<<endl;

                BigInt size = size_ - vector_idx_ - 1;

                AssertEQ(MA_SRC, size, map.size());

                PairVector pairs_sorted_tmp = pairs_sorted;

                for (UInt x = 0; x < pairs_sorted_tmp.size(); x++)
                {
                    if (pairs_sorted_tmp[x].key_ == pairs[vector_idx_].key_)
                    {
                        pairs_sorted_tmp.erase(pairs_sorted_tmp.begin() + x);
                    }
                }

                Base::checkContainerData(map, pairs_sorted_tmp);

                allocator.commit();

                Base::check(allocator, MEMORIA_SOURCE);

                pairs_sorted = pairs_sorted_tmp;
            }
        }
        catch (MemoriaThrowable& e)
        {
        	this->out()<<e<<endl;

        	Base::StorePairs(pairs, pairs_sorted);
            dump_name_ = Base::Store(allocator);
            throw;
        }
    }

    void replayRemoveTest()
    {
        DefaultLogHandlerImpl logHandler(Base::out());
        Allocator allocator;
        allocator.getLogger()->setHandler(&logHandler);

        Base::LoadAllocator(allocator, dump_name_);

        LoadVector(pairs, Base::pairs_data_file_);
        LoadVector(pairs_sorted, Base::pairs_sorted_data_file_);

        Ctr map(&allocator, CTR_FIND, ctr_name_);

        bool result = map.remove(pairs[vector_idx_].key_);

        AssertTrue(MA_SRC, result);

        Base::check(allocator, MEMORIA_SOURCE);

        BigInt size = size_ - vector_idx_ - 1;

        AssertEQ(MA_SRC, size, map.size());

        for (UInt x = 0; x < pairs_sorted.size(); x++)
        {
            if (pairs_sorted[x].key_ == pairs[vector_idx_].key_)
            {
                pairs_sorted.erase(pairs_sorted.begin() + x);
            }
        }

        Base::check(allocator, MEMORIA_SOURCE);

        Base::checkContainerData(map, pairs_sorted);

        allocator.commit();
    }

};

}

#endif
