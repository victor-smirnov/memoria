
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_VECTOR_VECTOR_TRANSFER_TEST_HPP_
#define MEMORIA_TESTS_VECTOR_VECTOR_TRANSFER_TEST_HPP_

#include <memoria/memoria.hpp>
#include <memoria/tools/tests.hpp>

#include <vector>

namespace memoria {

using namespace memoria::vapi;
using namespace std;

template <typename T>
class VectorTransferTest: public SPTestTask {
    typedef VectorTransferTest                                                  MyType;
    typedef SPTestTask                                                          Base;


    typedef typename SCtrTF<Vector<T>>::Type                                    Ctr;
    typedef typename Ctr::Iterator                                              Iterator;


    Int transfers_      = 1000;
    Int max_block_size_ = 1024*128;
    Int min_block_size_ = 128;

    Int block_size_;
    Int ctr1_name_;
    Int ctr2_name_;
    String dump_name_;
    BigInt pos1_;
    BigInt pos2_;


public:
    VectorTransferTest(StringRef name):
        Base(name)
    {
        Base::size_ = 16*1024*1024;

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

    BigInt compareVectors(T* v1, BigInt size1, T* v2, BigInt size2)
    {
        if (size1 != size2)
        {
            return -2;
        }

        for (Int c = 0; c < size1; c++)
        {
            if (v1[c] != v2[c])
            {
                return c;
            }
        }

        return -1;
    }

    void compareVectors(Ctr& v1, Ctr& v2, const char* src)
    {
        if (v2.size() != v2.size())
        {
            throw TestException(src, SBuf()<<"Vectors have different sizes: "<<v1.size()<<" "<<v2.size());
        }

        auto iter1 = v1.Begin();
        auto iter2 = v2.Begin();

        T buffer1[1024];
        T buffer2[1024];

        while (!iter1.isEof())
        {
            MemBuffer<T> buf1(buffer1, 1024);
            MemBuffer<T> buf2(buffer2, 1024);

            BigInt size1 = iter1.read(buf1);
            BigInt size2 = iter2.read(buf2);

            BigInt result = compareVectors(buffer1, size1, buffer2, size2);
            if (result != -1)
            {
                iter1.dump(out());
                iter2.dump(out());

                if (result == -2)
                {
                    throw TestException(src,
                                        SBuf()
                                        <<"Data buffers have different sizes: "
                                        <<size1<<" "<<size2);
                }
                else {

                    buf1.dump(out());
                    buf2.dump(out());

                    throw TestException(src,
                            SBuf()
                            <<"Buffer content mismatch at: "
                            <<result);
                }
            }
        }
    }

    void insertData(Ctr& v1, Ctr& v2)
    {
        auto i1 = v1.seek(pos1_);
        auto i2 = v2.seek(pos2_);

        auto data1 = i1.asData(block_size_);
        i2.insert(data1);

        i2.skip(-block_size_);

        auto i1_1 = v1.seek(pos2_);
        auto data2 = i2.asData(block_size_);
        i1_1.insert(data2);
    }

    void runInsertTest()
    {
        Allocator allocator;

        try {

            Ctr v1(&allocator);
            Ctr v2(&allocator);

            ctr1_name_ = v1.name();
            ctr2_name_ = v2.name();

            for (BigInt c = 0; c < Base::size_/1024; c++)
            {
                vector<T> buf = createBuffer<T>(1024, 100);
                v1<<buf;
                v2<<buf;
            }

            check(allocator, "Allocator check failed",  MEMORIA_SOURCE);

            allocator.commit();

            compareVectors(v1, v2, MEMORIA_SOURCE);

            for (Int c = 0; c < transfers_; c++)
            {
                block_size_ = getRandom(max_block_size_ - min_block_size_) + min_block_size_;
                pos1_ = getRandom(v1.size() - block_size_);
                pos2_ = getRandom(v1.size());

                insertData(v1, v2);

                check(allocator, "Allocator check failed",  MEMORIA_SOURCE);

                compareVectors(v1, v2, MEMORIA_SOURCE);

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
