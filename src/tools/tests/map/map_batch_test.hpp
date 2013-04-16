
// Copyright Victor Smirnov 2012-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_MAP_MAP_BATCH_TEST_HPP_
#define MEMORIA_TESTS_MAP_MAP_BATCH_TEST_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/containers/map2/map_factory.hpp>

#include "../shared/randomaccesslist_test_base.hpp"

#include <vector>
#include <algorithm>
#include <sstream>
#include <memory>


namespace memoria {

typedef SCtrTF<Map2<BigInt, Int>>::Type SumSet1Ctr;

class MapBatchTest: public RandomAccessListTestBase<
    Map2<BigInt, Int>,
    typename SumSet1Ctr::LeafPairsVector
>
{
    typedef RandomAccessListTestBase<
    		Map2<BigInt, Int>,
            typename SumSet1Ctr::LeafPairsVector
    >                                                                           Base;

    typedef typename Base::Ctr                                                  Ctr;
    typedef typename Base::Accumulator                                          Accumulator;
    typedef typename SumSet1Ctr::LeafPairsVector                                MemBuffer;


public:
    MapBatchTest():
        Base("MapBatch")
    {
        size_ = 1024*1024;
        Ctr::initMetadata();
    }


    virtual MemBuffer createBuffer(Int size) {
    	return createRandomBuffer(size);
    }

    virtual MemBuffer createRandomBuffer(Int size)
    {
        MemBuffer array(size);

        BigInt cnt = 0;
        for (auto& pair: array)
        {
            pair.first = cnt++;

            if (cnt == 10000) cnt = 0;
        }

        return array;
    }


    virtual void compareBuffers(const MemBuffer& src, const MemBuffer& tgt, const char* source)
    {
    	AssertEQ(source, src.size(), tgt.size(), SBuf()<<"buffer sizes are not equal");

    	for (size_t c = 0; c < src.size(); c++)
    	{
    		auto v1 = src[c].first;
    		auto v2 = tgt[c].first;

    		AssertEQ(source, v1, v2, [=](){return SBuf()<<"c="<<c;});
    	}
    }


    virtual Iterator seek(Ctr& array, BigInt pos)
    {
        Iterator i = array.Begin();

        for (BigInt c = 0; c < pos; c++)
        {
            if (!i.next())
            {
                break;
            }
        }

//        AssertEQ(MA_SRC, pos, i.iter().keyNum());

        return i;
    }

    virtual void insert(Iterator& iter, MemBuffer& data)
    {
        BigInt size = iter.model().getSize();

        iter.model().insertBatch(iter, data);

        AssertNEQ(MA_SRC, size, iter.model().getSize());

        checkSize(iter.model());
    }

    virtual void read(Iterator& iter, MemBuffer& data)
    {
        for (auto& value: data)
        {
        	value.first = iter.rawKey();

            if (!iter.next())
            {
                break;
            }
        }
    }

    virtual void remove(Iterator& iter, BigInt size)
    {
        auto iter2 = iter;
        skip(iter2, size);
        Accumulator keys;
        iter.model().removeMapEntries(iter, iter2, keys);
    }

    virtual void skip(Iterator& iter, BigInt offset)
    {
        if (offset > 0)
        {
        	Int actual = 0;

        	for (BigInt c = 0; c < offset; c++)
            {
                actual += iter.next();
            }

//        	cout<<"Actual: "<<actual<<endl;
        }
        else {
        	Int actual = 0;

            for (BigInt c = 0; c < -offset; c++)
            {
                actual += iter.prev();
            }

//            cout<<"Actual: "<<actual<<endl;
        }
    }

    virtual BigInt getPosition(Iterator& iter)
    {
        return iter.keyNum();
    }

    virtual BigInt getLocalPosition(Iterator& iter)
    {
        return iter.entry_idx();
    }

    virtual BigInt getSize(Ctr& array)
    {
        return array.getSize();
    }

    void checkSize(Ctr& array)
    {
        BigInt cnt = 0;

        for (auto iter = array.Begin(); iter.isNotEnd(); iter.next())
        {
            cnt++;
        }

        AssertEQ(MA_SRC, cnt, array.getSize());
    }

    virtual void checkIteratorPrefix(Iterator& iter, const char* source) {}

};

}


#endif

