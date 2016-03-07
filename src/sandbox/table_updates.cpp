// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <memoria/containers/map/map_factory.hpp>
#include <memoria/memoria.hpp>
#include <memoria/containers/table/table_factory.hpp>
#include <memoria/containers/seq_dense/seqd_factory.hpp>
#include <memoria/containers/vector/vctr_factory.hpp>

#include <memoria/core/container/metadata_repository.hpp>

#include <memoria/core/tools/time.hpp>
#include <memoria/core/tools/random.hpp>


using namespace memoria;
using namespace memoria::tools;
using namespace std;

using CtrT      = DCtrTF<Table<BigInt, Short>>::Type;
using Provider  = bttl::RandomDataInputProvider<CtrT, RngInt>;

template<Int StreamIdx>
using Adapter   = CtrT::Types::template InputTupleAdapter<StreamIdx>;

using IV = std::initializer_list<BigInt>;

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
        Int data_size   = 50;

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
    catch (memoria::Exception& ex) {
        cout<<ex.message()<<" at "<<ex.source()<<endl;
    }
}
