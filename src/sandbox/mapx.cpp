// Copyright Victor Smirnov 2014-2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#include <memoria/memoria.hpp>
#include <memoria/containers/mapx/mapx_factory.hpp>
#include <memoria/core/container/metadata_repository.hpp>


using namespace memoria;
using namespace std;

int main() {
	MEMORIA_INIT(SmallProfile<>);

	try {
		SmallInMemAllocator alloc;

		using RootT = SCtrTF<Root>::Type;
		using CtrT  = SCtrTF<MapX<BigInt, BigInt>>::Type;

		CtrT::initMetadata();

		CtrT ctr(&alloc);

		auto iter = ctr.Begin();

		Int size = 1000;

		for (Int c = 1; c <= size; c++)
		{
			iter.insert(1, c);
		}

		iter = ctr.findK(0);

//		iter.dump();

//		iter.skipBw(size - 100);
//		iter.skipBw(1);

//		iter.findBwGE(0, size - 1);


//		iter.findFwGT(0, size);
		iter.skipFw(size - 2);

		iter.dump();

//		DebugCounter = 1;
		iter.skipFw(2);

		iter.dump();

		iter.skipFw(2);
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
