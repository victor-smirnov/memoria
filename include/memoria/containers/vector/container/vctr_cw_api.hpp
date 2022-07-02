
// Copyright 2013-2022 Victor Smirnov
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


#include <memoria/containers/vector/vctr_names.hpp>

#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/api/vector/vector_api.hpp>

#include <memoria/prototypes/bt/tools/bt_tools.hpp>

namespace memoria {

MEMORIA_V1_CONTAINER_PART_BEGIN(mvector::CtrApiWName)

public:
    using Types = typename Base::Types;


    using typename Base::TreeNodePtr;
    using typename Base::Position;
    using typename Base::BranchNodeEntry;
    using typename Base::CtrSizeT;
    using typename Base::Profile;
    using typename Base::ApiProfileT;

protected:
    using Value = typename Types::Value;
    using ValueDataType = typename Types::ValueDataType;
    using ViewType  = DTTViewType<ValueDataType>;
    using CtrApiTypes = ICtrApiTypes<typename Types::ContainerTypeName, Profile>;

    using CtrInputBuffer = typename Types::CtrInputBuffer;

    using ValuesPath = IntList<0, 1>;

public:

    using CollectionChunkT = CollectionChunk<Value, ApiProfile<Profile>>;
    using ChunkSharedPtr = IterSharedPtr<CollectionChunkT>;


    using typename Base::CollectionChunkTypes;
    using typename Base::CollectionChunkImplT;

    ChunkSharedPtr insert(CtrSizeT at, CtrInputBuffer& buffer)
    {
        auto& self = this->self();

        auto iter = self.ctr_seek_entry(at);

        auto jj = self.ctr_insert_batch(std::move(iter), buffer);
        return memoria_static_pointer_cast<CollectionChunkImplT>(jj);
    }

    ChunkSharedPtr insert(CtrSizeT at, CtrBatchInputFn<CtrInputBuffer> producer)
    {
        auto& self = this->self();

        auto iter = self.ctr_seek_entry(at);

        auto jj = self.ctr_insert_batch(std::move(iter), producer);
        return memoria_static_pointer_cast<CollectionChunkImplT>(jj);
    }


    template <typename T, size_t SubstreamIdx = 0>
    class ValueBuffer {
        const T& value_;
    public:
        ValueBuffer(const T& value): value_(value) {}

        const auto& get(bt::StreamTag<0>, bt::StreamTag<SubstreamIdx>, size_t) const {
            return value_;
        }
    };


    void set(CtrSizeT pos, ViewType view)
    {
        auto& self = this->self();
        auto iter = self.ctr_seek_entry(pos);

        self.template ctr_update_entry2<ValuesPath>(std::move(iter), ValueBuffer<ViewType>(view));
    }

    ChunkSharedPtr prepend(CtrBatchInputFn<CtrInputBuffer> producer)
    {
        auto& self = this->self();

        auto iter = self.ctr_seek_entry(0);

        auto jj = self.ctr_insert_batch(std::move(iter), producer);
        return memoria_static_pointer_cast<CollectionChunkImplT>(jj);
    }

    ChunkSharedPtr append(CtrBatchInputFn<CtrInputBuffer> producer)
    {
        auto& self = this->self();

        auto iter = self.ctr_seek_entry(self.size());

        auto jj = self.ctr_insert_batch(std::move(iter), producer);
        return memoria_static_pointer_cast<CollectionChunkImplT>(jj);
    }

MEMORIA_V1_CONTAINER_PART_END

}
