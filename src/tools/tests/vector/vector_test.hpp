
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_VECTOR_VECTOR_TEST_HPP_
#define MEMORIA_TESTS_VECTOR_VECTOR_TEST_HPP_

#include <memoria/memoria.hpp>
#include <memoria/tools/tests.hpp>

#include "../shared/randomaccesslist_test_base.hpp"

#include <vector>

namespace memoria {

using namespace memoria::vapi;
using namespace std;

template <typename T>
class VectorTest: public RandomAccessListTestBase<
    Vector<T>,
    vector<T>
>
{
    typedef VectorTest<T>                                                       MyType;
    typedef MyType                                                              ParamType;

    typedef RandomAccessListTestBase<
                Vector<T>,
                vector<T>
    >                                                                           Base;

    typedef typename Base::Ctr                                                  Ctr;
    typedef typename Base::Iterator                                             Iterator;
    typedef typename Base::ID                                                   ID;

    typedef vector<T>															MemBuffer;

public:
    VectorTest(StringRef name):
        Base(name)
    {
        Base::max_block_size_ = 1024*40;
        Base::size_           = 1024*1024*16;
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

    virtual void compareBuffers(const MemBuffer& src, const MemBuffer& tgt, const char* source)
    {
    	AssertEQ(source, src.size(), tgt.size(), SBuf()<<"buffer sizes are not equal");

    	for (size_t c = 0; c < src.size(); c++)
    	{
    		auto v1 = src[c];
    		auto v2 = tgt[c];

    		AssertEQ(source, v1, v2);
    	}
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
        return iter.dataPos();
    }

    virtual BigInt getSize(Ctr& array)
    {
        return array.size();
    }

    ostream& out() {
    	return Base::out();
    }

    virtual void fillRandom(Ctr& ctr, BigInt size)
    {
    	MemBuffer data = createRandomBuffer(size);
    	Iterator iter = ctr.seek(0);
    	insert(iter, data);
    }

    void checkIterator(Iterator& iter, const char* source)
    {
        Base::checkIterator(iter, source);

        auto& path = iter.path();

        if (path.data().node().isSet())
        {
            if (iter.dataPos() < 0)
            {
                throw TestException(source, SBuf()<<"iter.dataPos() is negative: "<<iter.dataPos());
            }

            bool found = false;
            for (Int idx = 0; idx < path[0]->children_count(); idx++)
            {
                ID id = iter.model().getLeafData(path[0].node(), idx);
                if (id == path.data()->id())
                {
                    if (path.data().parent_idx() != idx)
                    {
                        iter.dump(out());
                        throw TestException(source, SBuf()<<"Invalid parent-child relationship for node:"
                                                          <<path[0]->id()
                                                          <<" DATA: "
                                                          <<path.data()->id()
                                                          <<" idx="
                                                          <<idx
                                                          <<" parent_idx="<<path.data().parent_idx());
                    }
                    else {
                        found = true;
                        break;
                    }
                }
            }

            if (!found)
            {
                iter.dump(out());
                throw TestException(source, SBuf()<<"Data: "
                                                  <<path.data()->id()
                                                  <<" is not fount is it's parent, parent_idx="
                                                  <<path.data().parent_idx());
            }
        }


        if (iter.isEnd())
        {
            if (iter.data().isSet())
            {
                iter.dump(out());
                throw TestException(MEMORIA_SOURCE, "Iterator is at End but data() is set");
            }
        }
        else {
            if (iter.data().isEmpty())
            {
                iter.dump(out());
                throw TestException(MEMORIA_SOURCE, "Iterator is NOT at End but data() is NOT set");
            }

            if (iter.path().data().parent_idx() != iter.key_idx())
            {
                iter.dump(out());
                throw TestException(MEMORIA_SOURCE, "Iterator data.parent_idx mismatch");
            }
        }
    }
};



}


#endif
