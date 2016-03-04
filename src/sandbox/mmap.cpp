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

		auto map = create<CtrName>(snp);

		using Ctr = typename DCtrTF<CtrName>::Type;

		auto map_data = createRandomShapedMapData<KeyType, ValueType>(
			400,
			1000,
//			[](auto k) {return "str_"+toString(k);},
			[](auto k) {return k;},
			[](auto k, auto v) {return getRandomG(v);}
		);

		auto iter = map->begin();

		using StreamAdaptor = mmap::SizedStreamAdaptor<MapData<KeyType, ValueType>>;
		using InputProvider = mmap::MMapStreamingAdapter<Ctr, StreamAdaptor>;

		StreamAdaptor stream_adaptor(map_data);

		InputProvider provider(stream_adaptor);

		auto totals = map->_insert(*iter.get(), provider);

		auto sizes = map->sizes();
		MEMORIA_ASSERT(totals, ==, sizes);

		snp->commit();

		check_snapshot(snp);

		FSDumpAllocator(snp, "mmap.dir");

		cout << "Totals: " << totals << endl;
	}
	catch (memoria::Exception& ex) {
		cout << ex.message() << " at " << ex.source() << endl;
	}

	catch (memoria::PackedOOMException& ex) {
		cout << "PackedOOMException at " << ex.source() << endl;
	}

	MetadataRepository<DefaultProfile<>>::cleanup();
}
