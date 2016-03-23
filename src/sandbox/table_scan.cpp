// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <memoria/v1/containers/map/map_factory.hpp>
#include <memoria/v1/memoria.hpp>
#include <memoria/v1/containers/table/table_factory.hpp>
#include <memoria/v1/containers/seq_dense/seqd_factory.hpp>
#include <memoria/v1/containers/vector/vctr_factory.hpp>

#include <memoria/v1/core/container/metadata_repository.hpp>

#include <memoria/v1/core/tools/time.hpp>
#include <memoria/v1/core/tools/random.hpp>

using namespace memoria;
using namespace memoria::tools;
using namespace std;

using CtrT      = DCtrTF<Table<BigInt, Byte, PackedSizeType::FIXED>>::Type;
//using Provider    = memoria::bttl::RandomDataInputProvider<CtrT, RngInt>;
using Provider  = memoria::bttl::DeterministicDataInputProvider<CtrT>;
using Position  = CtrT::Types::Position;


struct ScanFn {
    BigInt value_ = 0;

    template <typename Stream>
    void operator()(const Stream* obj, Int start, Int end)
    {
        value_++;
    }
};


int main(int argc, const char** argv, const char** envp) {
    MEMORIA_INIT(DefaultProfile<>);

    Seed(123);
    SeedBI(456);

    try {
        SmallInMemAllocator alloc;

        alloc.mem_limit() = 2*1024*1024*1024ll;

        CtrT::initMetadata();

        CtrT ctr(&alloc);

        ctr.setNewPageSize(16*1024);

        auto iter = ctr.seek(0);

        Int rows        = 1000000;
        Int cols        = 10;
        Int data_size   = 111;

        BigInt c0 = getTimeInMillis();

//      Provider provider({rows, cols, data_size}, getGlobalIntGenerator());
        Provider provider({rows, cols, data_size});

        ctr._insert(iter, provider, 1000000);


        BigInt c1 = getTimeInMillis();

        cout<<"Table Constructed in "<<FormatTime(c1 - c0)<<" s"<<endl;

        alloc.commit();

        if (argc > 1)
        {
            const char* dump_name = argv[1];

            cout<<"Dump to: "<<dump_name<<endl;

            OutputStreamHandler* os = FileOutputStreamHandler::create(dump_name);
            alloc.store(os);
            delete os;
        }

        ScanFn scan_fn;

        BigInt t0 = getTimeInMillis();

        for (int x = 0; x < 1; x++)
        {
            BigInt tt0 = getTimeInMillis();

            iter = ctr.seek(0);
            for (Int r = 0; r < rows; r++)
            {
                MEMORIA_ASSERT(iter.pos(), ==, r);
                MEMORIA_ASSERT(iter.cache().abs_pos()[0], ==, r);
                MEMORIA_ASSERT(iter.size(), ==, rows);

                iter.toData();

                for (Int c = 0; c < cols; c++)
                {
                    MEMORIA_ASSERT(iter.pos(), ==, c);
                    MEMORIA_ASSERT(iter.size(), ==, cols);

                    iter.toData();

                    iter.template scan<IntList<2>>(scan_fn);
                    MEMORIA_ASSERT_TRUE(iter.isSEnd());

                    iter.toIndex();
                    iter.skipFw(1);

                    if (c == cols -1){
                        MEMORIA_ASSERT_TRUE(iter.isSEnd());
                    }
                }

                iter.toIndex();
                iter.skipFw(1);
            }

            BigInt tt1 = getTimeInMillis();

            cout<<"One Scan finished in "<<FormatTime(tt1 - tt0)<<endl;
        }

        BigInt t1 = getTimeInMillis();

        cout<<"All Scans finished in "<<FormatTime(t1 - t0)<<endl;

        cout<<"Done"<<endl;
    }
    catch (memoria::Exception& ex) {
        cout<<ex.message()<<" at "<<ex.source()<<endl;
    }
}
