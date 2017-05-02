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
#include <memoria/v1/core/tools/fixed_array.hpp>
#include <memoria/v1/core/tools/dump.hpp>

#include <memory>
#include <vector>

using namespace memoria::v1;
using namespace std;

template <typename Key, typename Value>
using MapData = vector<pair<Key, vector<Value>>>;


using MMapIOBuffer = DefaultIOBuffer;

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
            make_pair(key_fn(c + 1), std::move(val))
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
            make_pair(key_fn(c + 1), std::move(val))
        );
    }

    return data;
}





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

template <typename V, int32_t N>
FixedArray<N> make_value(V&& num, TypeTag<FixedArray<N>>)
{
    FixedArray<N> array;

    *T2T<std::remove_reference_t<V>*>(array.data()) = num;

    return array;
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






class MMapBufferConsumer: public bt::BufferConsumer<MMapIOBuffer> {
    using IOBuffer = MMapIOBuffer;

    IOBuffer io_buffer_;
public:
    MMapBufferConsumer(): io_buffer_(65536) {}

    virtual IOBuffer& buffer() {return io_buffer_;}
    virtual int32_t process(IOBuffer& buffer, int32_t entries)
    {
//      cout << "Consume " << entries << " entries" << endl;
        return entries;
    }
};






int main()
{
    MEMORIA_INIT(DefaultProfile<>);

    using KeyType   = int64_t;
    using ValueType = uint64_t;//FixedArray<32>;

    using CtrName   = Map<KeyType, Vector<ValueType>>;

    DInit<CtrName>();

    try {
        auto alloc = PersistentInMemAllocator<>::create();
        auto snp   = alloc->master()->branch();
        try {
            auto map = create<CtrName>(snp, UUID(10000, 20000));

            map->setNewPageSize(32768);

            int32_t keys = 3;

            auto map_data = createRandomShapedMapData<KeyType, ValueType>(
                    keys,
                2000000,
                [](auto k) {return make_key(k, TypeTag<KeyType>());},
                [](auto k, auto v) {return make_value(k, TypeTag<ValueType>());}
            );

            mmap::MultimapIOBufferProducer<KeyType, ValueType> iobuf_adapter(map_data, 65536);

            long t0 = getTimeInMillis();
            auto totals = map->begin()->bulkio_insert(iobuf_adapter);
            long t1 = getTimeInMillis();

            cout << "Totals: " << totals << ", time " << (t1 - t0) << endl;

            FSDumpAllocator(snp, "mmap.dir");


//            auto ii = map->select(1, 0);
//
//            ii->dumpHeader();
//
//            for (int c = 0; c < 1000 && !ii->is_end(); c++)
//            {
//              ii->selectFw(1, 0);
//              ii->dumpHeader();
//              cout << endl << endl;
//            }



            auto iii = map->begin();
            int32_t c = 0 ;
            while (!iii->is_end())
            {
                iii->dumpHeader();
                cout << "Count: " << iii->countFw() << " -- " << std::get<1>(map_data[(c++)/2]).size() << endl << endl << endl;
            }



            /*
            MMapBufferConsumer consumer;

            int64_t tr0 = getTimeInMillis();
            map->begin()->bulkio_read(&consumer);
            int64_t tr1 = getTimeInMillis();

            cout << "Read Time: " << (tr1 - tr0) << endl;




            auto iter0 = map->begin();

            auto walker = iter0->template create_scan_walker<MMapIOBuffer>();

            MMapIOBuffer io_buffer(65536);

            int64_t trr0 = getTimeInMillis();

            size_t size = 0;

            while(true)
            {
                int32_t entries = iter0->bulkio_populate(*walker.get(), &io_buffer);
                if (entries > 0) {
                    size += entries;
                }
                else if (entries < 0) {
                    size += -entries;
                    break;
                }
            }

            int64_t trr1 = getTimeInMillis();

            cout << "Populate Time: " << (trr1 - trr0) << " entries: " << size << endl;
*/

//            auto map2 = create<CtrName>(snp);
//
//            auto iter = map->begin();
//            using CtrT = decltype(map)::element_type;
//
//            ChainedIOBufferProducer<MMapIOBuffer, CtrT::Iterator> chained_producer(iter.get(), 65536);
//
//            long ti0 = getTimeInMillis();
//            auto totals2 = map2->begin()->bulkio_insert(chained_producer);
//            long ti1 = getTimeInMillis();
//
//            cout << "Totals: " << totals2 << ", time " << (ti1 - ti0) << endl;

            snp->commit();
            snp->set_as_master();

            check_snapshot(snp);

//            FSDumpAllocator(alloc, "mmap.dir");

            alloc->store("mmap.memoria");
        }
        catch (...) {
            //FSDumpAllocator(snp, "mmap_fail.dir");
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
