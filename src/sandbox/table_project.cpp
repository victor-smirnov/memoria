// Copyright 2015 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.



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

using CtrT      = DCtrTF<Table<int64_t, int8_t, PackedSizeType::VARIABLE>>::Type;
using Provider  = v1::bttl::DeterministicDataInputProvider<CtrT>;


struct ScanFn {
    int64_t value_ = 0;

    template <typename Stream>
    void operator()(const Stream* obj, int32_t start, int32_t end)
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

        int32_t rows        = 1000000;
        int32_t cols        = 10;
        int32_t data_size   = 100;

        int64_t c0 = getTimeInMillis();

        Provider provider({rows, cols, data_size});

        ctr._insert(iter, provider);

        int64_t c1 = getTimeInMillis();

        cout<<"Table Constructed in "<<FormatTime(c1 - c0)<<" s"<<endl;

        alloc.commit();

        ScanFn scan_fn;

        int64_t t0 = getTimeInMillis();

        for (int32_t x = 0; x < 10; x++)
        {
            int64_t tt0 = getTimeInMillis();

            iter = ctr.seek(0);

            for (int32_t r = 0; r < rows; r++)
            {
                //iter = ctr.seek(r);

                MEMORIA_V1_ASSERT(iter.pos(), ==, r);
                MEMORIA_V1_ASSERT(iter.size(), ==, rows);

                auto tmp = iter;

                iter.toData(cols/2);

                MEMORIA_V1_ASSERT(iter.pos(), ==, cols/2);
                MEMORIA_V1_ASSERT(iter.size(), ==, cols);

                iter.toData();

                iter.template scan<IntList<2>>(scan_fn);
                MEMORIA_V1_ASSERT_TRUE(iter.isSEnd());

                //iter.toIndex(); // columns
                //iter.toIndex(); // rows

                iter = tmp;

                iter.skipFw(1); // next row
            }

            int64_t tt1 = getTimeInMillis();

            cout<<"One Projection finished in "<<FormatTime(tt1 - tt0)<<endl;
        }

        int64_t t1 = getTimeInMillis();

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
