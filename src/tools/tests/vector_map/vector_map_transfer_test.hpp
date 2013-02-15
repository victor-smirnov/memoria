
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_VECTOR_MAP_VECTOR_MAP_TRANSFER_TEST_HPP_
#define MEMORIA_TESTS_VECTOR_MAP_VECTOR_MAP_TRANSFER_TEST_HPP_

#include <memoria/memoria.hpp>
#include <memoria/tools/tests.hpp>

#include <vector>

namespace memoria {

using namespace memoria::vapi;
using namespace std;

template <typename T>
class VectorMapTransferTest: public SPTestTask {
    typedef VectorMapTransferTest                                               MyType;
    typedef SPTestTask                                                          Base;


    typedef typename SCtrTF<VectorMap<BigInt, T>>::Type                         Ctr;
    typedef typename Ctr::Iterator                                              Iterator;


    Int transfers_      = 10;
    Int max_block_size_ = 1024*128;
    Int min_block_size_ = 128;

    Int block_size_;
    Int ctr1_name_;
    Int ctr2_name_;
    String dump_name_;
    BigInt pos1_;
    BigInt pos2_;


public:
    VectorMapTransferTest(StringRef name):
        Base(name)
    {
        Base::size_ = 1024;

        MEMORIA_ADD_TEST_PARAM(transfers_);
        MEMORIA_ADD_TEST_PARAM(max_block_size_);
        MEMORIA_ADD_TEST_PARAM(min_block_size_);

        MEMORIA_ADD_TEST_PARAM(block_size_)->state();
        MEMORIA_ADD_TEST_PARAM(ctr1_name_)->state();
        MEMORIA_ADD_TEST_PARAM(ctr2_name_)->state();
        MEMORIA_ADD_TEST_PARAM(dump_name_)->state();

        MEMORIA_ADD_TEST_PARAM(pos1_)->state();
        MEMORIA_ADD_TEST_PARAM(pos2_)->state();

        MEMORIA_ADD_TEST_WITH_REPLAY(runInsertTest, runInsertReplay);
    }

    void compareVectors(Ctr& v1, Ctr& v2, const char* src)
    {
        if (v2.size() != v2.size())
        {
            throw TestException(src, SBuf()<<"VectorMaps have different key couts: "<<v1.size()<<" "<<v2.size());
        }

        if (v2.totalDataSize() != v2.totalDataSize())
        {
            throw TestException(
            		src,
            		SBuf()<<"VectorMaps have different sizes: "
            			  <<v1.totalDataSize()
            			  <<" "
            			  <<v2.totalDataSize()
            );
        }

        auto iter2 = v2.Begin();

        for (auto& iter1: v1)
        {
            if (iter1.key() != iter2.key())
            {
                throw TestException(src, SBuf()<<"Keys are not equal: "<<iter1.key()<<" "<<iter2.key());
            }

            vector<T> data1 = iter1.read();
            vector<T> data2 = iter2.read();

            if (data1.size() != data2.size())
            {
                throw TestException(src, SBuf()<<"Data sizes are not equal: "<<data1.size()<<" "<<data2.size());
            }

            for (UInt c = 0; c < data1.size(); c++)
            {
                if (data1[c] != data2[c])
                {
                    MemBuffer<T> b1(data1);
                    MemBuffer<T> b2(data2);

                    b1.dump(out());
                    b2.dump(out());

                    throw TestException(src, SBuf()<<"Data contents are not equal at: "<<c<<" "<<iter1.key());
                }
            }

            iter2++;
        }
    }

    void insertData(Ctr& v1, Ctr& v2)
    {
        v2[pos2_] = v1[pos1_].asData();
        v1[pos2_] = v2[pos1_].asData();
    }

    vector<T> createRandomBuffer()
    {
        Int size = getRandom(max_block_size_ - min_block_size_) + min_block_size_;
        return createBuffer<T>(size, getRandom(255));
    }

    void runInsertTest()
    {
        Allocator allocator;

        try {

            Ctr v1(&allocator);
            Ctr v2(&allocator);

            ctr1_name_ = v1.name();
            ctr2_name_ = v2.name();

            for (BigInt c = 0; c < size_; c++)
            {
                auto buf = createRandomBuffer();

                v1[c] = buf;
                v2[c] = buf;
                out()<<"Insert: "<<c<<endl;
            }

            allocator.commit();

            AssertEQ(MA_SRC, size_, v1.size());

            for (Int c = 0; c < transfers_; c++)
            {
                pos1_ = getRandom(v1.size());
                pos2_ = getRandom(v2.size());

                insertData(v1, v2);

                check(allocator, "Allocator check failed",  MEMORIA_SOURCE);
                compareVectors(v1, v2, MEMORIA_SOURCE);

                auto buf1 = createRandomBuffer();
                auto buf2 = createRandomBuffer();

                v1[pos1_] = v2[pos1_] = buf1;
                v1[pos2_] = v2[pos2_] = buf2;

                check(allocator, "Allocator check failed",  MEMORIA_SOURCE);
                compareVectors(v1, v2, MEMORIA_SOURCE);

                allocator.commit();
                out()<<c<<endl;
            }
        }
        catch (...) {
            dump_name_ = Store(allocator);
            throw;
        }
    }

    void runInsertReplay()
    {
        Allocator allocator;
        LoadAllocator(allocator, dump_name_);

        check(allocator, "Allocator check failed",  MEMORIA_SOURCE);

        Ctr v1(&allocator, CTR_FIND, ctr1_name_);
        Ctr v2(&allocator, CTR_FIND, ctr2_name_);

        insertData(v1, v2);

        check(allocator, "Allocator check failed",  MEMORIA_SOURCE);

        compareVectors(v1, v2, MEMORIA_SOURCE);
    }
};



}


#endif
