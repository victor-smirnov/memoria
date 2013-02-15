
// Copyright Victor Smirnov 2012-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_SUMSETBATCH_SUMSETBATCHTEST_HPP_
#define MEMORIA_TESTS_SUMSETBATCH_SUMSETBATCHTEST_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include "../shared/randomaccesslist_test_base.hpp"

#include <vector>
#include <algorithm>
#include <sstream>
#include <memory>


namespace memoria {

typedef SCtrTF<Set1>::Type SumSet1Ctr;

class SumsetBatchTest: public RandomAccessListTestBase<
    Set1,
    typename SumSet1Ctr::LeafPairsVector
>
{
    typedef RandomAccessListTestBase<
            Set1,
            typename SumSet1Ctr::LeafPairsVector
    >                                                                           Base;

    typedef typename Base::Ctr                                                  Ctr;
    typedef typename Base::Accumulator                                          Accumulator;
    typedef typename SumSet1Ctr::LeafPairsVector                                MemBuffer;

    static const Int Indexes                                                    = Ctr::Indexes;




public:
    SumsetBatchTest():
        Base("SumsetBatch")
    {
        size_ = 1024*1024;
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
            pair.keys[0] = cnt++;

            if (cnt == 10000) cnt = 0;
        }

        return array;
    }


    virtual void compareBuffers(const MemBuffer& src, const MemBuffer& tgt, const char* source)
    {
    	AssertEQ(source, src.size(), tgt.size(), SBuf()<<"buffer sizes are not equal");

    	for (size_t c = 0; c < src.size(); c++)
    	{
    		auto v1 = src[c].keys[0];
    		auto v2 = tgt[c].keys[0];

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

        AssertEQ(MA_SRC, pos, i.keyNum());

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
            for (Int c = 0; c < Indexes; c++)
            {
                value.keys[c] = iter.getRawKey(c);
            }

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
        iter.model().removeEntries(iter, iter2, keys);
    }

    virtual void skip(Iterator& iter, BigInt offset)
    {
        if (offset > 0)
        {
            for (BigInt c = 0; c < offset; c++)
            {
                iter.next();
            }
        }
        else {
            for (BigInt c = 0; c < -offset; c++)
            {
                iter.prev();
            }
        }
    }

    virtual BigInt getPosition(Iterator& iter)
    {
        return iter.keyNum();
    }

    virtual BigInt getLocalPosition(Iterator& iter)
    {
        return iter.key_idx();
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

