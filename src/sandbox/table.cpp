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


std::uniform_int_distribution<int>      distribution;
std::mt19937_64                         engine;
auto                                    generator               = std::bind(distribution, engine);


using namespace memoria;
using namespace std;

int main(int argc, const char** argv, const char** envp) {
    MEMORIA_INIT(DefaultProfile<>);

    try {
        SmallInMemAllocator alloc;

        alloc.mem_limit() = 2*1024*1024*1024ll;

        using CtrT  = DCtrTF<Table<BigInt, Byte>>::Type;

        CtrT::initMetadata();

        CtrT ctr(&alloc);

        auto iter = ctr.seek(0);

        using Provider = table::RandomDataInputProvider<CtrT, decltype(generator)>;

        Provider provider(ctr, 100, 10, 10, generator);

        using Position = Provider::Position;

        ctr.insert_provided_data(iter.leaf(), Position(), provider);

        cout<<"Data inserted!"<<endl;

        cout<<"Stream 0"<<endl;
        iter = ctr.seek(26);
        iter.dumpHeader();
        cout<<endl;

        cout<<"Stream 1"<<endl;
        iter.toData(9);
        iter.dumpHeader();
        cout<<endl;

        cout<<"Stream 2"<<endl;
        iter.toData(10);
        iter.dumpHeader();
        cout<<endl;

        cout<<"Stream 1"<<endl;
        iter.toIndex();
        iter.dumpHeader();
        cout<<endl;

        cout<<"Stream 0"<<endl;
        iter.toIndex();
        iter.dumpHeader();
        cout<<endl;

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
