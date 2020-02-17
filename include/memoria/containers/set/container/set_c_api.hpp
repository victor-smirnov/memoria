
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

public:
    using Types = typename Base::Types;
    using typename Base::IteratorPtr;

protected:
    using typename Base::NodeBaseG;
    using typename Base::Position;
    using typename Base::BranchNodeEntry;
    using typename Base::BlockUpdateMgr;

    using Key = typename Types::Key;

    using CtrSizeT = typename Types::CtrSizeT;

    using KeyView   = typename DataTypeTraits<Key>::ViewType;

    using Profile   = typename Types::Profile;

    using BufferT   = DataTypeBuffer<Key>;
    using CtrApiTypes = ICtrApiTypes<typename Types::ContainerTypeName, Profile>;

public:

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

    Result<CtrSharedPtr<SetIterator<Key, Profile>>> find(KeyView key) const noexcept
    {
        return memoria_static_pointer_cast<SetIterator<Key, Profile>>(self().ctr_set_find(key));
    }

//    bool contains_element(KeyView key) {
//        return false;
//    }

//    bool insert_element(KeyView key) {
//        return false;
//    }

//    bool remove_element(KeyView key) {
//        return self().remove(key);
//    }



    Result<CtrSharedPtr<SetIterator<Key, Profile>>> iterator() const noexcept
    {
        auto iter = self().ctr_begin();
        return memoria_static_pointer_cast<SetIterator<Key, Profile>>(std::move(iter));
    }

    VoidResult append(io::IOVectorProducer& producer) noexcept
    {
        auto& self = this->self();

        MEMORIA_TRY(iter, self.ctr_end());
        MEMORIA_TRY_VOID(iter->insert_iovector(producer, 0, std::numeric_limits<int64_t>::max()));

        return VoidResult::of();
    }

    virtual VoidResult prepend(io::IOVectorProducer& producer) noexcept
    {
        auto& self = this->self();

        MEMORIA_TRY(iter, self.ctr_begin());
        MEMORIA_TRY_VOID(iter->insert_iovector(producer, 0, std::numeric_limits<int64_t>::max()));

        return VoidResult::of();
    }

    virtual VoidResult insert(KeyView before, io::IOVectorProducer& producer) noexcept
    {
        auto& self = this->self();

        MEMORIA_TRY(iter, self.ctr_set_find(before));

        if (iter->is_found(before))
        {
            return MEMORIA_MAKE_GENERIC_ERROR("Requested key is found. Can't insert enties this way.");
        }
        else {
            MEMORIA_TRY_VOID(iter.get()->insert_iovector(producer, 0, std::numeric_limits<int64_t>::max()));
            return VoidResult::of();
        }
    }

    /**
     * Returns true if set is already containing the element
     */
    BoolResult insert(KeyView k) noexcept
    {
        MEMORIA_TRY(iter, self().ctr_set_find(k));

        if (iter->is_found(k))
        {
            return BoolResult::of(true);
        }
        else {
            MEMORIA_TRY_VOID(iter.get()->insert(k));
            return BoolResult::of(false);
        }
    }

    /**
     * Returns true if the set contained the element
     */
    BoolResult remove(KeyView key) noexcept
    {
        MEMORIA_TRY(iter, self().ctr_set_find(key));

        if ((!iter->iter_is_end()) && iter->key() == key)
        {
            MEMORIA_TRY_VOID(iter->remove(false));
            return BoolResult::of(true);
        }

        return BoolResult::of(false);
    }

    /**
     * Returns true if the set contains the element
     */
    BoolResult contains(KeyView k) noexcept
    {
        MEMORIA_TRY(iter, self().ctr_set_find(k));
        return BoolResult::of(iter->is_found(k));
    }

    Result<CtrSizeT> remove(CtrSizeT from, CtrSizeT to) noexcept
    {
        auto& self = this->self();

        MEMORIA_TRY(ii_from, self.ctr_seek(from));

        return ii_from->remove_from(to - from);
    }

    Result<CtrSizeT> remove_from(CtrSizeT from) noexcept
    {
        MEMORIA_TRY(size, self().size());
        return remove(from, size);
    }

    Result<CtrSizeT> remove_up_to(CtrSizeT pos) noexcept
    {
        auto& self = this->self();

        MEMORIA_TRY(ii_from, self.ctr_begin());

        return ii_from->remove_from(pos);
    }

    virtual VoidResult read_to(BufferT& buffer, CtrSizeT start, CtrSizeT length) const noexcept
    {
        using ResultT = VoidResult;
        auto& self = this->self();

        MEMORIA_TRY(ii, self.ctr_seek(start));

        CtrSizeT cnt{};
        SetScanner<CtrApiTypes, Profile> scanner(ii);

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
                MEMORIA_TRY_VOID(scanner.next_leaf());
            }
        }

        return ResultT::of();
    }

    virtual VoidResult insert(CtrSizeT at, const BufferT& buffer, size_t start, size_t size) noexcept
    {
        auto& self = this->self();

        if (start + size > buffer.size())
        {
            return MEMORIA_MAKE_GENERIC_ERROR("Vector insert_buffer range check error: {}, {}, {}", start, size, buffer.size());
        }

        SetProducer<CtrApiTypes> producer([&](auto& values, auto appended_size){
            size_t batch_size = 8192;
            size_t limit = (appended_size + batch_size <= size) ? batch_size : size - appended_size;

            for (size_t c = 0; c < limit; c++) {
                values.append(buffer[start + appended_size + c]);
            }

            return limit != batch_size;
        });

        MEMORIA_TRY(ii, self.ctr_seek(at));
        MEMORIA_TRY_VOID(ii->insert_iovector(producer, 0, std::numeric_limits<CtrSizeT>::max()));

        return VoidResult::of();
    }



MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(set::CtrApiName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS

#undef M_PARAMS
#undef M_TYPE

}
