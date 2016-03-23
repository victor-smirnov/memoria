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


using namespace memoria;
using namespace v1::tools;
using namespace std;

using CtrT      = DCtrTF<Table<BigInt, Byte, PackedSizeType::VARIABLE>>::Type;
using Provider  = v1::bttl::DeterministicDataInputProvider<CtrT>;


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

    try {
        SmallInMemAllocator alloc;

        alloc.mem_limit() = 2*1024*1024*1024ll;

        CtrT::initMetadata();

        CtrT ctr(&alloc);
        

        auto iter = ctr.seek(0);

        Int rows        = 1000000;
        Int cols        = 10;
        Int data_size   = 100;

        BigInt c0 = getTimeInMillis();

        Provider provider({rows, cols, data_size});

        ctr._insert(iter, provider);

        BigInt c1 = getTimeInMillis();

        cout<<"Table Constructed in "<<FormatTime(c1 - c0)<<" s"<<endl;

        alloc.commit();

        ScanFn scan_fn;

        BigInt t0 = getTimeInMillis();

        for (Int x = 0; x < 10; x++)
        {
            BigInt tt0 = getTimeInMillis();

            iter = ctr.seek(0);

            for (Int r = 0; r < rows; r++)
            {
                //iter = ctr.seek(r);

                MEMORIA_ASSERT(iter.pos(), ==, r);
                MEMORIA_ASSERT(iter.size(), ==, rows);

                auto tmp = iter;

                iter.toData(cols/2);

                MEMORIA_ASSERT(iter.pos(), ==, cols/2);
                MEMORIA_ASSERT(iter.size(), ==, cols);

                iter.toData();

                iter.template scan<IntList<2>>(scan_fn);
                MEMORIA_ASSERT_TRUE(iter.isSEnd());

                //iter.toIndex(); // columns
                //iter.toIndex(); // rows

                iter = tmp;

                iter.skipFw(1); // next row
            }

            BigInt tt1 = getTimeInMillis();

            cout<<"One Projection finished in "<<FormatTime(tt1 - tt0)<<endl;
        }

        BigInt t1 = getTimeInMillis();

        cout<<"All Projections finished in "<<FormatTime(t1 - t0)<<endl;


        if (argc > 1)
        {
            const char* dump_name = argv[1];

            cout<<"Dump to: "<<dump_name<<endl;

            OutputStreamHandler* os = FileOutputStreamHandler::create(dump_name);
            alloc.store(os);
            delete os;
        }

        cout<<"Done"<<endl;
    }
    catch (v1::Exception& ex) {
        cout<<ex.message()<<" at "<<ex.source()<<endl;
    }
}
