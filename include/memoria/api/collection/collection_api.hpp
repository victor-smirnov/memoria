
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
struct CollectionChunk: ChunkIteratorBase<CollectionChunk<Key, Profile>, Profile> {

    using Base = ChunkIteratorBase<CollectionChunk<Key, Profile>, Profile>;


    using typename Base::CtrSizeT;
    using typename Base::ChunkPtr;

    using KeyView = DTTViewType<Key>;

    virtual const KeyView& current_key() const = 0;
    virtual const Span<const KeyView>& keys() const = 0;


    virtual ChunkPtr read_to(DataTypeBuffer<Key>& buffer, CtrSizeT num) const = 0;


    virtual bool is_found(const KeyView& key) const = 0;

    template <typename Fn>
    void for_each_remaining(Fn&& fn)
    {
        auto span = keys();

        for (size_t c = this->entry_offset_in_chunk(); c < this->chunk_size(); c++) {
            fn(span[c]);
        }

        if (!this->is_after_end()) {
            ChunkPtr next;
            do {
                next = this->next_chunk();

                auto span = next->keys();
                for (size_t c = next->entry_offset_in_chunk(); c < next->chunk_size(); c++) {
                    fn(span[c]);
                }
            }
            while (!this->is_after_end(next));
        }
    }
};


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

    virtual void remove(CtrSizeT from, CtrSizeT to) MEMORIA_READ_ONLY_API
    virtual void remove_from(CtrSizeT from) MEMORIA_READ_ONLY_API
    virtual void remove_up_to(CtrSizeT pos) MEMORIA_READ_ONLY_API

    MMA_DECLARE_ICTRAPI();
};

}
