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
#include <memoria/v1/core/tools/random.hpp>


using namespace memoria;
using namespace v1::tools;
using namespace std;

using CtrT      = DCtrTF<Table<int64_t, int16_t>>::Type;
using Provider  = bttl::RandomDataInputProvider<CtrT, RngInt>;

template<int32_t StreamIdx>
using Adapter   = CtrT::Types::template InputTupleAdapter<StreamIdx>;

using IV = std::initializer_list<int64_t>;

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
        int32_t data_size   = 50;

        Provider provider({rows + 1, cols, data_size}, getGlobalIntGenerator());

        ctr._insert(iter, provider);

//      iter = ctr.seek(0);

//      ctr.add_to_stream_counter(iter.leaf(), iter.stream(), iter.idx(), 1001);

//      iter.dump();

//      iter.toData(5);
//      iter.toIndex();
//      iter.skipFw(1);
//      iter.skipBw(1);
//
//      iter.split();

//      iter.toData(1);

//      iter.template _insert<2>(Adapter<2>::convert(0xFF));


//      iter.template _insert<0>(Adapter<0>::convert(IV{22, 0}));
//
//      iter = ctr.seek(0);

//      iter.skipFw(2);
//      iter.toData(3);
//
//
//      iter.dump();
//
//      iter.remove_subtrees(6);
//
//      cout<<"*******************************===================*****************************"<<endl;
//
//      iter.dump();

        alloc.commit();

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
