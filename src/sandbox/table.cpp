// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <memoria/memoria.hpp>
#include <memoria/containers/mapx/mapx_factory.hpp>
#include <memoria/containers/table/table_factory.hpp>
#include <memoria/containers/seq_dense/seqd_factory.hpp>
#include <memoria/containers/vector/vctr_factory.hpp>

#include <memoria/core/container/metadata_repository.hpp>


std::uniform_int_distribution<int>      distribution;
std::mt19937_64                         engine;
auto                                    generator               = std::bind(distribution, engine);


using namespace memoria;
using namespace std;

int main(int argc, const char** argv, const char** envp) {
	MEMORIA_INIT(SmallProfile<>);

	try {
		SmallInMemAllocator alloc;

		alloc.mem_limit() = 2*1024*1024*1024ll;

		using CtrT  = SCtrTF<Table<BigInt, Byte>>::Type;

		CtrT::initMetadata();

		CtrT ctr(&alloc);

		auto iter = ctr.find(0);
//		iter.template _skipFw<0>(1);
//		iter.template _skipBw<0>(1);

		using Provider = table::RandomDataInputProvider<CtrT, decltype(generator)>;

		Provider provider(ctr, 1000000, 10, 100, generator);

		using Position = Provider::Position;

		ctr.insertData(iter.leaf(), Position(), provider);

		iter.leaf_rank(0);

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
	catch (memoria::vapi::Exception& ex) {
		cout<<ex.message()<<" at "<<ex.source()<<endl;
	}
}
