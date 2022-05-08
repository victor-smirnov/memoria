
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
#include <memoria/api/set/set_scanner.hpp>
#include <memoria/api/set/set_api.hpp>

#include <vector>

namespace memoria {

MEMORIA_V1_CONTAINER_PART_BEGIN(set::CtrApiName)

    using typename Base::ApiProfileT;

public:
    using Types = typename Base::Types;
    using typename Base::IteratorPtr;




    using typename Base::TreeNodeConstPtr;
    using typename Base::TreeNodePtr;
    using typename Base::Position;
    using typename Base::BranchNodeEntry;
    using typename Base::ShuttleTypes;
    using typename Base::TreePathT;
    using typename Base::NodeChain;
protected:

    using Key = typename Types::Key;

    using CtrSizeT = typename Types::CtrSizeT;

    using KeyView   = typename DataTypeTraits<Key>::ViewType;

    using Profile   = typename Types::Profile;

    using BufferT   = DataTypeBuffer<Key>;
    using CtrApiTypes = ICtrApiTypes<typename Types::ContainerTypeName, Profile>;

public:

    using CollectionEntryT = CollectionEntry<Key, ApiProfile<Profile>>;
    using EntrySharedPtr = IterSharedPtr<CollectionEntryT>;

    template <typename LeafPath>
    using TargetType = typename Types::template TargetType<LeafPath>;


    using typename Base::BranchNodeExtData;
    using typename Base::LeafNodeExtData;
    using typename Base::ContainerTypeName;

    void configure_types(
        const ContainerTypeName& type_name,
        BranchNodeExtData& branch_node_ext_data,
        LeafNodeExtData& leaf_node_ext_data
    ) {

    }

    IterSharedPtr<SetIterator<Key, ApiProfileT>> find(KeyView key) const
    {
        //return memoria_static_pointer_cast<SetIterator<Key, ApiProfileT>>(self().ctr_set_find(key));
        return self().ctr_set_find(key);
    }



    IterSharedPtr<SetIterator<Key, ApiProfileT>> iterator() const
    {
        auto iter = self().ctr_begin();
        return iter;//memoria_static_pointer_cast<SetIterator<Key, ApiProfileT>>(std::move(iter));
    }

    void append(io::IOVectorProducer& producer)
    {
        auto& self = this->self();

        auto iter = self.ctr_end();
        iter->insert_iovector(producer, 0, std::numeric_limits<int64_t>::max());
    }

    virtual void prepend(io::IOVectorProducer& producer)
    {
        auto& self = this->self();

        auto iter = self.ctr_begin();
        iter->insert_iovector(producer, 0, std::numeric_limits<int64_t>::max());
    }

    virtual void insert(KeyView before, io::IOVectorProducer& producer)
    {
        auto& self = this->self();

        auto iter = self.ctr_set_find(before);

        if (iter->is_found(before))
        {
            MEMORIA_MAKE_GENERIC_ERROR("Requested key is found. Can't insert enties this way.").do_throw();
        }
        else {
            iter.get()->insert_iovector(producer, 0, std::numeric_limits<int64_t>::max());
        }
    }

    /**
     * Returns true if set is already containing the element
     */
    bool insert(KeyView k)
    {
        IteratorPtr iter = self().ctr_set_find(k);

        if (iter->is_found(k))
        {
            return true;
        }
        else {
            iter.get()->insert(k);
            return false;
        }
    }

    /**
     * Returns true if the set contained the element
     */
    bool remove(KeyView key)
    {
        auto iter = self().ctr_set_find(key);

        if ((!iter->iter_is_end()) && iter->key() == key)
        {
            iter->remove(false);
            return true;
        }

        return false;
    }

    /**
     * Returns true if the set contains the element
     */
    bool contains(KeyView k)
    {
        auto iter = self().ctr_set_find(k);
        return iter->is_found(k);
    }

    CtrSizeT remove(CtrSizeT from, CtrSizeT to)
    {
        auto& self = this->self();

        auto ii_from = self.ctr_seek(from);

        return ii_from->remove_from(to - from);
    }

    CtrSizeT remove_from(CtrSizeT from)
    {
        auto size = self().size();
        return remove(from, size);
    }

    CtrSizeT remove_up_to(CtrSizeT pos)
    {
        auto& self = this->self();

        auto ii_from = self.ctr_begin();

        return ii_from->remove_from(pos);
    }

    virtual void read_to(BufferT& buffer, CtrSizeT start, CtrSizeT length) const
    {
        auto& self = this->self();

        auto ii = self.ctr_seek(start);

        CtrSizeT cnt{};
        SetScanner<CtrApiTypes, ApiProfileT> scanner(ii);

        size_t local_cnt;
        while (cnt < length && !scanner.is_end())
        {
            local_cnt = 0;
            CtrSizeT remainder   = length - cnt;
            CtrSizeT values_size = static_cast<CtrSizeT>(scanner.keys().size());

            if (values_size <= remainder)
            {
                buffer.append(scanner.keys());
                cnt += values_size;
            }
            else {
                buffer.append(scanner.keys().first(remainder));
                cnt += remainder;
            }

            if (cnt < length)
            {
                scanner.next_leaf();
            }
        }
    }

    virtual void insert(CtrSizeT at, const BufferT& buffer, size_t start, size_t size)
    {
        auto& self = this->self();

        if (start + size > buffer.size())
        {
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

        auto ii = self.ctr_seek(at);
        ii->insert_iovector(producer, 0, std::numeric_limits<CtrSizeT>::max());
    }


MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(set::CtrApiName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS

#undef M_PARAMS
#undef M_TYPE

}
