
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_SUM_SET_BATCH_SUM_SET_BATCH_TESTS_HPP_
#define MEMORIA_TESTS_SUM_SET_BATCH_SUM_SET_BATCH_TESTS_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include "../shared/btree_test_base.hpp"

#include <vector>
#include <algorithm>
#include <sstream>
#include <memory>


namespace memoria {

typedef SCtrTF<Set1>::Type SumSet1Ctr;

class SumsetBatchTest: public BTreeBatchTestBase<
    Set1,
    typename SumSet1Ctr::LeafPairsVector
>
{
    typedef BTreeBatchTestBase<
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



    virtual MemBuffer createBuffer(Ctr& ctr, Int size, BigInt value)
    {
        MemBuffer array(size);

        for (auto& pair: array)
        {
            pair.keys[0] = value;
        }

        return array;
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

        MEMORIA_TEST_THROW_IF(pos, !=, i.keyNum());

        return i;
    }

    virtual void insert(Iterator& iter, MemBuffer& data)
    {
        BigInt size = iter.model().getSize();

        iter.model().insertBatch(iter, data);

        MEMORIA_TEST_THROW_IF(size, ==, iter.model().getSize());

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

        MEMORIA_TEST_THROW_IF(cnt, !=, array.getSize());
    }

    virtual void checkIteratorPrefix(ostream& out, Iterator& iter, const char* source) {}

};

}


#endif

