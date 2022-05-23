
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
#include <memoria/api/collection/collection_api.hpp>

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
struct MapChunk {

    virtual ~MapChunk() noexcept = default;

    using ChunkPtr  = IterSharedPtr<MapChunk>;
    using KeyView   = DTTViewType<Key>;
    using ValueView = DTTViewType<Value>;
    using CtrSizeT  = ApiProfileCtrSizeT<Profile>;

    virtual const KeyView& current_key() const = 0;
    virtual const ValueView& current_value() const = 0;

    virtual CtrSizeT entry_offset() const = 0;
    virtual CtrSizeT collection_size() const = 0;

    virtual CtrSizeT chunk_offset() const = 0;

    virtual size_t chunk_size() const = 0;
    virtual size_t entry_offset_in_chunk() const = 0;

    virtual const Span<const KeyView>& keys() const = 0;
    virtual const Span<const ValueView>& values() const = 0;

    virtual bool is_before_start() const = 0;
    virtual bool is_after_end() const = 0;

    virtual ChunkPtr next(CtrSizeT num = 1) const = 0;
    virtual ChunkPtr next_chunk() const = 0;

    virtual ChunkPtr prev(CtrSizeT num = 1) const = 0;
    virtual ChunkPtr prev_chunk() const = 0;

    virtual ChunkPtr read_to(DataTypeBuffer<Key>& buffer, CtrSizeT num) const = 0;

    virtual void dump(std::ostream& out = std::cout) const = 0;

    virtual bool is_found(const KeyView& key) const = 0;


    template <typename Fn>
    void for_each_remaining(Fn&& fn)
    {
        auto keys_span   = this->keys();
        auto values_span = values();

        for (size_t c = this->entry_offset_in_chunk(); c < this->chunk_size(); c++) {
            fn(keys_span[c], values_span[c]);
        }

        if (!this->is_after_end()) {
            ChunkPtr next;
            do {
                next = this->next_chunk();

                auto keys_span = next->keys();
                auto values_span = next->values();

                for (size_t c = next->entry_offset_in_chunk(); c < next->chunk_size(); c++) {
                    fn(keys_span[c], values_span[c]);
                }
            }
            while (is_after_end(next));
        }
    }
};


template <typename Key, typename Value, typename Profile>
bool is_after_end(const IterSharedPtr<MapChunk<Key, Value, Profile>>& ptr) {
    return !ptr || ptr->is_after_end();
}

template <typename Key, typename Value, typename Profile>
bool is_before_start(const IterSharedPtr<MapChunk<Key, Value, Profile>>& ptr) {
    return !ptr || ptr->is_before_start();
}



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

    using CtrSizeT = ApiProfileCtrSizeT<Profile>;

    using ChunkIteratorPtr = IterSharedPtr<MapChunk<Key, Value, Profile>>;

    virtual void remove(CtrSizeT from, CtrSizeT to) = 0;
    virtual void remove_from(CtrSizeT from) = 0;
    virtual void remove_up_to(CtrSizeT pos) = 0;

    virtual ApiProfileCtrSizeT<Profile> size() const = 0;
    virtual bool upsert_key(KeyView key, ValueView value) = 0;
    virtual bool remove_key(KeyView key) = 0;

    virtual ChunkIteratorPtr find(KeyView key) const = 0;

    virtual ChunkIteratorPtr append(ProducerFn producer_fn) {
        Producer producer(producer_fn);
        return append(producer);
    }

    virtual ChunkIteratorPtr append(io::IOVectorProducer& producer) = 0;

    virtual ChunkIteratorPtr prepend(ProducerFn producer_fn) {
        Producer producer(producer_fn);
        return prepend(producer);
    }

    virtual ChunkIteratorPtr prepend(io::IOVectorProducer& producer) = 0;

    virtual ChunkIteratorPtr insert(KeyView before, ProducerFn producer_fn) {
        Producer producer(producer_fn);
        return insert(before, producer);
    }

    virtual ChunkIteratorPtr insert(KeyView before, io::IOVectorProducer& producer) = 0;

    virtual ChunkIteratorPtr first_entry() const {
        return seek_entry(0);
    }

    virtual ChunkIteratorPtr seek_entry(CtrSizeT num) const = 0;

    virtual ChunkIteratorPtr last_enry() const
    {
        CtrSizeT size = this->size();
        if (size) {
            return seek_entry(size - 1);
        }
        else {
            return seek_entry(0);
        }
    }

    virtual Optional<Datum<Value>> remove_and_return(KeyView key) = 0;
    virtual Optional<Datum<Value>> replace_and_return(KeyView key, ValueView value) = 0;

    virtual void with_value(
            KeyView key,
            std::function<Optional<Datum<Value>> (Optional<Datum<Value>>)> value_fn
    ) = 0;

    template <typename Fn>
    void for_each(Fn&& fn)
    {
        auto ss = this->first_entry();
        while (!is_after_end(ss))
        {
            auto key_ii_b = ss->keys().begin();
            auto key_ii_e = ss->keys().end();

            auto value_ii_b = ss->values().begin();
            auto value_ii_e = ss->values().end();

            while (key_ii_b != key_ii_e && value_ii_b != value_ii_e)
            {
                fn(*key_ii_b, *value_ii_b);

                ++key_ii_b;
                ++value_ii_b;
            }

            ss = ss->next_chunk();
        }
    }

    template <typename Fn>
    void for_each_chunk(Fn&& fn)
    {
        auto ss = this->first_entry();
        while (!is_after_end(ss))
        {
            fn(ss->keys(), ss->values());
            ss = ss->next_chunk();
        }
    }

    MMA_DECLARE_ICTRAPI();
};

}
