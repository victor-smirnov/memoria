
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

#include <memoria/v1/api/common/ctr_api_btss.hpp>

#include <memoria/v1/api/datatypes/traits.hpp>
#include <memoria/v1/api/datatypes/encoding_traits.hpp>
#include <memoria/v1/api/datatypes/io_vector_traits.hpp>

#include <memoria/v1/core/types/typehash.hpp>

#include <memoria/v1/core/iovector/io_vector.hpp>

#include <memoria/v1/api/map/map_scanner.hpp>
#include <memoria/v1/api/map/map_producer.hpp>
#include <memoria/v1/api/map/map_api_factory.hpp>

#include <memoria/v1/core/strings/string_codec.hpp>

namespace memoria {
namespace v1 {

template <typename Key, typename Value>
struct MapIterator {
    using KeyV   = typename DataTypeTraits<Key>::ValueType;
    using ValueV = typename DataTypeTraits<Value>::ValueType;

    virtual ~MapIterator() noexcept {}

    virtual KeyV key() const = 0;
    virtual ValueV value() const = 0;
    virtual bool is_end() const = 0;
    virtual void next() = 0;
};




template <typename Key, typename Value, typename Profile>
struct ICtrApi<Map<Key, Value>, Profile>: public CtrReferenceable {

    using KeyView   = typename DataTypeTraits<Key>::ViewType;
    using ValueView = typename DataTypeTraits<Value>::ViewType;

    using ApiTypes  = ICtrApiTypes<Map<Key, Value>, Profile>;

    using Producer      = MapProducer<ApiTypes>;
    using ProducerFn    = typename Producer::ProducerFn;

    virtual int64_t map_size() const = 0;
    virtual void assign_key(KeyView key, ValueView value) = 0;
    virtual void remove_key(KeyView key) = 0;

    void append_entries(ProducerFn producer_fn) {
        Producer producer(producer_fn);
        append_entries(producer);
    }

    virtual void append_entries(io::IOVectorProducer& producer) = 0;

    void prepend_entries(ProducerFn producer_fn) {
        Producer producer(producer_fn);
        prepend_entries(producer);
    }

    virtual void prepend_entries(io::IOVectorProducer& producer) = 0;


    void insert_entries(KeyView before, ProducerFn producer_fn) {
        Producer producer(producer_fn);
        insert_entries(before, producer);
    }

    virtual void insert_entries(KeyView before, io::IOVectorProducer& producer) = 0;

    virtual CtrSharedPtr<MapIterator<Key, Value>> iterator()    = 0;
    virtual CtrSharedPtr<BTSSIterator<Profile>> raw_iterator()  = 0;

    MapScanner<ApiTypes, Profile> scanner() {
        return MapScanner<ApiTypes, Profile>(raw_iterator());
    }

    MMA1_DECLARE_ICTRAPI();
};





    
}}
