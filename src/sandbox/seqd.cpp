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



#include <memoria/v1/memoria.hpp>
#include <memoria/v1/containers/seq_dense/seqd_factory.hpp>

#include <memoria/v1/core/tools/random.hpp>
#include <memoria/v1/core/tools/time.hpp>

using namespace memoria;
using namespace std;

int main(int argc, const char** argv, const char** envp) {
    MEMORIA_INIT(DefaultProfile<>);

    try {
        SmallInMemAllocator alloc;

        alloc.mem_limit() = 2*1024*1024*1024ll;

        constexpr Int BitsPerSymbol = 1;

        using CtrT = DCtrTF<Sequence<BitsPerSymbol>>::Type;

        CtrT::initMetadata();

        CtrT ctr(&alloc);

//      ctr.setNewPageSize(32*1024);

        auto iter = ctr.seek(0);

        using Provider = seq_dense::RandomSequenceInputProvider<CtrT>;
        Provider provider(ctr, getGlobalBigIntGenerator(), 4000000, 1000000);

        ctr.insert(iter, provider);

        alloc.commit();

        cout<<"size: "<<ctr.size()<<endl;

        auto i1 = ctr.seek(ctr.size());

        BigInt total_rank = 0;

        for (Int s = 0; s < 1<<BitsPerSymbol; s++) {
            total_rank += i1.rank(s);
        }

        cout<<"iter rank0: "<<i1.rank(0)<<endl;
        cout<<"iter rank1: "<<i1.rank(1)<<endl;

        cout<<"Total Rank: "<<total_rank<<endl;

        auto size = ctr.size();

        cout<<"ctr rank0: "<<ctr.rank(0, size+1000, 0)<<endl;
        cout<<"ctr rank1: "<<ctr.rank(0, size+1000, 1)<<endl;


        auto ii = ctr.seek(size - 1);

        cout<<"ii rank0: "<<i1.rankBw(size - 1, 0)<<endl;

        cout<<"Allocated: "<<(alloc.allocated()/1024)<<"K"<<endl;

        auto r0 = ctr.rank(0, size, 0);
        cout<<"select0: "<<ctr.select(0, r0/2).pos()<<endl;

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
    catch (v1::PackedOOMException& ex) {
        cout<<"PackedOOMException at "<<ex.source()<<endl;
    }
    catch (v1::Exception& ex) {
        cout<<ex.message()<<" at "<<ex.source()<<endl;
    }
}
