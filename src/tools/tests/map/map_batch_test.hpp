
// Copyright Victor Smirnov 2012-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_MAP_BATCH_TEST_HPP_
#define MEMORIA_TESTS_MAP_BATCH_TEST_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include "../shared/inmem_sequence_test_base.hpp"

#include <vector>
#include <algorithm>
#include <sstream>
#include <memory>


namespace memoria {

template <
    typename T
>
class MapBatchTest: public SequenceCreateTestBase<
    T,
    std::vector<typename SCtrTF<T>::Type::Types::Entry>
>
{
    typedef MapBatchTest<T>                                                     MyType;

    typedef SequenceCreateTestBase<
    			T,
    		    std::vector<typename SCtrTF<T>::Type::Types::Entry>
    >                                                                           Base;

    typedef typename Base::Ctr                                                  Ctr;
    typedef typename Base::Iterator                                             Iterator;
    typedef typename Ctr::Accumulator                                           Accumulator;
    typedef typename Base::ID                                                   ID;

    typedef typename SCtrTF<T>::Type::Types::Entry								Entry;

    typedef std::vector<Entry>                 									MemBuffer;

public:
    MapBatchTest(StringRef name):
        Base(name)
    {
        Base::max_block_size_ = 1024*2;
        Base::size_           = 1024*1024;
    }

    virtual MemBuffer createBuffer(Int size)
    {
        MemBuffer data(size);
        for (auto& item: data)
        {
            Entry entry;

            entry.key() 	= getRandom(10) + 1;
            entry.value() 	= getRandom(100) + 1;

        	item = entry;
        }

        return data;
    }

    virtual MemBuffer createRandomBuffer(Int size)
    {
        MemBuffer data(size);
        for (auto& item: data)
        {
        	Entry entry;

        	entry.key() 	= getRandom(10) + 1;
        	entry.value() 	= getRandom(100) + 1;

        	item = entry;
        }

        return data;
    }

    virtual Iterator seek(Ctr& array, BigInt pos)
    {
        return array.seek(pos);
    }

    virtual void insert(Iterator& iter, MemBuffer& data)
    {
        iter.insert(data);
    }

    virtual void read(Iterator& iter, MemBuffer& data)
    {
        iter.read(data);
    }

    virtual void remove(Iterator& iter, BigInt size) {
        iter.removeNext(size);
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

    std::ostream& out() {
        return Base::out();
    }


    virtual void compareBuffers(const MemBuffer& src, const MemBuffer& tgt, const char* source)
    {
    	AssertEQ(source, src.size(), tgt.size(), SBuf()<<"buffer sizes are not equal");

    	for (size_t c = 0; c < src.size(); c++)
    	{
    		typename MemBuffer::value_type v1 = src[c];
    		typename MemBuffer::value_type v2 = tgt[c];

    		AssertEQ(source, v1.value(), v2.value(), [=](){return SBuf()<<"c="<<c;});
    	}
    }
};



}


#endif

