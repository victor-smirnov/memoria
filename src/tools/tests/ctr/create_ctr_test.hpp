
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_CREATE_CTR_CREATE_CTR_TEST_HPP_
#define MEMORIA_TESTS_CREATE_CTR_CREATE_CTR_TEST_HPP_

#include <memoria/tools/profile_tests.hpp>
#include <memoria/tools/tools.hpp>

#include <vector>

#include "../tests_inc.hpp"



namespace memoria {



class CreateCtrTest: public SPTestTask {

    typedef CreateCtrTest                                                       MyType;

    typedef KVPair<BigInt, BigInt>                                              Pair;
    typedef vector<Pair>                                                        PairVector;
    typedef DCtrTF<WT>::Type                                                    WTCtr;
    typedef DCtrTF<Map<BigInt, BigInt>>::Type                                   MapCtr;

    PairVector pairs_;

    Int map_size_           = 10000;
    Int wt_size_            = 500;

    Int iteration_          = 0;

    BigInt map_name_;
    BigInt wt_name_;

public:

    CreateCtrTest(): SPTestTask("Create")
    {
        MapCtr::initMetadata();


        MEMORIA_ADD_TEST_PARAM(map_size_)->setDescription("Size of the Map container");
        MEMORIA_ADD_TEST_PARAM(wt_size_)->setDescription("Size of the WaveletTree container");


        MEMORIA_ADD_TEST_PARAM(map_name_)->state();
        MEMORIA_ADD_TEST_PARAM(wt_name_)->state();
        MEMORIA_ADD_TEST_PARAM(iteration_)->state();


        MEMORIA_ADD_TEST(runCreateCtrTest);
        MEMORIA_ADD_TEST(runStoreTest);
    }

    virtual ~CreateCtrTest() throw() {}


    void assertEmpty(const char* src, Allocator& allocator)
    {
        AssertEQ(src, allocator.size(), 0);
    }

    void assertSize(const char* src, Allocator& allocator, Int size)
    {
        AssertEQ(src, allocator.size(), size);
    }

    void runCreateCtrTest()
    {
        DefaultLogHandlerImpl logHandler(out());

        Allocator allocator;
        allocator.getLogger()->setHandler(&logHandler);
        allocator.commit();

        assertEmpty(MA_SRC, allocator);

        AssertThrows<Exception>(MA_SRC, []{
            MapCtr map(nullptr);
        });

        AssertThrows<Exception>(MA_SRC, [&]{
            MapCtr map(&allocator, 0);
        });

        assertEmpty(MA_SRC, allocator);

        AssertThrows<NoCtrException>(MA_SRC, [&]{
            MapCtr map(&allocator, CTR_FIND, 12345);
        });

        assertEmpty(MA_SRC, allocator);

        // Ensure subsequent CTR_FIND with the same name
        // doesn't affect allocator
        AssertThrows<NoCtrException>(MA_SRC, [&]{
            MapCtr map(&allocator, CTR_FIND, 12345);
        });

        assertEmpty(MA_SRC, allocator);

        AssertDoesntThrow(MA_SRC, [&]{
            MapCtr map(&allocator, CTR_CREATE | CTR_FIND, 12345);
        });

        assertSize(MA_SRC, allocator, 1);

        AssertThrows<CtrTypeException>(MA_SRC, [&]{
            WTCtr map(&allocator, CTR_FIND, 12345);
        });

        assertSize(MA_SRC, allocator, 1);

        AssertThrows<NoCtrException>(MA_SRC, [&]{
            WTCtr map(&allocator, CTR_FIND, 12346);
        });

        assertSize(MA_SRC, allocator, 1);

        AssertDoesntThrow(MA_SRC, [&]{
            WTCtr map(&allocator, CTR_FIND | CTR_CREATE, 12346);
        });

        assertSize(MA_SRC, allocator, 2);

        AssertDoesntThrow(MA_SRC, [&]{
            WTCtr map(&allocator, CTR_FIND | CTR_CREATE, 12346);
        });

        assertSize(MA_SRC, allocator, 2);

        AssertDoesntThrow(MA_SRC, [&]{
            WTCtr map(&allocator, CTR_FIND, 12346);
        });

        assertSize(MA_SRC, allocator, 2);

        BigInt name;

        {   // Container object lifecycle scope.

            MapCtr map(&allocator);
            name = map.name();

            assertSize(MA_SRC, allocator, 3);

            for (Int c = 0; c < 1000; c++)
            {
                map[c] = c + 1;
            }

            // Container's data still exists in allocator
        }   // after control leaves the cope

        AssertEQ(MA_SRC, name, INITAL_CTR_NAME_COUNTER + 1);

        assertSize(MA_SRC, allocator, 3);

        MapCtr map(&allocator, CTR_FIND, name);

        for (auto pair: map)
        {
            AssertEQ(MA_SRC, pair.first, pair.second - 1);
        }

        //Container removal is not fully implemented yet
        //map.drop();

        assertSize(MA_SRC, allocator, 3);

        allocator.commit();

        assertSize(MA_SRC, allocator, 3);


        BigInt name1 = allocator.createCtrName();

        allocator.rollback();

        BigInt name2 = allocator.createCtrName();

        AssertEQ(MA_SRC, name1, name2);
    }


    void runStoreTest()
    {
        DefaultLogHandlerImpl logHandler(out());

        Allocator allocator;
        allocator.getLogger()->setHandler(&logHandler);

        MapCtr map(&allocator);

        map_name_ = map.name();

        BigInt t00 = getTimeInMillis();

        for (Int c = 0; c < map_size_; c++)
        {
            map[getRandom()] = getRandom();
        }

        WTCtr wt_ctr(&allocator);
        wt_ctr.prepare();

        wt_name_ = wt_ctr.name();

        for (Int c = 0; c < wt_size_; c++)
        {
            wt_ctr.insert(c, getRandom());
        }

        allocator.commit();

        forceCheck(allocator, MA_SRC);

        BigInt t0 = getTimeInMillis();

        String name = this->getResourcePath("alloc1.dump");

        StoreAllocator(allocator, name);

        BigInt t1 = getTimeInMillis();

        Allocator new_alloc;

        LoadAllocator(new_alloc, name);

        BigInt t2 = getTimeInMillis();

        out()<<"Store Time: "<<FormatTime(t1 - t0)<<endl;
        out()<<"Load Time:  "<<FormatTime(t2 - t1)<<endl;

        forceCheck(new_alloc, MA_SRC);

        MapCtr new_map(&new_alloc, CTR_FIND, map.name());

        AssertEQ(MA_SRC, map.size(), new_map.size());

        auto new_iter = new_map.Begin();

        for (auto iter = map.Begin(); !iter.isEnd(); iter++, new_iter++)
        {
            AssertEQ(MA_SRC, iter.key(), new_iter.key());
            AssertEQ(MA_SRC, iter.value(), new_iter.value());
        }

        BigInt t22 = getTimeInMillis();

        WTCtr new_wt(&new_alloc, CTR_FIND, wt_ctr.name());

        AssertEQ(MA_SRC, wt_ctr.size(), new_wt.size());

        for (Int c = 0; c < wt_ctr.size(); c++)
        {
            auto sym1 = wt_ctr.value(c);
            auto sym2 = new_wt.value(c);

            AssertEQ(MA_SRC, sym1, sym2);
        }

        BigInt t33 = getTimeInMillis();

        out()<<"Create Time: "<<FormatTime(t0 - t00)<<endl;
        out()<<"check Time:  "<<FormatTime(t22 - t2)<<endl;
        out()<<"check Time:  "<<FormatTime(t33 - t22)<<endl;
    }


    template <typename T>
    void compareBuffers(const vector<T>& src, const vector<T>& tgt, const char* source)
    {
        AssertEQ(source, src.size(), tgt.size(), SBuf()<<"buffer sizes are not equal");

        for (size_t c = 0; c < src.size(); c++)
        {
            auto v1 = src[c];
            auto v2 = tgt[c];

            AssertEQ(source, v1, v2, [=](){return SBuf()<<"c="<<c;});
        }
    }

};


}


#endif

