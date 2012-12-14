
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_VECTOR_VECTOR_TEST_HPP_
#define MEMORIA_TESTS_VECTOR_VECTOR_TEST_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>

#include "../shared/btree_test_base.hpp"

namespace memoria {

using namespace memoria::vapi;


//class VectorReplay: public ReplayParams {
//public:
//
//    Int     data_;
//    bool    insert_;
//    Int     block_size_;
//
//    Int     page_step_;
//
//    BigInt  pos_;
//
//    Int     cnt_;
//
//    BigInt  ctr_name_;
//
//public:
//    VectorReplay(): ReplayParams(), data_(0), insert_(true), block_size_(0), page_step_(-1), pos_(-1), cnt_(0)
//    {
//        Add("data",         data_);
//        Add("insert",       insert_);
//        Add("data_size",    block_size_);
//        Add("page_step",    page_step_);
//        Add("pos",          pos_);
//        Add("cnt",          cnt_);
//        Add("ctr_name",     ctr_name_);
//    }
//};


template <typename T>
class VectorTest: public BTreeBatchTestBase<
    VectorCtr<T>,
    ArrayData<T>
>
{
    typedef VectorTest                                              MyType;
    typedef MyType                                                  ParamType;

    typedef BTreeBatchTestBase<
                VectorCtr<T>,
                ArrayData<T>
    >                                                               Base;

    typedef typename Base::Ctr                                      Ctr;
    typedef typename Base::Iterator                                 Iterator;
    typedef typename Base::ID                                       ID;


public:
    VectorTest(StringRef name):
        Base(name)
    {
        Ctr::initMetadata();

        Base::max_block_size_ = 1024*40;
        Base::size_           = 1024*1024*16;
    }

    virtual ArrayData<T> createBuffer(Ctr& array, Int size, BigInt value)
    {
        ArrayData<T> data((SizeT)size);

        for (Int c = 0; c < size; c++)
        {
            *(data.data() + c) = value;
        }

        return data;
    }

    virtual Iterator seek(Ctr& array, BigInt pos)
    {
        return array.seek(pos);
    }

    virtual void insert(Iterator& iter, const ArrayData<T>& data)
    {
        iter.insert(data);
    }

    virtual void read(Iterator& iter, ArrayData<T>& data)
    {
        iter.read(data);
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
