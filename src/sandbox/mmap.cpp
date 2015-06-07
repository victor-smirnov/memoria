// Copyright Victor Smirnov 2015+.
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


		using CtrT  = SCtrTF<Map<BigInt, Vector<Byte>>>::Type;

		CtrT::initMetadata();

		CtrT map(&alloc);

		auto iter = map.find(0);

		iter.key();


		alloc.commit();

		OutputStreamHandler* os = FileOutputStreamHandler::create("mapxx.dump");

		alloc.store(os);

		delete os;
	}
	catch (memoria::vapi::Exception& ex) {
		cout<<ex.message()<<" at "<<ex.source()<<endl;
	}
}
