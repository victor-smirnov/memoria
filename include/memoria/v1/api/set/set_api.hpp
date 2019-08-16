
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

template <typename Key>
struct SetIterator {
    using KeyV   = typename DataTypeTraits<Key>::ValueType;

    virtual ~SetIterator() noexcept {}

    virtual KeyV key() const = 0;
    virtual bool is_end() const = 0;
    virtual void next() = 0;
};
    
template <typename Key, typename Profile> 
struct ICtrApi<Set<Key>, Profile>: public CtrReferenceable<Profile> {


    using KeyView   = typename DataTypeTraits<Key>::ViewType;
    using ApiTypes  = ICtrApiTypes<Set<Key>, Profile>;

    using Producer      = SetProducer<ApiTypes>;
    using ProducerFn    = typename Producer::ProducerFn;

    virtual ProfileCtrSizeT<Profile> size() const = 0;

    virtual CtrSharedPtr<BTSSIterator<Profile>> find_element_raw(KeyView key) = 0;

    SetScanner<ApiTypes, Profile> find_element(KeyView key)
    {
        return SetScanner<ApiTypes, Profile>(find_element_raw(key));
    }

    virtual bool contains_element(KeyView key)   = 0;
    virtual bool remove_element(KeyView key)     = 0;
    virtual bool insert_element(KeyView key)     = 0;


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

    virtual CtrSharedPtr<SetIterator<Key>> iterator()           = 0;
    virtual CtrSharedPtr<BTSSIterator<Profile>> raw_iterator()  = 0;

    SetScanner<ApiTypes, Profile> scanner() {
        return SetScanner<ApiTypes, Profile>(raw_iterator());
    }

    MMA1_DECLARE_ICTRAPI();
};




}}
