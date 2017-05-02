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
#include <memoria/v1/prototypes/bt_tl/bttl_input.hpp>

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





template <typename Key, typename Value>
class MapIOBufferAdapter: public bttl::iobuf::FlatTreeIOBufferAdapter<2, MMapIOBuffer> {

    using Base   = bttl::iobuf::FlatTreeIOBufferAdapter<2, MMapIOBuffer>;
    using MyType = MapIOBufferAdapter<Key, Value>;

    using typename Base::IOBuffer;

    using Data = MapData<Key, Value>;
    using Positions = core::StaticVector<int32_t, 2>;


    const Data& data_;

    IOBuffer io_buffer_;
    Positions positions_;
    int32_t level_ = 0;

    struct StructureAdapter: public bttl::iobuf::FlatTreeStructureGeneratorBase<StructureAdapter, 2> {
        MyType* adapter_;
        StructureAdapter(MyType* adapter):
            adapter_(adapter)
        {}


        auto prepare(const StreamTag<0>&)
        {
            return adapter_->data().size();
        }

        template <int32_t Idx, typename Pos>
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
        return structure_generator_.query();
    }

    virtual int32_t populate_stream(int32_t stream, IOBuffer& buffer, int32_t length)
    {
        if (stream == 1)
        {
            auto& idx    = structure_generator_.counts()[1];
            auto key_idx = structure_generator_.counts()[0];

            const auto& data = data_[key_idx - 1].second;

            int32_t c;
            for (c = 0; c < length; c++)
            {
                auto pos = buffer.pos();
                if (!IOBufferAdapter<Value>::put(buffer, data[idx]))
                {
                    buffer.pos(pos);
                    idx += c;
                    return c;
                }
            }

            idx += length;

            return c;
        }
        else {
            auto& idx = structure_generator_.counts()[0];

            int32_t c;
            for (c = 0; c < length; c++)
            {
                auto pos = buffer.pos();
                if (!IOBufferAdapter<Key>::put(buffer, data_[idx].first))
                {
                    buffer.pos(pos);
                    idx += c;
                    return c;
                }
            }

            idx += length;

            return c;
        }
    }

};



template <typename Key, typename Value>
class MapIOBufferValuesAdapter: public bttl::iobuf::FlatTreeIOBufferAdapter<2, MMapIOBuffer> {

    using Base   = bttl::iobuf::FlatTreeIOBufferAdapter<2, MMapIOBuffer>;
    using MyType = MapIOBufferValuesAdapter<Key, Value>;

    using typename Base::IOBuffer;

    using Data   = std::vector<Value>;
    using Positions = core::StaticVector<int32_t, 2>;


    const Data* data_;

    IOBuffer io_buffer_;
    Positions positions_;
    int32_t level_ = 0;

    size_t idx_ = 0;

    struct StructureAdapter: public bttl::iobuf::FlatTreeStructureGeneratorBase<StructureAdapter, 2> {

        using AdapterBase = bttl::iobuf::FlatTreeStructureGeneratorBase<StructureAdapter, 2>;

        MyType* adapter_;
        StructureAdapter(MyType* adapter):
            AdapterBase(1),
            adapter_(adapter)
        {}

        auto prepare(const StreamTag<0>&)
        {
            return 0ul;
        }

        template <int32_t Idx, typename Pos>
        auto prepare(const StreamTag<Idx>&, const Pos& pos)
        {
            return adapter_->data().size();
        }
    };


    StructureAdapter structure_generator_;

public:


    MapIOBufferValuesAdapter(size_t iobuffer_size = 65536):
        io_buffer_(iobuffer_size),
        structure_generator_(this)
    {
    }

    void setup(const Data& data)
    {
        data_ = &data;
        io_buffer_.rewind();
        structure_generator_.counts().clear();

        structure_generator_.init();
    }

    const Data& data() {return *data_;}

    virtual IOBuffer& buffer() {return io_buffer_;}

    virtual bttl::iobuf::RunDescr query()
    {
        return structure_generator_.query();
    }

    virtual int32_t populate_stream(int32_t stream, IOBuffer& buffer, int32_t length)
    {
        if (stream == 1)
        {
            auto& counts = structure_generator_.counts()[1];

            int32_t c;
            for (c = 0; c < length; c++, counts++)
            {
                auto pos = buffer.pos();
                if (!IOBufferAdapter<Value>::put(buffer, (*data_)[counts]))
                {
                    buffer.pos(pos);
                    return c;
                }
            }

            return c;
        }
        else {
            throw Exception(MA_SRC, "Invalid stream");
        }
    }

};





class MMapBufferConsumer: public bt::BufferConsumer<MMapIOBuffer> {
    using IOBuffer = MMapIOBuffer;

    IOBuffer io_buffer_;
public:
    MMapBufferConsumer(): io_buffer_(65536) {}

    virtual IOBuffer& buffer() {return io_buffer_;}
    virtual int32_t process(IOBuffer& buffer, int32_t entries)
    {
        return entries;
    }
};






int main()
{
    MEMORIA_INIT(DefaultProfile<>);

    using KeyType   = int64_t;
    using ValueType = uint64_t;//FixedArray<32>;

    using CtrName = Map<KeyType, Vector<ValueType>>;

    DInit<CtrName>();

    try {
        auto alloc = PersistentInMemAllocator<>::create();
        auto snp   = alloc->master()->branch();
        try {
            auto map = create<CtrName>(snp);

            int64_t tt, t0;
            tt = t0 = getTimeInMillis();

            int64_t thresholdInc = 100000;
            int64_t threshold = thresholdInc;

            MapIOBufferValuesAdapter<KeyType, ValueType> adapter;

            std::vector<ValueType> data(200);

            for (int64_t c = 0; c < 1; c++)
            {
                int64_t key = getBIRandomG();

                auto iter = map->find_or_create(key);

                if (iter->values_size() == 0)
                {
                    iter->toData();

                    iter->dump();

                    adapter.setup(data);

                    iter->bulkio_insert(adapter);
                }

                if (threshold == c)
                {
                    int64_t ts = getTimeInMillis();
                    cout << c << " records, time =  " << (ts - tt) << endl;
                    tt = ts;

                    threshold += thresholdInc;
                }
            }

            int64_t t1 = getTimeInMillis();

            cout << "Total keys: " << map->size() << " in " << (t1 - t0) << endl;

            FSDumpAllocator(snp, "mmap.dir");
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
