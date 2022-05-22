
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

#include <memoria/api/set/set_api_factory.hpp>
#include <memoria/api/set/set_producer.hpp>

#include <memoria/core/datatypes/buffer/buffer.hpp>

#include <memory>

namespace memoria {

template <typename Key, typename Profile> 
struct ICtrApi<Set<Key>, Profile>: public ICtrApi<Collection<Key>, Profile> {

    using KeyView   = typename DataTypeTraits<Key>::ViewType;
    using ApiTypes  = ICtrApiTypes<Set<Key>, Profile>;

    using Producer      = SetProducer<ApiTypes>;
    using ProducerFn    = typename Producer::ProducerFn;

    using BufferT       = DataTypeBuffer<Key>;
    using DataTypeT     = Key;
    using CtrSizeT      = ApiProfileCtrSizeT<Profile>;

    using ChunkIteratorPtr = IterSharedPtr<CollectionChunk<Key, Profile>>;


    virtual void read_to(BufferT& buffer, CtrSizeT start, CtrSizeT length) const = 0;
    virtual ChunkIteratorPtr insert(CtrSizeT at, const BufferT& buffer, size_t start, size_t length) = 0;

    virtual ChunkIteratorPtr insert(CtrSizeT at, const BufferT& buffer) {
        return insert(at, buffer, 0, buffer.size());
    }

    virtual void remove(CtrSizeT from, CtrSizeT to) = 0;
    virtual void remove_from(CtrSizeT from) = 0;
    virtual void remove_up_to(CtrSizeT pos) = 0;

    virtual ApiProfileCtrSizeT<Profile> size() const = 0;

    virtual ChunkIteratorPtr find(KeyView key) const = 0;

    virtual bool contains(KeyView key)  = 0;
    virtual bool remove(KeyView key)    = 0;
    virtual bool upsert(KeyView key)    = 0;


    ChunkIteratorPtr append(ProducerFn producer_fn)  {
        Producer producer(producer_fn);
        return append(producer);
    }

    virtual ChunkIteratorPtr append(io::IOVectorProducer& producer) = 0;

    ChunkIteratorPtr prepend(ProducerFn producer_fn) {
        Producer producer(producer_fn);
        return prepend(producer);
    }

    virtual ChunkIteratorPtr prepend(io::IOVectorProducer& producer) = 0;


    ChunkIteratorPtr insert(KeyView before, ProducerFn producer_fn) {
        Producer producer(producer_fn);
        return insert(before, producer);
    }

    virtual ChunkIteratorPtr insert(KeyView before, io::IOVectorProducer& producer) = 0;

    template <typename Fn>
    void for_each(Fn&& fn)
    {
        auto ss = this->first_entry();
        while (!is_after_end(ss))
        {
            for (auto key_view: ss->keys()) {
                fn(key_view);
            }

            ss = ss->next_chunk();
        }
    }

    MMA_DECLARE_ICTRAPI();
};

}
