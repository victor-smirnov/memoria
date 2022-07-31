
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

#include <memoria/api/common/ctr_api_btfl.hpp>
#include <memoria/core/tools/span.hpp>
#include <memoria/api/multimap/multimap_api_factory.hpp>

#include <memory>
#include <tuple>

namespace memoria {

template <typename Key, typename Value, typename Profile>
struct MultimapValuesChunk;

template <typename Key, typename Value, typename Profile>
struct MultimapKeysChunk: ChunkIteratorBase<MultimapKeysChunk<Key, Value, Profile>, Profile> {

    using Base = ChunkIteratorBase<MultimapKeysChunk<Key, Value, Profile>, Profile>;

    using typename Base::CtrSizeT;
    using typename Base::ChunkPtr;

    using KeyView = DTTViewType<Key>;
    using ValuesChunkPtr = IterSharedPtr<MultimapValuesChunk<Key, Value, Profile>>;

    virtual DTTConstPtr<Key> current_key() const = 0;
    virtual DTTConstSpan<Key> keys() const = 0;

    //virtual ChunkPtr read_to(DataTypeBuffer<Key>& buffer, CtrSizeT num) const = 0;

    virtual bool is_found(const KeyView& key) const = 0;

    virtual ValuesChunkPtr values_chunk() const = 0;
    virtual ValuesChunkPtr values_chunk(size_t idx) const = 0;
};


template <typename Key, typename Value, typename Profile>
struct MultimapValuesChunk: ChunkIteratorBase<MultimapValuesChunk<Key, Value, Profile>, Profile> {

    using Base = ChunkIteratorBase<MultimapValuesChunk<Key, Value, Profile>, Profile>;

    using typename Base::CtrSizeT;
    using typename Base::ChunkPtr;

    using ValueView = DTTViewType<Value>;
    using KeysChunkPtr = IterSharedPtr<MultimapKeysChunk<Key, Value, Profile>>;

    virtual ~MultimapValuesChunk() noexcept = default;

    virtual DTTConstPtr<Value> current_value() const = 0;

    virtual DTTConstSpan<Value> values() const = 0;

    //virtual ChunkPtr read_to(DataTypeBuffer<Value>& buffer, CtrSizeT num) const = 0;

    virtual bool is_found(const ValueView& key) const = 0;

    virtual KeysChunkPtr my_key() const = 0;

    virtual CtrSizeT size() const = 0;
};





template <typename Key_, typename Value_, typename Profile>
struct ICtrApi<Multimap<Key_, Value_>, Profile>: public CtrReferenceable<Profile> {
public:
    using Key = Key_;
    using Value = Value_;

    using KeyView   = typename DataTypeTraits<Key>::ViewType;
    using ValueView = typename DataTypeTraits<Value>::ViewType;

    using ApiTypes  = ICtrApiTypes<Multimap<Key, Value>, Profile>;

    using KeysChunkT = MultimapKeysChunk<Key, Value, Profile>;
    using KeysChunkPtrT = IterSharedPtr<KeysChunkT>;

    using CtrInputBuffer = typename ApiTypes::CtrInputBuffer;

public:

    
    static constexpr int32_t DataStreams = 2;
    using CtrSizeT  = ApiProfileCtrSizeT<Profile>;
    using CtrSizesT = ApiProfileCtrSizesT<Profile, DataStreams + 1>;
    

    virtual KeysChunkPtrT find_key(KeyView key) const = 0;
    virtual KeysChunkPtrT seek_key(CtrSizeT pos) const = 0;

    virtual bool contains(const KeyView& key) const = 0;
    virtual bool remove(const KeyView& key) MEMORIA_READ_ONLY_API

    virtual bool remove_all(const KeyView& from, const KeyView& to) MEMORIA_READ_ONLY_API //[from, to)
    virtual bool remove_from(const KeyView& from) MEMORIA_READ_ONLY_API //[from, end)
    virtual bool remove_before(const KeyView& up_to) MEMORIA_READ_ONLY_API //[begin, up_to)

    virtual CtrSizeT size() const = 0;

    bool upsert(KeyView key, Span<const ValueView> data)
    {
        return upsert(key, [&](CtrInputBuffer& buff) {
            buff.symbols().append_run(0, 1);
            buff.keys().append(key);

            if (data.size() > 0) {
                buff.symbols().append_run(1, data.size());
                buff.values().append(data);
            }

            return true;
        });
    }

    // returns true if the entry was updated, and false if new entry was inserted
    virtual bool upsert(KeyView key, CtrBatchInputFn<CtrInputBuffer> producer) MEMORIA_READ_ONLY_API

    void append_entry(KeyView key, Span<const ValueView> data)
    {
        return append_entries([&](auto& buff){
            buff.symbols().append_run(0, 1);
            buff.keys().append(key);

            buff.symbols().append_run(1, data.size());
            buff.values().append(data);

            return true;
        });
    }

    virtual void append_entries(CtrBatchInputFn<CtrInputBuffer> producer) MEMORIA_READ_ONLY_API

    void prepend_entry(KeyView key, Span<const ValueView> data)
    {
        return prepend_entries([&](auto& buff){
            buff.symbols().append_run(0, 1);
            buff.keys().append(key);

            buff.symbols().append_run(1, data.size());
            buff.values().append(data);

            return true;
        });
    }


    virtual void prepend_entries(CtrBatchInputFn<CtrInputBuffer> producer) MEMORIA_READ_ONLY_API

    void insert_entry(KeyView before, KeyView key, Span<const ValueView> data)
    {
        return insert_entries(before, [&](auto& buff){
            buff.symbols().append_run(0, 1);
            buff.keys().append(key);

            buff.symbols().append_run(1, data.size());
            buff.values().append(data);

            return true;
        });
    }

    virtual void insert_entries(KeyView before, CtrBatchInputFn<CtrInputBuffer> producer) MEMORIA_READ_ONLY_API

    MMA_DECLARE_ICTRAPI();
};

}
