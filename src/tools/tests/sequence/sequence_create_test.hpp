
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_SEQUENCE_SEQUENCE_CREATE_TEST_HPP_
#define MEMORIA_TESTS_SEQUENCE_SEQUENCE_CREATE_TEST_HPP_

#include <memoria/memoria.hpp>
#include <memoria/tools/tests.hpp>

#include "../shared/randomaccesslist_test_base.hpp"

#include <vector>

namespace memoria {

using namespace memoria::vapi;
using namespace std;

template <Int BitsPerSymbol, bool Dense = true>
class SequenceCreateTest: public RandomAccessListTestBase <
									Sequence<BitsPerSymbol, Dense>,
									SymbolSequence<BitsPerSymbol>
						  >
{
    typedef SequenceCreateTest<BitsPerSymbol, Dense>                            MyType;
    typedef MyType                                                              ParamType;
    typedef SymbolSequence<BitsPerSymbol>										MemBuffer;

    typedef RandomAccessListTestBase <
				Sequence<BitsPerSymbol, Dense>,
				SymbolSequence<BitsPerSymbol>
    		>                                                          			Base;

    typedef typename Base::Ctr               									Ctr;
    typedef typename Base::Iterator                                             Iterator;
    typedef typename Base::Accumulator                                          Accumulator;
    typedef typename Base::ID                                                   ID;
    typedef typename Ctr::Value                                                 Symbol;

    typedef typename Ctr::ElementType											T;

public:
    SequenceCreateTest(StringRef name):
        Base(name)
    {
    	Base::size_ 			= 32 * 1024 * 1024;
    	Base::max_block_size_ 	= 1024 * 128;
    	Base::check_size_ 		= 1000;
    }

    virtual Iterator seek(Ctr& array, BigInt pos) {
    	return array.seek(pos);
    }

    virtual void insert(Iterator& iter, MemBuffer& data)
    {
    	auto src = data.source();
    	iter.insert(src);
    }

    virtual void read(Iterator& iter, MemBuffer& data)
    {
    	auto tgt = data.target();
    	iter.read(tgt);
    }

    virtual void skip(Iterator& iter, BigInt offset)
    {
    	iter.skip(offset);
    }

    virtual BigInt getSize(Ctr& ctr) {
    	return ctr.size();
    }

    virtual BigInt getPosition(Iterator& iter) {
    	return iter.pos();
    }

    virtual void compareBuffers(const MemBuffer& src, const MemBuffer& tgt, const char* source)
    {
    	AssertEQ(source, src.size(), tgt.size(), SBuf()<<"buffer sizes are not equal");

    	for (size_t c = 0; c < src.size(); c++)
    	{
    		auto v1 = src[c];
    		auto v2 = tgt[c];

    		AssertEQ(source, v1, v2, [=](){return SBuf()<<"c="<<c;});
    	}
    }

    virtual MemBuffer createBuffer(Int size)
    {
    	MemBuffer data(size);

    	data.resize(size);

    	data.fillCells([](UBigInt& cell) {
    		cell = 0;
    	});

        return data;
    }

    virtual MemBuffer createRandomBuffer(Int size)
    {
    	MemBuffer data(size);

    	data.resize(size);

    	data.fillCells([](UBigInt& cell) {
    		cell = getBIRandom();
    	});

    	return data;
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

    virtual void remove(Iterator& iter, BigInt size)
    {
    	iter.remove(size);
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
