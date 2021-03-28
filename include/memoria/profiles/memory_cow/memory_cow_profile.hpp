
// Copyright 2019 Victor Smirnov
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

#include <memoria/profiles/common/common.hpp>
#include <memoria/profiles/common/block.hpp>
#include <memoria/profiles/core_cow_api/core_cow_api_profile.hpp>

#include <memoria/core/container/allocator.hpp>
#include <memoria/core/tools/uuid.hpp>

#include <unordered_set>

namespace memoria {

template <>
struct ProfileTraits<MemoryCoWProfile<>>: ApiProfileTraits<CoreCowApiProfile<>> {
    using Base = ApiProfileTraits<CoreCowApiProfile<>>;

    using typename Base::CtrID;
    using typename Base::CtrSizeT;
    using typename Base::SnapshotID;

    using BlockID       = MemCoWBlockID<uint64_t>;
    using Profile       = MemoryCoWProfile<>;

    using Block = AbstractPage<BlockID, BlockID, uint64_t, SnapshotID>;
    using BlockType = Block;

    using AllocatorType = ICoWAllocator<Profile>;

    using BlockGuardT = LWBlockHandler<Block>;
    using BlockG = BlockGuardT;

    template <typename TargetBlockType>
    using BlockGuardTF = LWBlockHandler<TargetBlockType>;

    static constexpr bool IsCoW = true;


    static BlockID make_random_block_id() {
        return BlockID(0);
    }

    static UUID make_random_block_guid() {
        return UUID::make_random();
    }

    static UUID make_random_ctr_id() {
        return UUID::make_random();
    }

    static UUID make_random_snapshot_id() {
        return UUID::make_random();
    }
};


template <>
struct ProfileSpecificBlockTools<MemoryCoWProfile<>>{

    template <typename BlockT>
    static void after_deserialization(BlockT* block) noexcept {
        // do nothing here
    }
};


template <typename ValueHolder>
struct FieldFactory<MemCoWBlockID<ValueHolder>> {
    using Type = MemCoWBlockID<ValueHolder>;

    static constexpr size_t Size = sizeof(ValueHolder);

    template <typename SerializationData>
    static void serialize(SerializationData& data, const Type& field)
    {
        std::memcpy(data.buf, &field.value(), Size);
        data.buf    += Size;
        data.total  += Size;
    }

    template <typename DeserializationData>
    static void deserialize(DeserializationData& data, Type& field)
    {
        std::memcpy(&field.value(), data.buf, Size);
        data.buf += Size;
    }

    template <typename SerializationData>
    static void serialize(SerializationData& data, const Type* field, int32_t size)
    {
        for (int32_t c = 0; c < size; c++)
        {
            std::memcpy(data.buf, &field[c].value(), Size);
            data.buf    += Size;
            data.total  += Size;
        }
    }

    template <typename DeserializationData>
    static void deserialize(DeserializationData& data, Type* field, int32_t size)
    {
        for (int32_t c = 0; c < size; c++)
        {
            std::memcpy(&field[c].value(), data.buf, Size);
            data.buf += Size;
        }
    }
};


template <>
struct IBlockOperations<MemoryCoWProfile<>>: IBlockOperationsBase<MemoryCoWProfile<>> {

    using Base = IBlockOperationsBase<MemoryCoWProfile<>>;
    using typename Base::IDValueResolver;
    using typename Base::BlockType;

    virtual VoidResult mem_cow_resolve_ids(BlockType* block, const IDValueResolver* id_resolver) const noexcept = 0;
};




template <>
struct ContainerOperations<MemoryCoWProfile<>>: ContainerOperationsBase<MemoryCoWProfile<>> {

};


//template <>
//struct CtrReferenceable<MemoryCoWProfile<>>: CtrReferenceableBase<MemoryCoWProfile<>> {
//    using Profile = MemoryCoWProfile<>;

//    virtual CtrSharedPtr<CtrReferenceable<Profile>> shared_self() noexcept = 0;

//    virtual VoidResult traverse_ctr(BTreeTraverseNodeHandler<Profile>& node_handler) const noexcept = 0;
//};


template <typename ValueHolder>
ApiBlockIDHolder<2> block_id_holder_from(const MemCoWBlockID<ValueHolder>& uuid) noexcept {
    ApiBlockIDHolder<2> holder;
    holder.array[0] = uuid.value();
    return holder;
}

template <size_t N, typename ValueHolder>
void block_id_holder_to(const ApiBlockIDHolder<N>& holder, MemCoWBlockID<ValueHolder>& uuid) noexcept {
    static_assert(N >= 1, "");
    uuid.value() = holder.array[0];
}


}
