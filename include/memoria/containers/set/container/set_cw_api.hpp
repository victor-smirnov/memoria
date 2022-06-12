
// Copyright 2016 Victor Smirnov
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

#include <memoria/prototypes/bt/shuttles/bt_shuttle_base.hpp>


#include <memoria/containers/set/set_names.hpp>
#include <memoria/containers/set/set_tools.hpp>
#include <memoria/containers/set/set_api_impl.hpp>
#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>


#include <memoria/api/set/set_producer.hpp>
#include <memoria/api/set/set_api.hpp>

#include <memoria/containers/collection/collection_entry_impl.hpp>

#include <vector>

namespace memoria {

MEMORIA_V1_CONTAINER_PART_BEGIN(set::CtrWApiName)

    using typename Base::ApiProfileT;

public:
    using Types = typename Base::Types;

    using typename Base::TreeNodeConstPtr;
    using typename Base::TreeNodePtr;
    using typename Base::Position;
    using typename Base::BranchNodeEntry;
    using typename Base::ShuttleTypes;
    using typename Base::TreePathT;


protected:

    using Key = typename Types::Key;

    using CtrSizeT = typename Types::CtrSizeT;

    using KeyView   = typename DataTypeTraits<Key>::ViewType;

    using Profile   = typename Types::Profile;

    using BufferT   = DataTypeBuffer<Key>;
    using CtrApiTypes = ICtrApiTypes<typename Types::ContainerTypeName, Profile>;



public:

    using CollectionChunkT = CollectionChunk<Key, ApiProfile<Profile>>;
    using ChunkSharedPtr = IterSharedPtr<CollectionChunkT>;


    using typename Base::CollectionChunkTypes;
//    struct CollectionChunkTypes: Types {
//        using CollectionKeyType = Key;
//        using ShuttleTypes = typename Base::ShuttleTypes;
//    };


    //using CollectionChunkImplT = CollectionChunkImpl<CollectionChunkTypes>;
    using typename Base::CollectionChunkImplT;

    //using EntriesPath = IntList<0, 1>;

    using typename Base::EntriesPath;

    template <typename ShuttleTypes>
    using FindShuttle = bt::FindForwardShuttle<ShuttleTypes, EntriesPath, CollectionChunkImplT>;


    template <typename LeafPath>
    using TargetType = typename Types::template TargetType<LeafPath>;


    using typename Base::BranchNodeExtData;
    using typename Base::LeafNodeExtData;
    using typename Base::ContainerTypeName;

//    void configure_types(
//        const ContainerTypeName& type_name,
//        BranchNodeExtData& branch_node_ext_data,
//        LeafNodeExtData& leaf_node_ext_data
//    ) {

//    }

//    IterSharedPtr<CollectionChunkImplT> ctr_set_find(const KeyView& k) const
//    {
//        return self().ctr_descend(
//            TypeTag<CollectionChunkImplT>{},
//            bt::ShuttleTag<FindShuttle>{},
//            k, 0, SearchType::GE
//        );
//    }


//    ChunkSharedPtr find(KeyView key) const
//    {
//        return self().ctr_set_find(key);
//    }



    ChunkSharedPtr append(io::IOVectorProducer& producer)
    {
        auto& self = this->self();

        auto iter = self.ctr_seek_entry(self.size());

        auto jj = self.ctr_insert_iovector2(std::move(iter), producer, 0, std::numeric_limits<CtrSizeT>::max());
        return memoria_static_pointer_cast<CollectionChunkImplT>(jj);
    }

    ChunkSharedPtr prepend(io::IOVectorProducer& producer)
    {
        auto& self = this->self();
        auto iter = self.ctr_seek_entry(0);
        auto jj = self.ctr_insert_iovector2(std::move(iter), producer, 0, std::numeric_limits<CtrSizeT>::max());
        return memoria_static_pointer_cast<CollectionChunkImplT>(jj);
    }

    ChunkSharedPtr insert(KeyView before, io::IOVectorProducer& producer)
    {
        auto& self = this->self();

        auto iter = self.ctr_set_find(before);

        if (iter->is_found(before))
        {
            MEMORIA_MAKE_GENERIC_ERROR("Requested key is found. Can't insert enties this way.").do_throw();
        }
        else {
            auto jj = self.ctr_insert_iovector2(std::move(iter), producer, 0, std::numeric_limits<CtrSizeT>::max());
            return memoria_static_pointer_cast<CollectionChunkImplT>(jj);
        }
    }

    /**
     * Returns true if set is already containing the element
     */
    bool upsert(KeyView k)
    {
        auto iter = self().ctr_set_find(k);

        if (iter->is_found(k)) {
            return true;
        }
        else {
            self().ctr_insert_entry(
                std::move(iter),
                set::KeyEntry<KeyView, CtrSizeT>(k)
            );

            return false;
        }
    }

    /**
     * Returns true if the set contained the element
     */
    bool remove(KeyView key)
    {
        auto& self = this->self();
        auto iter = self.ctr_set_find(key);

        if (iter->is_found(key))
        {
            auto idx = iter->iter_leaf_position();
            self.ctr_remove_entry(iter->path(), idx);

            return true;
        }

        return false;
    }

    /**
     * Returns true if the set contains the element
     */
//    bool contains(KeyView k)
//    {
//        auto iter = self().ctr_set_find(k);
//        return iter->is_found(k);
//    }

    void remove(CtrSizeT from, CtrSizeT to)
    {
        self().ctr_remove(from, to);
    }

    void remove_from(CtrSizeT from) {
        self().ctr_remove_from(from);
    }

    void remove_up_to(CtrSizeT pos) {
        self().ctr_remove_up_to(pos);
    }

//    virtual void read_to(BufferT& buffer, CtrSizeT start, CtrSizeT length) const
//    {
//        auto& self = this->self();

//        auto ii = self.seek_entry(start);

//        CtrSizeT cnt{};

//        size_t local_cnt;
//        while (cnt < length && is_valid_chunk(ii))
//        {
//            local_cnt = 0;
//            CtrSizeT remainder   = length - cnt;
//            CtrSizeT values_size = static_cast<CtrSizeT>(ii->keys().size());

//            if (values_size <= remainder)
//            {
//                buffer.append(ii->keys());
//                cnt += values_size;
//            }
//            else {
//                buffer.append(ii->keys().first(remainder));
//                cnt += remainder;
//            }

//            if (cnt < length)
//            {
//                ii = ii->next_chunk();
//            }
//        }
//    }

    ChunkSharedPtr insert(CtrSizeT at, const BufferT& buffer, size_t start, size_t size)
    {
        auto& self = this->self();

        if (start + size > buffer.size()) {
            MEMORIA_MAKE_GENERIC_ERROR("Vector insert_buffer range check error: {}, {}, {}", start, size, buffer.size()).do_throw();
        }

        SetProducer<CtrApiTypes> producer([&](auto& values, auto appended_size){
            size_t batch_size = 8192;
            size_t limit = (appended_size + batch_size <= size) ? batch_size : size - appended_size;

            for (size_t c = 0; c < limit; c++) {
                values.append(buffer[start + appended_size + c]);
            }

            return limit != batch_size;
        });

        auto ii = self.ctr_seek_entry(at);
        auto jj = self.ctr_insert_iovector2(std::move(ii), producer, 0, std::numeric_limits<CtrSizeT>::max());
        return memoria_static_pointer_cast<CollectionChunkImplT>(jj);
    }


MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(set::CtrWApiName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS

#undef M_PARAMS
#undef M_TYPE

}
