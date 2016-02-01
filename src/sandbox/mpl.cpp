
// Copyright Victor Smirnov 2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <memoria/allocators/persistent-inmem/factory.hpp>
#include <memoria/core/tools/time.hpp>

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
		auto alloc = PersistentInMemAllocator<>::create();

		auto txn = alloc->master()->branch();

		DCtr<Vector<Byte>> ctr(txn.get(), CTR_CREATE);

		vector<Byte> data(100000);

		for (auto& v: data) v = getRandomG(255);

		ctr.seek(0).insert(data.begin(), data.size());

		txn->set_metadata("Transaction's metadata in plain text");

		txn->commit();

		txn->set_as_master();
		txn->set_as_branch("MyCoolBranch");


		FSDumpAllocator(txn, "pdump1.dir");

		FSDumpAllocator(alloc->master(), "pdump2.dir");
		FSDumpAllocator(alloc->find_branch("MyCoolBranch"), "pdump2_nb.dir");

		BigInt t0 = getTimeInMillis();

		std::string file_name = "store.dump";

		unique_ptr <FileOutputStreamHandler> out(FileOutputStreamHandler::create(file_name.c_str()));
		alloc->store(out.get());

		BigInt t1 = getTimeInMillis();

		unique_ptr <FileInputStreamHandler> in(FileInputStreamHandler::create(file_name.c_str()));
		auto alloc2 = PersistentInMemAllocator<>::load(in.get());

		BigInt t2 = getTimeInMillis();

		cout<<"Store: "<<FormatTime(t1 - t0)<<" Load: "<<FormatTime(t2 - t1)<<endl;

		FSDumpAllocator(alloc2->master(), "pdump3.dir");
		FSDumpAllocator(alloc2->find_branch("MyCoolBranch"), "pdump3_nb.dir");

		cout<<alloc2->find_branch("MyCoolBranch")->metadata()<<endl;
	}
	catch (vapi::Exception& ex) {
		cout<<ex.source()<<": "<<ex.message()<<endl;
	}
	catch (vapi::Exception* ex) {
		cout<<ex->source()<<": "<<ex->message()<<endl;
	}

	MetadataRepository<DefaultProfile<>>::cleanup();
}


