
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_SEQUENCE_SEQUENCE_TEST_HPP_
#define MEMORIA_TESTS_SEQUENCE_SEQUENCE_TEST_HPP_

#include <memoria/memoria.hpp>
#include <memoria/tools/tests.hpp>

#include "../shared/btree_test_base.hpp"

#include <vector>

namespace memoria {

using namespace memoria::vapi;
using namespace std;

template <Int BitsPerSymbol, bool Dense = true>
class SequenceTest: public BTreeBatchTestBase<
    Sequence<BitsPerSymbol, Dense>,
    SymbolSequence<BitsPerSymbol>
>
{
    typedef SequenceTest<BitsPerSymbol, Dense>                                  MyType;
    typedef MyType                                                              ParamType;
    typedef SymbolSequence<BitsPerSymbol>										MemBuffer;

    typedef BTreeBatchTestBase<
    		Sequence<BitsPerSymbol, Dense>,
    		SymbolSequence<BitsPerSymbol>
    >                                                                           Base;

    typedef typename Base::Ctr                                                  Ctr;
    typedef typename Base::Iterator                                             Iterator;
    typedef typename Base::ID                                                   ID;

    typedef typename Ctr::ElementType											T;

    static const Int Symbols = 1 << BitsPerSymbol;

public:
    SequenceTest(StringRef name):
        Base(name)
    {
        Base::max_block_size_ = 1024*40;
        Base::size_           = 1024*1024*16;
    }

    virtual MemBuffer createBuffer(Ctr& array, Int size, BigInt value)
    {
    	MemBuffer data(size);

    	data.resize(size);

    	data.fillCells([](UBigInt& cell) {
    		cell = getBIRandom();
    	});

        return data;
    }

    virtual Iterator seek(Ctr& array, BigInt pos)
    {
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

    void checkIterator(ostream& out, Iterator& iter, const char* source)
    {
        Base::checkIterator(out, iter, source);

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
                        iter.dump(out);
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
                iter.dump(out);
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
                iter.dump(out);
                throw TestException(MEMORIA_SOURCE, "Iterator is at End but data() is set");
            }
        }
        else {
            if (iter.data().isEmpty())
            {
                iter.dump(out);
                throw TestException(MEMORIA_SOURCE, "Iterator is NOT at End but data() is NOT set");
            }

            if (iter.path().data().parent_idx() != iter.key_idx())
            {
                iter.dump(out);
                throw TestException(MEMORIA_SOURCE, "Iterator data.parent_idx mismatch");
            }
        }
    }
};




}


#endif
