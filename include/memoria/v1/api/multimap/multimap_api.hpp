
// Copyright 2017 Victor Smirnov
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

#pragma once

#include <memoria/v1/api/common/ctr_api_btfl.hpp>

#include <memoria/v1/api/multimap/multimap_output.hpp>
#include <memoria/v1/api/multimap/multimap_input.hpp>
#include <memoria/v1/api/multimap/multimap_entry.hpp>
#include <memoria/v1/api/multimap/multimap_producer.hpp>

#include <memoria/v1/core/iovector/io_vector.hpp>

#include <memoria/v1/core/tools/span.hpp>

#include <memoria/v1/api/datatypes/traits.hpp>
#include <memoria/v1/core/types/typehash.hpp>

#include <memoria/v1/api/multimap/multimap_api_factory.hpp>

#include <memory>
#include <tuple>

namespace memoria {
namespace v1 {

template <typename Key, typename Value, typename Profile>
struct MultimapIterator: BTFLIterator<Profile> {
    using KeyV   = typename DataTypeTraits<Key>::ValueType;
    using ValueV = typename DataTypeTraits<Value>::ValueType;


    virtual KeyV key() const = 0;
    virtual ValueV value() const = 0;
    virtual bool is_end() const = 0;
    virtual bool next() = 0;
};


template <typename Key_, typename Value_, typename Profile>
struct ICtrApi<Multimap<Key_, Value_>, Profile>: public CtrReferenceable<Profile> {
public:
    using Key = Key_;
    using Value = Value_;


    using KeyView   = typename DataTypeTraits<Key>::ViewType;
    using ValueView = typename DataTypeTraits<Value>::ViewType;

    using ApiTypes  = ICtrApiTypes<Multimap<Key, Value>, Profile>;

    using Producer      = MultimapProducer<ApiTypes>;
    using ProducerFn    = typename Producer::ProducerFn;

    using IteratorAPIPtr = CtrSharedPtr<MultimapIterator<Key_, Value_, Profile>>;

public:

    
    static constexpr int32_t DataStreams = 2;
    using CtrSizeT  = ProfileCtrSizeT<Profile>;
    using CtrSizesT = ProfileCtrSizesT<Profile, DataStreams + 1>;
    
    virtual bool contains(const KeyView& key) const = 0;
    virtual bool remove(const KeyView& key) = 0;

    virtual bool remove_all(const KeyView& from, const KeyView& to) = 0; //[from, to)
    virtual bool remove_from(const KeyView& from) = 0; //[from, end)
    virtual bool remove_before(const KeyView& to) = 0; //[begin, to)

    virtual CtrSizeT size() const = 0;

    virtual CtrSharedPtr<IEntriesScanner<ApiTypes, Profile>> entries_scanner(IteratorAPIPtr iterator) const = 0;
    virtual CtrSharedPtr<IValuesScanner<ApiTypes, Profile>>  values_scanner(IteratorAPIPtr iterator) const = 0;

    virtual IteratorAPIPtr seek(CtrSizeT pos) const = 0;
    virtual IteratorAPIPtr find(KeyView key) const = 0;
    virtual IteratorAPIPtr iterator() const = 0;

    virtual CtrSharedPtr<IKeysScanner<ApiTypes, Profile>> keys() const = 0;

    bool upsert(KeyView key, ProducerFn producer_fn) {
        Producer producer(producer_fn);
        return upsert(key, producer);
    }


    bool upsert(KeyView key, Span<const ValueView> data)
    {
        return upsert(key, [&](auto& seq, auto& keys, auto& values, auto& sizes){
            seq.append(1, data.size());
            values.append_buffer(data);
            return true;
        });
    }

    // returns true if the entry was updated, and false if new entry was inserted
    virtual bool upsert(KeyView key, io::IOVectorProducer& producer) = 0;

    void append_entries(ProducerFn producer_fn) {
        Producer producer(producer_fn);
        append_entries(producer);
    }

    void append_entry(KeyView key, Span<const ValueView> data)
    {
        append_entries([&](auto& seq, auto& keys, auto& values, auto& sizes){
            seq.append(0, 1);
            keys.append(key);

            seq.append(1, data.size());
            values.append_buffer(data);

            return true;
        });
    }

    virtual void append_entries(io::IOVectorProducer& producer) = 0;


    void prepend_entries(ProducerFn producer_fn) {
        Producer producer(producer_fn);
        prepend_entries(producer);
    }

    void prepend_entry(KeyView key, Span<const ValueView> data)
    {
        prepend_entries([&](auto& seq, auto& keys, auto& values, auto& sizes){
            seq.append(0, 1);
            keys.append(key);

            seq.append(1, data.size());
            values.append_buffer(data);

            return true;
        });
    }


    virtual void prepend_entries(io::IOVectorProducer& producer) = 0;


    void insert_entries(KeyView before, ProducerFn producer_fn) {
        Producer producer(producer_fn);
        insert_entries(before, producer);
    }

    void insert_entry(KeyView before, KeyView key, Span<const ValueView> data)
    {
        insert_entries(before, [&](auto& seq, auto& keys, auto& values, auto& sizes){
            seq.append(0, 1);
            keys.append(key);

            seq.append(1, data.size());
            values.append_buffer(data);

            return true;
        });
    }

    virtual void insert_entries(KeyView before, io::IOVectorProducer& producer) = 0;

    MMA1_DECLARE_ICTRAPI();
};


    
}}
