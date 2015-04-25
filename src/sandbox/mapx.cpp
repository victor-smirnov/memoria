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
		using MapT  = SCtrTF<MapX<BigInt, BigInt>>::Type;
		using SecT  = SCtrTF<Sequence<1>>::Type;
		using VecT  = SCtrTF<Vector<Int>>::Type;

		MapT::initMetadata();
		VecT::initMetadata();
		SecT::initMetadata();
		RootT::initMetadata();

		MapT map(&alloc);

		auto iter = map.Begin();

		int size = 10;

		for (int c = 0; c < size; c++) {
			iter.insert(1, c);
		}

		iter = map.findK(3);

		iter.dump();

		iter.template _updateStream<0, IntList<0>>(MakeStaticVector<BigInt>(456));
		iter.template _updateStream<0, IntList<1>>(555);

		iter.dump();

		iter.template _updateStream<0, IntList<0, 1>>(MakeStaticVector<BigInt>(777), 333);

		cout<<"LeafEntry: "<<iter.template _readLeafEntry<0, IntList<0, 1>>(iter.idx())<<endl;
		cout<<"Key: "<<iter.key()<<endl;
		cout<<"Value: "<<iter.value()<<endl;

		iter.dump();


//		iter.findFwGT(0, 100);
//		iter.findFwGE(0, 100);
//		iter.findBwGT(0, 100);
//		iter.findBwGE(0, 100);

//		iter.skipBw(1);
		iter.remove();

		iter.dump();

		alloc.commit();

		OutputStreamHandler* os = FileOutputStreamHandler::create("mapxx.dump");

		alloc.store(os);

		delete os;
	}
	catch (memoria::vapi::Exception& ex) {
		cout<<ex.message()<<" at "<<ex.source()<<endl;
	}
}
