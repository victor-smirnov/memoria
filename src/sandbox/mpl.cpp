
// Copyright Victor Smirnov 2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <memoria/allocators/persistent-inmem/factory.hpp>
#include <memoria/core/tools/time.hpp>
#include <memoria/core/tools/random.hpp>

#include <memoria/containers/vector/vctr_factory.hpp>
#include <memoria/containers/map/map_factory.hpp>

#include <iostream>
#include <string>
#include <vector>

using namespace memoria;
using namespace std;

template <typename T>
vector<T> create_random_vector(size_t size, T max = 255)
{
	vector<T> data(size);
	for (auto& v: data) v = getRandomG(max);
	return data;
}

template <typename T>
vector<T> create_vector(size_t size, T fill_value = 0)
{
	vector<T> data(size);
	for (auto& v: data) v = fill_value;
	return data;
}



int main()
{
	MEMORIA_INIT(DefaultProfile<>);

	DCtr<Vector<Byte>>::initMetadata();
	DCtr<Vector<VLen<Granularity::Byte>>>::initMetadata();
	DCtr<Vector<BigInt>>::initMetadata();

	try {
		auto alloc = PersistentInMemAllocator<>::create();

		auto txn1 = alloc->master()->branch();

		auto ctr1 	= create<Vector<Byte>>(txn1);
		auto ctr1_v = create<Vector<VLen<Granularity::Byte>>>(txn1);
		auto ctr1_vv = create<Vector<BigInt>>(txn1);


		auto data1 = create_random_vector<Byte>(10000, 127);
		ctr1->seek(0)->insert(data1.begin(), data1.size());

		auto data1v = create_random_vector<BigInt>(10000);
		ctr1_v->seek(0)->insert(data1v.begin(), data1v.size());

		ctr1_vv->seek(0)->insert(data1v.begin(), data1v.size());

		auto ctr_name = ctr1->master_name();

		txn1->commit();

		FSDumpAllocator(txn1, "pdump1.dir");

		cout<<"Create new snapshot"<<endl;
		auto txn2 = txn1->branch();

		auto ctr2 = find<Vector<Byte>>(txn2, ctr_name);

		auto data2 = create_vector<Byte>(10000, 0x22);

		auto iter = ctr2->end();
		iter->insert(data2.begin(), data2.size());

		FSDumpAllocator(txn2, "pdump2_t.dir");

		txn2->commit();
		txn2->set_as_master();

		cout<<"Clear Txn1"<<endl;
		txn1->drop();
		txn1.reset();

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
	catch (vapi::MemoriaThrowable& ex) {
		ex.dump(cout);
	}
	catch (PackedOOMException& ex) {
		ex.dump(cout);
	}

	MetadataRepository<DefaultProfile<>>::cleanup();

}


