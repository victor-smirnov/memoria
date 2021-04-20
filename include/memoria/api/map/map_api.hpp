
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

#include <memoria/api/common/ctr_api_btss.hpp>
#include <memoria/api/common/iobuffer_adatpters.hpp>

#include <memoria/core/datatypes/traits.hpp>
#include <memoria/core/datatypes/encoding_traits.hpp>
#include <memoria/core/datatypes/io_vector_traits.hpp>

#include <memoria/core/types/typehash.hpp>

#include <memoria/core/iovector/io_vector.hpp>

#include <memoria/api/map/map_scanner.hpp>
#include <memoria/api/map/map_producer.hpp>
#include <memoria/api/map/map_api_factory.hpp>

#include <memoria/core/strings/string_codec.hpp>

#include <memoria/core/datatypes/buffer/buffer.hpp>

namespace memoria {

template <typename Key, typename Value, typename Profile>
struct MapIterator: BTSSIterator<Profile> {

    using KeyView   = typename DataTypeTraits<Key>::ViewType;

    virtual Datum<Key> key() const = 0;
    virtual Datum<Value> value() const = 0;

    virtual bool is_end() const = 0;
    virtual bool next() = 0;

    virtual bool is_found(const KeyView& key) const = 0;
};




template <typename Key, typename Value, typename Profile>
struct ICtrApi<Map<Key, Value>, Profile>: public CtrReferenceable<Profile> {

    using KeyView   = typename DataTypeTraits<Key>::ViewType;
    using ValueView = typename DataTypeTraits<Value>::ViewType;

    using ApiTypes  = ICtrApiTypes<Map<Key, Value>, Profile>;

    using Producer      = MapProducer<ApiTypes>;
    using ProducerFn    = typename Producer::ProducerFn;

    virtual ApiProfileCtrSizeT<Profile> size() const = 0;
    virtual void assign_key(KeyView key, ValueView value) = 0;
    virtual void remove_key(KeyView key) = 0;

    virtual CtrSharedPtr<MapIterator<Key, Value, Profile>> find(KeyView key) const = 0;

    void append(ProducerFn producer_fn) {
        Producer producer(producer_fn);
        return append(producer);
    }

    virtual void append(io::IOVectorProducer& producer) = 0;

    void prepend(ProducerFn producer_fn) {
        Producer producer(producer_fn);
        return prepend(producer);
    }

    virtual void prepend(io::IOVectorProducer& producer) = 0;

    void insert(KeyView before, ProducerFn producer_fn) {
        Producer producer(producer_fn);
        return insert(before, producer);
    }

    virtual void insert(KeyView before, io::IOVectorProducer& producer) = 0;

    virtual CtrSharedPtr<MapIterator<Key, Value, Profile>> iterator() const = 0;

    MapScanner<ApiTypes, Profile> scanner() const {
        return MapScanner<ApiTypes, Profile>(iterator());
    }

    template <typename Fn>
    MapScanner<ApiTypes, Profile> scanner(Fn&& iterator_producer) const {
        return MapScanner<ApiTypes, Profile>(iterator_producer(this));
    }


    virtual Optional<Datum<Value>> remove_and_return(KeyView key) = 0;
    virtual Optional<Datum<Value>> replace_and_return(KeyView key, ValueView value) = 0;

    virtual void with_value(
            KeyView key,
            std::function<Optional<Datum<Value>> (Optional<Datum<Value>>)> value_fn
    ) = 0;

    MMA_DECLARE_ICTRAPI();
};

}
