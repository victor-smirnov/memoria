
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_TESTS_FILEALLOC_PAGESHARED_TEST_HPP_
#define MEMORIA_TESTS_FILEALLOC_PAGESHARED_TEST_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/allocators/file/factory.hpp>

#include "file_alloc_test_base.hpp"

namespace memoria {

class FileAllocatorPageSharedTest: public FileAllocatorTestBase {
    typedef FileAllocatorTestBase                                               Base;
    typedef FileAllocatorPageSharedTest                                         MyType;

protected:
    typedef typename Base::Allocator                                            Allocator;

    typedef typename FCtrTF<Vector<Int>>::Type                                  Ctr;

public:

    FileAllocatorPageSharedTest(StringRef name): Base(name)
    {
        Ctr::initMetadata();

//      MEMORIA_ADD_TEST(testNewDB);
//      MEMORIA_ADD_TEST(testSingleCtr);
//      MEMORIA_ADD_TEST(testSingleIterator);
//      MEMORIA_ADD_TEST(testDeletePages);
        MEMORIA_ADD_TEST(testPageSharedPoolSize);
    }

    void testNewDB()
    {
        String name = getResourcePath("new.db");
        Allocator allocator(name, OpenMode::READ | OpenMode::WRITE | OpenMode::CREATE | OpenMode::TRUNC);

        AssertEQ(MA_SRC, allocator.locked(), 0);

        AssertEQ(MA_SRC, allocator.shared_pool_capacity(), 0);
    }

    void testSingleCtr()
    {
        String name = getResourcePath("single_ctr.db");
        Allocator allocator(name, OpenMode::READ | OpenMode::WRITE | OpenMode::CREATE | OpenMode::TRUNC);

        Ctr ctr(&allocator, CTR_CREATE);

        AssertEQ(MA_SRC, allocator.shared_pool_capacity(), 0);
    }


    void testSingleIterator()
    {
        String name = getResourcePath("single_iterator.db");
        Allocator allocator(name, OpenMode::READ | OpenMode::WRITE | OpenMode::CREATE | OpenMode::TRUNC);

        Ctr ctr(&allocator, CTR_CREATE);

        {// scope

            auto iter = ctr.seek(0);

            AssertEQ(MA_SRC, allocator.shared_pool_capacity(), 1);
        }

        AssertEQ(MA_SRC, allocator.shared_pool_capacity(), 0);
    }

    void testDeletePages()
    {
        String name = getResourcePath("delete-pages.db");
        Allocator allocator(name, OpenMode::READ | OpenMode::WRITE | OpenMode::CREATE | OpenMode::TRUNC);

        Ctr ctr(&allocator, CTR_CREATE);

        vector<Int> data(500000);

        ctr.seek(0).insert(data);

        allocator.commit();

        AssertEQ(MA_SRC, allocator.read_locked(), 0);

        auto begin = ctr.seek(0);

        AssertEQ(MA_SRC, allocator.read_locked(), 1);

        begin.remove(ctr.size());

        AssertEQ(MA_SRC, ctr.size(), 0);

        AssertEQ(MA_SRC, allocator.read_locked(), 0);

        auto begin3 = ctr.seek(0);
        auto begin2 = ctr.seek(0);
        auto begin1 = ctr.seek(0);

        AssertEQ(MA_SRC, allocator.read_locked(), 0);

        allocator.commit();

        AssertEQ(MA_SRC, allocator.read_locked(), 1);
    }

    void testPageSharedPoolSize()
    {
        String name = getResourcePath("pool-size.db");
        Allocator allocator(name, OpenMode::READ | OpenMode::WRITE | OpenMode::CREATE | OpenMode::TRUNC);

        Ctr ctr(&allocator, CTR_CREATE);

        vector<Ctr::Iterator> iter;

        AssertDoesntThrow(MA_SRC, [&allocator, &ctr, &iter](){
            for (Int c = 0; c < allocator.shared_pool_max_size() + 1; c++)
            {
                iter.push_back(ctr.seek(0));
            }
        });

        // 1 - blockmap page
        // 2 - rootmap page
        AssertEQ(MA_SRC, allocator.updated(), 2);

        allocator.commit();

        AssertEQ(MA_SRC, allocator.read_locked(), 1);
        AssertEQ(MA_SRC, allocator.updated(), 0);

        {// scope
            vector<Ctr::Iterator> iter_vector;

            AssertDoesntThrow(MA_SRC, [&allocator, &iter_vector](){

                vector<Ctr> ctr_vector;

                for (Int c = 0; c < allocator.shared_pool_max_size() - 3; c++)
                {
                    ctr_vector.emplace(ctr_vector.end(), Ctr(&allocator, CTR_CREATE));
                    iter_vector.emplace(iter_vector.end(), ctr_vector[c].seek(0));
                }
            });

            allocator.commit();

            AssertEQ(MA_SRC, allocator.read_locked(), allocator.shared_pool_max_size() - 2);
            AssertEQ(MA_SRC, allocator.shared_pool_capacity(), allocator.shared_pool_max_size() - 2);
        }

        AssertEQ(MA_SRC, allocator.read_locked(), 1);
        AssertEQ(MA_SRC, allocator.shared_pool_capacity(), 1);
    }
};

}


#endif

