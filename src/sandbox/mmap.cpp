// Copyright Victor Smirnov 2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <memoria/memoria.hpp>

#include <memoria/containers/multimap/mmap_factory.hpp>

#include <memoria/core/container/metadata_repository.hpp>

#include <memoria/core/tools/time.hpp>
#include <memoria/core/tools/random.hpp>

#include <memory>
#include <vector>

using namespace memoria;
using namespace std;

template <typename Key, typename Value>
using MapData = vector<pair<Key, vector<Value>>>;


template <typename Key, typename Value, typename Fn1, typename Fn2>
MapData<Key, Value> createMapData(size_t keys, size_t values, Fn1&& key_fn, Fn2&& value_fn)
{
	MapData<Key, Value> data;

	for (size_t c = 0;c < keys; c++)
	{
		vector<Value> val;

		for (size_t v = 0; v < values; v++)
		{
			val.push_back(value_fn(c, v));
		}

		data.push_back(
			make_pair(key_fn(c), std::move(val))
		);
	}

	return data;
}

template <typename Value, typename Fn2>
vector<Value> createValueData(size_t values, Fn2&& value_fn)
{
	vector<Value> data;

	for (size_t v = 0; v < values; v++)
	{
		data.push_back(value_fn(v));
	}

	return data;
}

template <typename Key, typename Fn2>
vector<Key> createKeyData(size_t keys, Fn2&& key_fn)
{
	vector<Key> data;

	for (size_t v = 0; v < keys; v++)
	{
		data.push_back(key_fn(v));
	}

	return data;
}


template <typename Key, typename Value, typename Fn1, typename Fn2>
MapData<Key, Value> createRandomShapedMapData(size_t keys, size_t values, Fn1&& key_fn, Fn2&& value_fn)
{
	MapData<Key, Value> data;

	for (size_t c = 0;c < keys; c++)
	{
		vector<Value> val;

		size_t values_size = getRandomG(values);

		for (size_t v = 0; v < values_size; v++)
		{
			val.push_back(value_fn(c, v));
		}

		data.push_back(
			make_pair(key_fn(c), std::move(val))
		);
	}

	return data;
}



template <typename T> struct TypeTag {};

template <typename V, typename T>
T make_key(V&& num, TypeTag<T>) {
	return num;
}

template <typename V>
String make_key(V&& num, TypeTag<String>)
{
	stringstream ss;
	ss<<"'";
	ss.width(16);
	ss << num;
	ss<<"'";
	return ss.str();
}

template <typename V>
UUID make_key(V&& num, TypeTag<UUID>)
{
	return UUID(num);
}



template <typename V, typename T>
T make_value(V&& num, TypeTag<T>) {
	return num;
}

template <typename V>
String make_value(V&& num, TypeTag<String>)
{
	stringstream ss;
	ss << num;
	return ss.str();
}

template <typename V>
UUID make_value(V&& num, TypeTag<UUID>)
{
	if (num != 0) {
		return UUID::make_random();
	}
	else {
		return UUID();
	}
}


int main() {
	MEMORIA_INIT(DefaultProfile<>);

	using KeyType 	= double;
	using ValueType = String;

	using CtrName = Map<KeyType, Vector<ValueType>>;

	DInit<CtrName>();

	try {
		auto alloc = PersistentInMemAllocator<>::create();
		auto snp   = alloc->master()->branch();
		try {
			auto map   = create<CtrName>(snp);

			using Ctr  = typename DCtrTF<CtrName>::Type;

			auto map_data = createRandomShapedMapData<KeyType, ValueType>(
					10,
					10000,
					[](auto k) {return make_key(k, TypeTag<KeyType>());},
					[](auto k, auto v) {return make_value(getRandomG(), TypeTag<ValueType>());}
			);

			auto iter = map->begin();

			using EntryAdaptor 	= mmap::MMapAdaptor<Ctr>;
			using ValueAdaptor 	= mmap::MMapValueAdaptor<Ctr>;
			using KeyAdaptor 	= mmap::MMapKeyAdaptor<Ctr>;

			EntryAdaptor stream_adaptor(map_data);

			auto totals = map->_insert(*iter.get(), stream_adaptor);

			auto sizes = map->sizes();
			MEMORIA_ASSERT(totals, ==, sizes);

			auto iter2 = map->seek(8);

			iter2->toData();

			auto map_values = createValueData<ValueType>(100000, [](auto x){return make_value(0, TypeTag<ValueType>());});
			ValueAdaptor value_adaptor(map_values);

			map->_insert(*iter2.get(), value_adaptor);

			auto key_data = createKeyData<KeyType>(1000, [](auto k){return make_key(k + 1000, TypeTag<KeyType>());});
			KeyAdaptor key_adaptor(key_data);

			auto iter3 = map->end();

			map->_insert(*iter3.get(), key_adaptor);

			int idx = 0;

			auto s_iter = map->seek(9);

			s_iter->scan_values([&](auto&& value){
				cout << "Value: " << (idx++) << " " << hex << value << dec << endl;
			});

			idx = 0;
			map->seek(5)->scan_keys([&](auto&& value){
				cout << "Key: " << (idx++) << " " << value << endl;
			});


			auto iter4 = map->seek(8);

			iter4->remove();

			snp->commit();

//
//			auto values2 = map->seek(2)->read_values();
//
//			dumpVector(cout, values2);
//
//			cout << "Values2.size = " << values2.size() << endl;

			FSDumpAllocator(snp, "mmap.dir");

			check_snapshot(snp);

			cout << "Totals: " << totals << endl;
		}
		catch (...) {
			FSDumpAllocator(snp, "mmap_fail.dir");
			throw;
		}
	}
	catch (memoria::Exception& ex) {
		cout << ex.message() << " at " << ex.source() << endl;
	}
	catch (memoria::PackedOOMException& ex) {
		cout << "PackedOOMException at " << ex.source() << endl;
	}

	MetadataRepository<DefaultProfile<>>::cleanup();
}
