// Copyright Victor Smirnov 2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <memoria/memoria.hpp>

#include <memoria/containers/map/map_factory.hpp>
#include <memoria/containers/seq_dense/seqd_factory.hpp>
#include <memoria/containers/vector/vctr_factory.hpp>

#include <memoria/core/container/metadata_repository.hpp>

#include <memoria/core/tools/time.hpp>

#include <memory>
#include <vector>

using namespace memoria;
using namespace std;

template <typename T>
using SVector = shared_ptr<vector<T>>;





int main() {
	MEMORIA_INIT(DefaultProfile<>);

	cout<<"Field Factory: "<<HasFieldFactory<double>::Value<<endl;
	cout<<"ValueCodec: "<<HasValueCodec<double>::Value<<endl;



	using KeyType = String;
	using ValueType = String;

	DInit<Map<KeyType, ValueType>>();

	using Entry = pair<KeyType, ValueType>;


	ListPrinter<TL<int8_t, char, uint8_t>>::print(cout)<<endl;

	try {
		auto alloc = PersistentInMemAllocator<>::create();
		auto snp   = alloc->master()->branch();

		auto map = create<Map<KeyType, ValueType>>(snp);

		auto data = make_shared<vector<Entry>>(100000);

		for (size_t c = 0; c < data->size(); c++)
		{
			data->operator[](c) = Entry("key_"+toString(c), "val_"+toString(c));
		}

		map->end()->bulk_insert(data->begin(), data->end());

		FSDumpAllocator(snp, "drop_ctr_before.dir");

		map->seek(1000)->remove(map->size());

		//map->drop();

		snp->drop_ctr(map->name());

		FSDumpAllocator(snp, "drop_ctr_after.dir");

		snp->commit();



		unique_ptr <FileOutputStreamHandler> out(FileOutputStreamHandler::create("drop_ctr.dump"));
		alloc->store(out.get());
	}
	catch (memoria::Exception& ex) {
		cout << ex.message() << " at " << ex.source() << endl;
	}

	catch (memoria::PackedOOMException& ex) {
		cout << "PackedOOMException at " << ex.source() << endl;
	}

	MetadataRepository<DefaultProfile<>>::cleanup();
}
