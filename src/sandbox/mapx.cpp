// Copyright Victor Smirnov 2014-2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#include <memoria/memoria.hpp>
#include <memoria/containers/mapx/mapx_factory.hpp>
#include <memoria/containers/seq_dense/seqd_factory.hpp>
#include <memoria/containers/vector/vctr_factory.hpp>

#include <memoria/core/container/metadata_repository.hpp>



using namespace memoria;
using namespace std;

int main() {
	MEMORIA_INIT(SmallProfile<>);

	try {
		SmallInMemAllocator alloc;

		using RootT = SCtrTF<Root>::Type;
		using CtrT  = SCtrTF<MapX<BigInt, BigInt>>::Type;
		using SecT  = SCtrTF<Sequence<1>>::Type;
		using VecT  = SCtrTF<Vector<Int>>::Type;

		CtrT::initMetadata();
		VecT::initMetadata();
		SecT::initMetadata();
		RootT::initMetadata();

		CtrT map(&alloc);

		auto iter = map.Begin();

		for (Int c = 0; c < 1000; c++) {
			iter.insert(1, c);
		}

		iter = map.findK(1000);

		iter.dumpCache(cout);


		while (iter.prevLeaf()) {
			iter.dumpCache(cout);
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
