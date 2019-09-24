
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

#include <memoria/v1/api/set/set_api_factory.hpp>
#include <memoria/v1/api/set/set_scanner.hpp>
#include <memoria/v1/api/set/set_producer.hpp>

#include <memory>

namespace memoria {
namespace v1 {

template <typename Key, typename Profile>
struct SetIterator: BTSSIterator<Profile> {
    using KeyV   = typename DataTypeTraits<Key>::ValueType;

    virtual ~SetIterator() noexcept {}

    virtual KeyV key() const = 0;
    virtual bool is_end() const = 0;
    virtual bool next() = 0;
};
    
template <typename Key, typename Profile> 
struct ICtrApi<Set<Key>, Profile>: public CtrReferenceable<Profile> {


    using KeyView   = typename DataTypeTraits<Key>::ViewType;
    using ApiTypes  = ICtrApiTypes<Set<Key>, Profile>;

    using Producer      = SetProducer<ApiTypes>;
    using ProducerFn    = typename Producer::ProducerFn;

    virtual ProfileCtrSizeT<Profile> size() const = 0;

    virtual CtrSharedPtr<SetIterator<Key, Profile>> find(KeyView key) const = 0;

    virtual bool contains(KeyView key)   = 0;
    virtual bool remove(KeyView key)     = 0;
    virtual bool insert(KeyView key)     = 0;


    void append(ProducerFn producer_fn) {
        Producer producer(producer_fn);
        append(producer);
    }

    virtual void append(io::IOVectorProducer& producer) = 0;

    void prepend(ProducerFn producer_fn) {
        Producer producer(producer_fn);
        prepend(producer);
    }

    virtual void prepend(io::IOVectorProducer& producer) = 0;


    void insert(KeyView before, ProducerFn producer_fn) {
        Producer producer(producer_fn);
        insert(before, producer);
    }

    virtual void insert(KeyView before, io::IOVectorProducer& producer) = 0;

    virtual CtrSharedPtr<SetIterator<Key, Profile>> iterator() const = 0;

    template <typename Fn>
    SetScanner<ApiTypes, Profile> scanner(Fn&& iterator_producer) const {
        return SetScanner<ApiTypes, Profile>(iterator_producer(this));
    }

    SetScanner<ApiTypes, Profile> scanner() const {
        return SetScanner<ApiTypes, Profile>(iterator());
    }

    MMA1_DECLARE_ICTRAPI();
};




}}
