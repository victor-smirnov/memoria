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



int main() {
	MEMORIA_INIT(DefaultProfile<>);

	using KeyType = BigInteger;
	using ValueType = Byte;

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
					1000,
					//			[](auto k) {return "str_"+toString(k);},
					[](auto k) {return k;},
					//			[](auto k) {return UUID(0, k);},
					[](auto k, auto v) {return getRandomG(v);}
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

			auto map_values = createValueData<ValueType>(100000, [](auto x){return 0;});
			ValueAdaptor value_adaptor(map_values);

			map->_insert(*iter2.get(), value_adaptor);

			auto key_data = createKeyData<KeyType>(1000, [](auto k){return k + 1000;});
			KeyAdaptor key_adaptor(key_data);

			auto iter3 = map->end();

			map->_insert(*iter3.get(), key_adaptor);

			snp->commit();

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
