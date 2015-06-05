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


		using CtrT  = SCtrTF<Sequence<8>>::Type;

		CtrT::initMetadata();

		CtrT ctr(&alloc);

		auto iter = ctr.Begin();

		int size = 10000;

		for (int c = 0; c < size; c++) {
			iter.insert((c % 3) > 0);
		}

		iter = ctr.Begin();

		for (int c = 0; c < size; c++)
		{
			if (iter.symbol() != (c % 3 > 0))
			{
				cout<<"Mismatch: "<<c<<iter.symbol()<<endl;
			}

			iter.setSymbol(1);

			iter++;
		}

		alloc.logger().level() = Logger::ERROR;

		alloc.commit();

		cout<<"Check "<<alloc.check()<<endl;

		OutputStreamHandler* os = FileOutputStreamHandler::create("seqdx.dump");

		alloc.store(os);

		delete os;
	}
	catch (memoria::vapi::Exception& ex) {
		cout<<ex.message()<<" at "<<ex.source()<<endl;
	}
}
