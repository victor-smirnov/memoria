
// Copyright Victor Smirnov 2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <memoria/allocators/persistent-inmem/factory.hpp>

#include <iostream>
#include <string>
#include <vector>

using namespace memoria;
using namespace std;

int main()
{
	MEMORIA_INIT(DefaultProfile<>);

	DCtr<Vector<Byte>>::initMetadata();

	try {
		auto alloc = std::make_shared<PersistentInMemAllocator<>>();

		auto txn = alloc->master();

		DCtr<Vector<Byte>> ctr(txn.get(), CTR_CREATE);

		vector<Byte> data(100000);

		ctr.seek(0).insert(data.begin(), data.size());

		txn->commit();

		txn->set_as_master();

		FSDumpAllocator(txn.get(), "pdump1.dir");

		std::string file_name = "store.dump";

		unique_ptr <FileOutputStreamHandler> out(FileOutputStreamHandler::create(file_name.c_str()));
		alloc->store(out.get());

		unique_ptr <FileInputStreamHandler> in(FileInputStreamHandler::create(file_name.c_str()));
		auto alloc2 = PersistentInMemAllocator<>::load(in.get());

		FSDumpAllocator(alloc2->master().get(), "pdump2.dir");
	}
	catch (vapi::Exception& ex) {
		cout<<ex.source()<<": "<<ex.message()<<endl;
	}
	catch (vapi::Exception* ex) {
		cout<<ex->source()<<": "<<ex->message()<<endl;
	}
}


