
// Copyright 2017-2025 Victor Smirnov
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

#include <memoria/api/set/set_api_factory.hpp>

#include <memory>

namespace memoria {

template <typename Key, typename Profile> 
struct ICtrApi<Set<Key>, Profile>: public ICtrApi<Collection<Key>, Profile> {
    using KeyView   = DTTViewType<Key>;
    using ApiTypes  = ICtrApiTypes<Set<Key>, Profile>;

    using DataTypeT     = Key;
    using CtrSizeT      = ApiProfileCtrSizeT<Profile>;

    using CtrInputBuffer = typename ApiTypes::CtrInputBuffer;

    using ChunkIteratorPtr = IterSharedPtr<CollectionChunk<Key, Profile>>;


    virtual void read_to(CtrInputBuffer& buffer, CtrSizeT start, CtrSizeT length) const = 0;
    virtual ChunkIteratorPtr insert(CtrSizeT at, const CtrInputBuffer& buffer) MEMORIA_READ_ONLY_API

    virtual ApiProfileCtrSizeT<Profile> size() const = 0;

    virtual ChunkIteratorPtr find(const KeyView& key) const = 0;

    virtual bool contains(const KeyView& key)  = 0;

    virtual bool remove(const KeyView& key) MEMORIA_READ_ONLY_API

    virtual bool upsert(const KeyView& key) MEMORIA_READ_ONLY_API

    virtual ChunkIteratorPtr append(CtrBatchInputFn<CtrInputBuffer> producer) MEMORIA_READ_ONLY_API

    virtual ChunkIteratorPtr prepend(CtrBatchInputFn<CtrInputBuffer> producer) MEMORIA_READ_ONLY_API

    virtual ChunkIteratorPtr insert(const KeyView& before, CtrBatchInputFn<CtrInputBuffer> producer) MEMORIA_READ_ONLY_API

    template <typename Fn>
    void for_each(Fn&& fn)
    {
        auto ss = this->first_entry();
        while (is_valid_chunk(ss))
        {
            auto keys = ss->keys();
            for (size_t c = 0; c < keys.size(); c++) {
                fn(keys[c]);
            }

            ss = ss->next_chunk();
        }
    }

    MMA_DECLARE_ICTRAPI();
};

}
