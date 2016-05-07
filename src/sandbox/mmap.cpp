// Copyright 2016 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.



#include <memoria/v1/memoria.hpp>

#include <memoria/v1/containers/multimap/mmap_factory.hpp>

#include <memoria/v1/core/container/metadata_repository.hpp>

#include <memoria/v1/core/tools/time.hpp>
#include <memoria/v1/core/tools/random.hpp>
#include <memoria/v1/prototypes/bt_tl/bttl_iobuf_input.hpp>

#include <memory>
#include <vector>

using namespace memoria::v1;
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





template <typename Key, typename Value>
class MapIOBufferAdapter: public bttl::iobuf::FlatTreeIOBufferAdapter<2, IOBuffer> {

	using Base 	 = bttl::iobuf::FlatTreeIOBufferAdapter<2, IOBuffer>;
	using MyType = MapIOBufferAdapter<Key, Value>;



	using Data = MapData<Key, Value>;
	using Positions = core::StaticVector<Int, 2>;


	const Data& data_;

	IOBuffer io_buffer_;
	Positions positions_;
	Int level_ = 0;

	struct StructureAdapter: public bttl::iobuf::FlatTreeStructureAdapterBase<StructureAdapter, 2> {
		MyType* adapter_;
		StructureAdapter(MyType* adapter):
			adapter_(adapter)
		{}


		auto prepare(const StreamTag<0>&)
		{
			return adapter_->data().size();
		}

		template <Int Idx, typename Pos>
		auto prepare(const StreamTag<Idx>&, const Pos& pos)
		{
			return adapter_->data()[pos[Idx - 1]].second.size();
		}
	};


	StructureAdapter structure_generator_;

public:


	MapIOBufferAdapter(const Data& data, size_t iobuffer_size = 65536):
		data_(data),
		io_buffer_(iobuffer_size),
		structure_generator_(this)
	{
		structure_generator_.init();
	}

	const Data& data() {return data_;}

	virtual IOBuffer& buffer() {return io_buffer_;}

	virtual bttl::iobuf::RunDescr query()
	{
		auto r = structure_generator_.query();

//		cout << "query: " << r.symbol() << " " << r.length() << endl;

		return r;
	}

	virtual Int populate_stream(Int stream, IOBuffer& buffer, Int length)
	{
		if (stream == 1)
		{
			Int c;
			for (c = 0; c < length; c++)
			{
				auto pos = buffer.pos();
				if (!IOBufferAdaptor<Value>::put(buffer, structure_generator_.counts()[0]))
				{
					buffer.pos(pos);
					structure_generator_.counts()[1] += c;
					break;
				}
			}

			structure_generator_.counts()[1] += length;

			return c;
		}
		else {
			Int c;
			for (c = 0; c < length; c++)
			{
				auto key = structure_generator_.counts()[0];
//				cout << "write key: " << key << endl;

				auto pos = buffer.pos();
				if (!IOBufferAdaptor<Key>::put(buffer, key))
				{
					buffer.pos(pos);
					structure_generator_.counts()[0] += c;
					break;
				}
			}

			structure_generator_.counts()[0] += length;

			return c;
		}
	}

};









int main() {
    MEMORIA_INIT(DefaultProfile<>);

    using KeyType   = BigInt;
    using ValueType = UByte;

    using CtrName = Map<KeyType, Vector<ValueType>>;

    DInit<CtrName>();

    try {
        auto alloc = PersistentInMemAllocator<>::create();
        auto snp   = alloc->master()->branch();
        try {
            auto map   = create<CtrName>(snp);

//

            auto map_data = createRandomShapedMapData<KeyType, ValueType>(
            		2000,
					10000,
                    [](auto k) {return make_key(k, TypeTag<KeyType>());},
                    [](auto k, auto v) {return make_value(getRandomG(), TypeTag<ValueType>());}
            );

            auto iter = map->begin();


            MapIOBufferAdapter<KeyType, ValueType> iobuf_adapter(map_data, 512);
            auto totals = map->begin()->bulkio_insert(iobuf_adapter);

//
//            using ValueAdaptor  = mmap::MMapValueAdaptor<Ctr>;
//            using KeyAdaptor    = mmap::MMapKeyAdaptor<Ctr>;

//            using Ctr = typename DCtrTF<CtrName>::Type;
//            using EntryAdaptor = mmap::MMapAdaptor<Ctr>;
//            EntryAdaptor stream_adaptor(map_data);
//            auto totals = map->begin()->bulk_insert(stream_adaptor);


            auto sizes = map->sizes();
            MEMORIA_V1_ASSERT(totals, ==, sizes);

//            auto iter2 = map->seek(8);
//
//            iter2->toData();
//
//            auto map_values = createValueData<ValueType>(100000, [](auto x){return make_value(0, TypeTag<ValueType>());});
//            ValueAdaptor value_adaptor(map_values);
//
//            map->begin()->bulk_insert(value_adaptor);
//
//            auto key_data = createKeyData<KeyType>(1000, [](auto k){return make_key(k + 1000, TypeTag<KeyType>());});
//            KeyAdaptor key_adaptor(key_data);
//
//            auto iter3 = map->end();
//
//            map->_insert(*iter3.get(), key_adaptor);
//
//            int idx = 0;
//
//            auto s_iter = map->seek(9);
//
//            s_iter->scan_values([&](auto&& value){
//                cout << "Value: " << (idx++) << " " << hex << value << dec << endl;
//            });
//
//            idx = 0;
//            map->seek(5)->scan_keys([&](auto&& value){
//                cout << "Key: " << (idx++) << " " << value << endl;
//            });


//            auto iter4 = map->seek(8);
//
//            iter4->remove();

            snp->commit();

//
//          auto values2 = map->seek(2)->read_values();
//
//          dumpVector(cout, values2);
//
//          cout << "Values2.size = " << values2.size() << endl;

            FSDumpAllocator(snp, "mmap.dir");

            check_snapshot(snp);

            cout << "Totals: " << totals << endl;
        }
        catch (...) {
        	FSDumpAllocator(snp, "mmap_fail.dir");
        	throw;
        }
    }
    catch (::memoria::v1::Exception& ex) {
        cout << ex.message() << " at " << ex.source() << endl;
    }
    catch (::memoria::v1::PackedOOMException& ex) {
        cout << "PackedOOMException at " << ex.source() << endl;
    }

    MetadataRepository<DefaultProfile<>>::cleanup();
}
