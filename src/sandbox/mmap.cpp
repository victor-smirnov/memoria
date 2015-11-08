// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <memoria/containers/map/map_factory.hpp>
#include <memoria/memoria.hpp>
#include <memoria/containers/seq_dense/seqd_factory.hpp>
#include <memoria/containers/vector/vctr_factory.hpp>

#include <memoria/core/container/metadata_repository.hpp>


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

		using CtrT  = DCtrTF<Map<BigInt, Vector<Byte>>>::Type;

		CtrT::initMetadata();

		CtrT map(&alloc);

		auto iter = map.find(0);

		using Provider = mmap::RandomDataInputProvider<CtrT, decltype(generator)>;

		Provider provider(map, 10000000, 10, generator);

		using Position = Provider::Position;

		map.insert_provided_data(iter.leaf(), Position(), provider);

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
