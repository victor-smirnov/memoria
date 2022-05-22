
// Copyright 2022 Victor Smirnov
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

#include <memoria/api/collection/collection_api_factory.hpp>

#include <memoria/core/datatypes/buffer/buffer.hpp>

#include <memory>

namespace memoria {

template <typename Key, typename Profile>
struct CollectionChunk {

    using KeyView = DTTViewType<Key>;
    using CtrSizeT = ApiProfileCtrSizeT<Profile>;
    using ChunkPtr = IterSharedPtr<CollectionChunk>;

    virtual ~CollectionChunk() noexcept = default;

    virtual const KeyView& current_key() const = 0;

    virtual CtrSizeT entry_offset() const = 0;
    virtual CtrSizeT collection_size() const = 0;

    virtual CtrSizeT chunk_offset() const = 0;

    virtual size_t chunk_size() const = 0;
    virtual size_t entry_offset_in_chunk() const = 0;

    virtual const Span<const KeyView>& keys() const = 0;

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
        auto span = keys();

        for (size_t c = entry_offset_in_chunk(); c < chunk_size(); c++) {
            fn(span[c]);
        }

        if (!is_after_end()) {
            ChunkPtr next;
            do {
                next = next_chunk();

                auto span = next->keys();
                for (size_t c = next->entry_offset_in_chunk(); c < next->chunk_size(); c++) {
                    fn(span[c]);
                }
            }
            while (is_after_end(next));
        }
    }
};


template <typename Key, typename Profile>
bool is_after_end(const IterSharedPtr<CollectionChunk<Key, Profile>>& ptr) {
    return !ptr || ptr->is_after_end();
}

template <typename Key, typename Profile>
bool is_before_start(const IterSharedPtr<CollectionChunk<Key, Profile>>& ptr) {
    return !ptr || ptr->is_before_start();
}



template <typename Key, typename Profile> 
struct ICtrApi<Collection<Key>, Profile>: public CtrReferenceable<Profile> {

    using Base = CtrReferenceable<Profile>;

    using KeyView   = typename DataTypeTraits<Key>::ViewType;
    using ApiTypes  = ICtrApiTypes<Collection<Key>, Profile>;

    using ChunkSharedPtr = IterSharedPtr<CollectionChunk<Key, Profile>>;

    using BufferT       = DataTypeBuffer<Key>;
    using DataTypeT     = Key;
    using CtrSizeT      = ApiProfileCtrSizeT<Profile>;

    virtual ChunkSharedPtr first_entry() const {
        return seek_entry(0);
    }

    virtual ChunkSharedPtr last_entry() const {
        CtrSizeT size = this->size();
        if (size) {
            return seek_entry(size - 1);
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Container is empty").do_throw();
        }
    }

    virtual ChunkSharedPtr seek_entry(CtrSizeT num) const = 0;
    virtual CtrSizeT size() const = 0;

    MMA_DECLARE_ICTRAPI();
};

}
