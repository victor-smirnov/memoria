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

template <typename KeyType, typename ValueType>
class MapEntryProvider {
	size_t pos_ = 0;
	SVector<pair<KeyType, ValueType>> data_;
public:
	MapEntryProvider(const SVector<pair<KeyType, ValueType>>& data): data_(data) {}
	MapEntryProvider() {}

	bool has_next() const {
		return pos_ < data_->size();
	}

	auto next()
	{
		Incrementer incr(pos_);
		return std::pair<const KeyType&, const ValueType&>(data_->at(pos_).first, data_->at(pos_).second);
	}

private:
	class Incrementer {
		size_t& value_;
	public:
		Incrementer(size_t& value): value_(value) {}
		~Incrementer() {value_++;}
	};
};


int main() {
	MEMORIA_INIT(DefaultProfile<>);

	cout<<"Field Factory: "<<HasFieldFactory<double>::Value<<endl;
	cout<<"ValueCodec: "<<HasValueCodec<double>::Value<<endl;

	ListPrinter<TL<int8_t, char, uint8_t>>::print(cout)<<endl;

	using KeyType = BigInt;
	using ValueType = UByte;

	DInit<Map<KeyType, ValueType>>();

	try {
		auto alloc = PersistentInMemAllocator<>::create();
		auto snp   = alloc->master()->branch();

		auto map = create<Map<KeyType, ValueType>>(snp);

		map->assign(-1, 0xFF);

		auto data = make_shared<vector<pair<KeyType, ValueType>>>(10);

		for (size_t c = 0; c < data->size(); c++)
		{
			//data->operator[](c) = make_pair(toString(c, true), (ValueType)c);
			data->operator[](c) = make_pair((KeyType)c, (ValueType)c);

//			data->operator[](c) = make_pair(toString(c), toString(c));
		}

		BigInt t0 = getTimeInMillis();

		cout << "Inserted: " << map->end()->bulk_insert(MapEntryProvider<KeyType, ValueType>(data)) << endl;

		BigInt t1 = getTimeInMillis();

		cout << "Insertion time: " << FormatTime(t1 - t0) << " s" << endl;

		snp->commit();

		BigInt t2 = getTimeInMillis();

		check_snapshot(snp);

		BigInt t3 = getTimeInMillis();

		cout << "Check time: " << FormatTime(t3 - t2) << " s" << endl;

		BigInt t4 = getTimeInMillis();

		FSDumpAllocator(snp, "map_batch_data.dir");

		unique_ptr <FileOutputStreamHandler> out(FileOutputStreamHandler::create("map_batch_data.dump"));
		alloc->store(out.get());

		BigInt t5 = getTimeInMillis();

		cout << "Dump time: " << FormatTime(t5 - t4) << " s" << endl;
	}
	catch (memoria::Exception& ex) {
		cout << ex.message() << " at " << ex.source() << endl;
	}

	catch (memoria::PackedOOMException& ex) {
		cout << "PackedOOMException at " << ex.source() << endl;
	}

	MetadataRepository<DefaultProfile<>>::cleanup();
}
