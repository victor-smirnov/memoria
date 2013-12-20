
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_vctr_vctr_TEST_HPP_
#define MEMORIA_TESTS_vctr_vctr_TEST_HPP_

#include <memoria/memoria.hpp>
#include <memoria/tools/tests.hpp>

#include "../shared/inmem_sequence_test_base.hpp"

#include <vector>

namespace memoria {

using namespace memoria::vapi;
using namespace std;

template <typename T>
class VectorTest: public SequenceCreateTestBase<
    Vector<T>,
    vector<T>
>
{
    typedef VectorTest<T>                                                       MyType;
    typedef MyType                                                              ParamType;

    typedef SequenceCreateTestBase<
                Vector<T>,
                vector<T>
    >                                                                           Base;

    typedef typename Base::Ctr                                                  Ctr;
    typedef typename Base::Iterator                                             Iterator;
    typedef typename Ctr::Accumulator                                           Accumulator;
    typedef typename Base::ID                                                   ID;

    typedef vector<T>                                                           MemBuffer;

public:
    VectorTest(StringRef name):
        Base(name)
    {
        Base::max_block_size_ = 1024*40;
        Base::size_           = 1024*1024*2;
    }

    virtual MemBuffer createBuffer(Int size)
    {
        MemBuffer data(size);
        for (auto& item: data)
        {
            item = 0;
        }

        return data;
    }

    virtual MemBuffer createRandomBuffer(Int size)
    {
        MemBuffer data(size);
        for (auto& item: data)
        {
            item = getRandom();
        }

        return data;
    }

    virtual Iterator seek(Ctr& array, BigInt pos)
    {
        return array.seek(pos);
    }

    virtual void insert(Iterator& iter, vector<T>& data)
    {
        iter.insert(data);
    }

    virtual void read(Iterator& iter, vector<T>& data)
    {
        data = iter.subVector(data.size());
        iter.skip(data.size());
    }

    virtual void remove(Iterator& iter, BigInt size) {
        iter.remove(size);
    }

    virtual void skip(Iterator& iter, BigInt offset)
    {
        iter.skip(offset);
    }

    virtual BigInt getPosition(Iterator& iter)
    {
        return iter.pos();
    }

    virtual BigInt getLocalPosition(Iterator& iter)
    {
        return iter.key_idx();
    }

    virtual BigInt getSize(Ctr& array)
    {
        return array.size();
    }

    ostream& out() {
        return Base::out();
    }

    virtual void compareBuffers(const MemBuffer& src, const MemBuffer& tgt, const char* source)
    {
    	AssertEQ(source, src.size(), tgt.size(), SBuf()<<"buffer sizes are not equal");

    	for (size_t c = 0; c < src.size(); c++)
    	{
    		typename MemBuffer::value_type v1 = src[c];
    		typename MemBuffer::value_type v2 = tgt[c];

    		AssertEQ(source, v1, v2, [=](){return SBuf()<<"c="<<c;});
    	}
    }
};



}


#endif
