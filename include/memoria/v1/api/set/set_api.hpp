
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

    virtual ~SetIterator() noexcept {}

    virtual Datum<Key> key() const noexcept = 0;
    virtual bool is_end() const noexcept = 0;
    virtual BoolResult next() noexcept = 0;

};
    
template <typename Key, typename Profile> 
struct ICtrApi<Set<Key>, Profile>: public CtrReferenceable<Profile> {

    using KeyView   = typename DataTypeTraits<Key>::ViewType;
    using ApiTypes  = ICtrApiTypes<Set<Key>, Profile>;

    using Producer      = SetProducer<ApiTypes>;
    using ProducerFn    = typename Producer::ProducerFn;

    virtual Result<ProfileCtrSizeT<Profile>> size() const noexcept = 0;

    virtual Result<CtrSharedPtr<SetIterator<Key, Profile>>> find(KeyView key) const noexcept = 0;

    virtual BoolResult contains(KeyView key) noexcept  = 0;
    virtual BoolResult remove(KeyView key) noexcept    = 0;
    virtual BoolResult insert(KeyView key) noexcept    = 0;


    VoidResult append(ProducerFn producer_fn) noexcept {
        Producer producer(producer_fn);
        return append(producer);
    }

    virtual VoidResult append(io::IOVectorProducer& producer) noexcept = 0;

    VoidResult prepend(ProducerFn producer_fn) noexcept {
        Producer producer(producer_fn);
        return prepend(producer);
    }

    virtual VoidResult prepend(io::IOVectorProducer& producer) noexcept = 0;


    VoidResult insert(KeyView before, ProducerFn producer_fn) noexcept {
        Producer producer(producer_fn);
        return insert(before, producer);
    }

    virtual VoidResult insert(KeyView before, io::IOVectorProducer& producer) noexcept = 0;

    virtual Result<CtrSharedPtr<SetIterator<Key, Profile>>> iterator() const noexcept = 0;

    template <typename Fn>
    SetScanner<ApiTypes, Profile> scanner(Fn&& iterator_producer) const {
        return SetScanner<ApiTypes, Profile>(iterator_producer(this));
    }

    SetScanner<ApiTypes, Profile> scanner() const {
        return SetScanner<ApiTypes, Profile>(iterator().get_or_terminate());
    }

    MMA1_DECLARE_ICTRAPI();
};




}}
