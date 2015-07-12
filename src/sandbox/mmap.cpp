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

		auto iter = map.find(1);

		iter.insertKey(1);
		iter.addSize(1000);

		iter.toLocalPos();

		iter.dump();

//		iter.key();
//
//		iter.seek(0);
//
//		iter+=1;
//		iter-=1;
//
//		iter.toIndex();

		mmap::RandomDataInputProvider<CtrT> provider(map, 1000, 1000);

		using Position = mmap::RandomDataInputProvider<CtrT>::Position;

		map.insertData(iter.leaf(), Position(), provider);

		alloc.commit();

		OutputStreamHandler* os = FileOutputStreamHandler::create("mapxx.dump");

		alloc.store(os);

		delete os;
	}
	catch (memoria::vapi::Exception& ex) {
		cout<<ex.message()<<" at "<<ex.source()<<endl;
	}
}
