// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <memoria/containers/map/map_factory.hpp>
#include <memoria/memoria.hpp>
#include <memoria/containers/seq_dense/seqd_factory.hpp>
#include <memoria/containers/vector/vctr_factory.hpp>

#include <memoria/core/container/metadata_repository.hpp>



using namespace memoria;
using namespace std;

int main() {
	MEMORIA_INIT(DefaultProfile<>);

	try {
		SmallInMemAllocator alloc;

		cout<<"Dummy: "<<sizeof(DCtrTF<Map<double, BigInt>>::Types::Boo)<<endl;

		using MapT  = DCtrTF<Map<double, BigInt>>::Type;



		MapT::initMetadata();

		MapT map(&alloc);

		auto iter = map.Begin();

		int size = 300;

		for (int c = 0; c < size; c++) {
			iter.insert(c + 1, c);
		}

		iter = map.find(1);

		while (!iter.isEnd()) {
			cout<<iter.key()<<" -- "<<iter.value()<<endl;
			iter++;
		}

		alloc.commit();

		OutputStreamHandler* os = FileOutputStreamHandler::create("mapxx.dump");

		alloc.store(os);

		delete os;
	}
	catch (memoria::vapi::Exception& ex) {
		cout<<ex.message()<<" at "<<ex.source()<<endl;
	}
}
