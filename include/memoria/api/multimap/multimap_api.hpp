
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

#include <memoria/api/common/ctr_api_btfl.hpp>

#include <memoria/api/multimap/multimap_output.hpp>
#include <memoria/api/multimap/multimap_input.hpp>
#include <memoria/api/multimap/multimap_producer.hpp>

#include <memoria/core/iovector/io_vector.hpp>

#include <memoria/core/tools/span.hpp>

#include <memoria/core/datatypes/traits.hpp>
#include <memoria/core/types/typehash.hpp>

#include <memoria/api/multimap/multimap_api_factory.hpp>

#include <memory>
#include <tuple>

namespace memoria {

template <typename Key, typename Value, typename Profile>
struct MultimapIterator: BTFLIterator<Profile> {
    virtual Datum<Key> key() const = 0;
    virtual Datum<Value> value() const = 0;
    virtual bool is_end() const noexcept = 0;
    virtual BoolResult next() noexcept = 0;
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
    using CtrSizeT  = ApiProfileCtrSizeT<Profile>;
    using CtrSizesT = ApiProfileCtrSizesT<Profile, DataStreams + 1>;
    
    virtual BoolResult contains(const KeyView& key) const noexcept = 0;
    virtual BoolResult remove(const KeyView& key) noexcept = 0;

    virtual BoolResult remove_all(const KeyView& from, const KeyView& to) noexcept = 0; //[from, to)
    virtual BoolResult remove_from(const KeyView& from) noexcept = 0; //[from, end)
    virtual BoolResult remove_before(const KeyView& to) noexcept = 0; //[begin, to)

    virtual Result<CtrSizeT> size() const noexcept = 0;

    virtual CtrSharedPtr<IEntriesScanner<ApiTypes, Profile>> entries_scanner(IteratorAPIPtr iterator) const = 0;
    virtual CtrSharedPtr<IValuesScanner<ApiTypes, Profile>>  values_scanner(IteratorAPIPtr iterator) const = 0;

    virtual Result<CtrSharedPtr<IEntriesScanner<ApiTypes, Profile>>> entries_scanner() const noexcept
    {
        using ResultT = Result<CtrSharedPtr<IEntriesScanner<ApiTypes, Profile>>>;

        MEMORIA_TRY(iter, this->iterator());
        return ResultT::of(entries_scanner(iter));
    }

    virtual Result<IteratorAPIPtr> seek(CtrSizeT pos) const noexcept = 0;
    virtual Result<IteratorAPIPtr> find(KeyView key) const noexcept = 0;
    virtual Result<IteratorAPIPtr> iterator() const noexcept = 0;

    virtual Result<CtrSharedPtr<IKeysScanner<ApiTypes, Profile>>> keys() const noexcept = 0;

    BoolResult upsert(KeyView key, ProducerFn producer_fn) noexcept {
        Producer producer(producer_fn);
        return upsert(key, producer);
    }


    BoolResult upsert(KeyView key, Span<const ValueView> data) noexcept
    {
        return upsert(key, [&](auto& seq, auto& keys, auto& values, auto& sizes) {
            seq.append(0, 1);
            keys.append(key);

            if (data.size() > 0) {
                seq.append(1, data.size());
                values.append(data);
            }

            return true;
        });
    }

    // returns true if the entry was updated, and false if new entry was inserted
    virtual BoolResult upsert(KeyView key, io::IOVectorProducer& producer) noexcept = 0;

    VoidResult append_entries(ProducerFn producer_fn) noexcept {
        Producer producer(producer_fn);
        return append_entries(producer);
    }

    VoidResult append_entry(KeyView key, Span<const ValueView> data) noexcept
    {
        return append_entries([&](auto& seq, auto& keys, auto& values, auto& sizes){
            seq.append(0, 1);
            keys.append(key);

            seq.append(1, data.size());
            values.append(data);

            return true;
        });
    }

    virtual VoidResult append_entries(io::IOVectorProducer& producer) noexcept = 0;


    VoidResult prepend_entries(ProducerFn producer_fn) noexcept {
        Producer producer(producer_fn);
        return prepend_entries(producer);
    }

    VoidResult prepend_entry(KeyView key, Span<const ValueView> data) noexcept
    {
        return prepend_entries([&](auto& seq, auto& keys, auto& values, auto& sizes){
            seq.append(0, 1);
            keys.append(key);

            seq.append(1, data.size());
            values.append(data);

            return true;
        });
    }


    virtual VoidResult prepend_entries(io::IOVectorProducer& producer) noexcept = 0;


    VoidResult insert_entries(KeyView before, ProducerFn producer_fn) noexcept {
        Producer producer(producer_fn);
        return insert_entries(before, producer);
    }

    VoidResult insert_entry(KeyView before, KeyView key, Span<const ValueView> data) noexcept
    {
        return insert_entries(before, [&](auto& seq, auto& keys, auto& values, auto& sizes){
            seq.append(0, 1);
            keys.append(key);

            seq.append(1, data.size());
            values.append(data);

            return true;
        });
    }

    virtual VoidResult insert_entries(KeyView before, io::IOVectorProducer& producer) noexcept = 0;

    MMA_DECLARE_ICTRAPI();
};

}
