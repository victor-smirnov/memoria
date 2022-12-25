
// Copyright 2017-2022 Victor Smirnov
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
#include <memoria/api/collection/collection_api.hpp>

#include <memoria/core/datatypes/traits.hpp>
#include <memoria/core/reflection/typehash.hpp>

#include <memoria/api/map/map_api_factory.hpp>
#include <memoria/core/datatypes/buffer/buffer.hpp>

namespace memoria {

template <typename Key, typename Value, typename Profile>
struct MapChunk: ChunkIteratorBase<MapChunk<Key, Value, Profile>, Profile> {
    using Base = ChunkIteratorBase<MapChunk<Key, Value, Profile>, Profile>;

    using typename Base::ChunkPtr;
    using typename Base::CtrSizeT;

    using KeyView   = DTTViewType<Key>;
    using ValueView = DTTViewType<Value>;

    virtual DTView<Key> current_key() const = 0;
    virtual DTView<Value> current_value() const = 0;

    virtual DTSpan<Key> keys() const = 0;
    virtual DTSpan<Value> values() const = 0;

    virtual ChunkPtr read_to(DataTypeBuffer<Key>& buffer, CtrSizeT num) const = 0;

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
            while (!this->is_after_end(next));
        }
    }
};


template <typename Key, typename Value, typename Profile>
struct ICtrApi<Map<Key, Value>, Profile>: public CtrReferenceable<Profile> {

    using KeyView   = DTTViewType<Key>;
    using ValueView = DTTViewType<Value>;

    using ApiTypes  = ICtrApiTypes<Map<Key, Value>, Profile>;

    using CtrSizeT = ApiProfileCtrSizeT<Profile>;

    using ChunkIteratorPtr = IterSharedPtr<MapChunk<Key, Value, Profile>>;

    using CtrInputBuffer = typename ApiTypes::CtrInputBuffer;

    virtual void remove(CtrSizeT from, CtrSizeT to) MEMORIA_READ_ONLY_API

    virtual void remove_from(CtrSizeT from) MEMORIA_READ_ONLY_API
    virtual void remove_up_to(CtrSizeT pos) MEMORIA_READ_ONLY_API

    virtual ApiProfileCtrSizeT<Profile> size() const = 0;
    virtual bool upsert_key(const KeyView& key, ValueView value) MEMORIA_READ_ONLY_API
    virtual bool remove_key(const KeyView& key) MEMORIA_READ_ONLY_API

    virtual ChunkIteratorPtr find(const KeyView& key) const = 0;
    virtual ChunkIteratorPtr append(CtrBatchInputFn<CtrInputBuffer> producer) MEMORIA_READ_ONLY_API

    virtual ChunkIteratorPtr prepend(CtrBatchInputFn<CtrInputBuffer> producer) MEMORIA_READ_ONLY_API


    virtual ChunkIteratorPtr insert(const KeyView& before, CtrBatchInputFn<CtrInputBuffer> producer) MEMORIA_READ_ONLY_API

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

    virtual Optional<Datum<Value>> remove_and_return(const KeyView& key) MEMORIA_READ_ONLY_API
    virtual Optional<Datum<Value>> replace_and_return(const KeyView& key, const ValueView& value) MEMORIA_READ_ONLY_API

    virtual void with_value(
            KeyView key,
            std::function<Optional<Datum<Value>> (Optional<Datum<Value>>)> value_fn
    ) MEMORIA_READ_ONLY_API

    template <typename Fn>
    void for_each(Fn&& fn)
    {
        auto ss = this->first_entry();
        while (is_valid_chunk(ss))
        {
            auto keys = ss->keys();
            auto values = ss->values();

            for (size_t c = 0; c < keys.size(); c++)
            {
                fn(keys[c], values[c]);
            }

            ss = ss->next_chunk();
        }
    }

    template <typename Fn>
    void for_each_chunk(Fn&& fn)
    {
        auto ss = this->first_entry();
        while (is_valid_chunk(ss))
        {
            fn(ss->keys(), ss->values());
            ss = ss->next_chunk();
        }
    }

    MMA_DECLARE_ICTRAPI();
};

}
