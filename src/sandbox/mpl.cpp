
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

vector<Byte> create_random_vector(size_t size)
{
	vector<Byte> data(size);
	for (auto& v: data) v = getRandomG(255);
	return data;
}

vector<Byte> create_vector(size_t size, Byte fill_value = 0)
{
	vector<Byte> data(size);
	for (auto& v: data) v = fill_value;
	return data;
}


int main()
{
	MEMORIA_INIT(DefaultProfile<>);

	DCtr<Vector<Byte>>::initMetadata();

	try {
		auto alloc = PersistentInMemAllocator<>::create();

		auto txn1 = alloc->master()->branch();

		DCtr<Vector<Byte>> ctr1(txn1.get(), CTR_CREATE);

		auto ctr_name = ctr1.name();

		vector<Byte> data1 = create_random_vector(10000);
		ctr1.seek(0).insert(data1.begin(), data1.size());

		txn1->freeze();

		FSDumpAllocator(txn1, "pdump1.dir");

		cout<<"Create new snapshot"<<endl;
		auto txn2 = txn1->branch();

		DCtr<Vector<Byte>> ctr2(txn2.get(), CTR_FIND, ctr_name);

		vector<Byte> data2 = create_vector(10000, 0x22);
		ctr2.End().insert(data2.begin(), data2.size());

//		txn2->dump_persistent_tree();

//		FSDumpAllocator(txn2, "pdump2_t.dir");

		txn2->freeze();
		txn2->set_as_master();

		cout<<"Clear Txn1"<<endl;
		txn1->drop();
		txn1.reset();

//		txn2->dump_persistent_tree();

		FSDumpAllocator(txn2, "pdump2_t.dir");

		FSDumpAllocator(alloc->master(), "pdump2.dir");

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
	}
	catch (vapi::Exception& ex) {
		cout<<ex.source()<<": "<<ex.message()<<endl;
	}
	catch (vapi::Exception* ex) {
		cout<<ex->source()<<": "<<ex->message()<<endl;
	}

	MetadataRepository<DefaultProfile<>>::cleanup();
}


