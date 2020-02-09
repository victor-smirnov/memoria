
// Copyright 2015 Victor Smirnov
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


#include <memoria/containers/multimap/multimap_names.hpp>
#include <memoria/containers/multimap/multimap_tools.hpp>
#include <memoria/containers/multimap/multimap_output_entries.hpp>
#include <memoria/containers/multimap/multimap_output_values.hpp>
#include <memoria/containers/multimap/multimap_output_keys.hpp>
#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/api/multimap/multimap_input.hpp>
#include <memoria/core/tools/static_array.hpp>

#include <memoria/api/multimap/multimap_api.hpp>

#include <vector>

namespace memoria {

MEMORIA_V1_CONTAINER_PART_BEGIN(multimap::CtrApiName)
public:
    using Types = typename Base::Types;
    using typename Base::IteratorPtr;

    using SharedIterator = typename IteratorPtr::element_type;

protected:
    using typename Base::Profile;
    using typename Base::NodeBaseG;
    using typename Base::Position;
    using typename Base::BranchNodeEntry;
    using typename Base::BlockUpdateMgr;
    using typename Base::CtrSizeT;
    using typename Base::CtrSizesT;

    using Key   = typename Types::Key;
    using Value = typename Types::Value;

    using KeyView   = typename DataTypeTraits<Key>::ViewType;
    using ValueView = typename DataTypeTraits<Value>::ViewType;

    using CtrApiTypes = ICtrApiTypes<typename Types::ContainerTypeName, Profile>;

    using IteratorAPI = MultimapIterator<Key, Value, Profile>;
    using IteratorAPIPtr = CtrSharedPtr<IteratorAPI>;

    using typename Base::BranchNodeExtData;
    using typename Base::LeafNodeExtData;
    using typename Base::ContainerTypeName;



public:
    void configure_types(
        const ContainerTypeName& type_name,
        BranchNodeExtData& branch_node_ext_data,
        LeafNodeExtData& leaf_node_ext_data
    ) {

    }

    Result<IteratorPtr> begin() const noexcept {
        return self().template ctr_seek_stream<0>(0);
    }

    Result<IteratorPtr> end() const noexcept
    {
        auto& self = this->self();

        auto res = self.sizes();
        MEMORIA_RETURN_IF_ERROR(res);

        auto ii = self.template ctr_seek_stream<1>(res.get()[1]);
        MEMORIA_RETURN_IF_ERROR(ii);

        ii.get()->iter_stream() = 1;

        MEMORIA_RETURN_IF_ERROR_FN(ii.get()->iter_to_structure_stream());

        return ii;
    }

    Result<IteratorPtr> ctr_seek(CtrSizeT idx) const noexcept
    {
        auto& self = this->self();
        auto ii = self.template ctr_seek_stream<0>(idx);
        MEMORIA_RETURN_IF_ERROR(ii);

        ii.get()->iter_stream() = 0;

        ii.get()->iter_to_structure_stream();

        return ii;
    }

    Result<IteratorAPIPtr> seek(CtrSizeT idx) const noexcept
    {
        auto& self = this->self();
        auto ii = self.template ctr_seek_stream<0>(idx);
        MEMORIA_RETURN_IF_ERROR(ii);

        ii.get()->iter_stream() = 0;

        MEMORIA_RETURN_IF_ERROR_FN(ii.get()->iter_to_structure_stream());

        return memoria_static_pointer_cast<MultimapIterator<Key, Value, Profile>>(std::move(ii));
    }

    template <typename ScannerApi, typename ScannerImpl>
    CtrSharedPtr<ScannerApi> as_scanner(IteratorAPIPtr iterator) const
    {
        if (MMA_UNLIKELY(!iterator)) {
            return CtrSharedPtr<ScannerApi>{};
        }
        else if (iterator->cxx_type() == typeid(typename Base::Iterator))
        {
            auto iter = memoria_static_pointer_cast<SharedIterator>(iterator);
            return ctr_make_shared<ScannerImpl>(iter);
        }
        else {
            MMA_THROW(RuntimeException()) << WhatCInfo("Invalid iterator type");
        }
    }


    virtual CtrSharedPtr<IEntriesScanner<CtrApiTypes, Profile>> entries_scanner(IteratorAPIPtr iterator) const
    {
        return as_scanner<
                IEntriesScanner<CtrApiTypes, Profile>,
                multimap::EntriesIteratorImpl<CtrApiTypes, Profile, IteratorPtr>
        >(iterator);
    }

    virtual CtrSharedPtr<IValuesScanner<CtrApiTypes, Profile>> values_scanner(IteratorAPIPtr iterator) const
    {
        if (MMA_UNLIKELY(!iterator)) {
            return CtrSharedPtr<IValuesScanner<CtrApiTypes, Profile>>{};
        }
        else if (iterator->cxx_type() == typeid(typename Base::Iterator))
        {
            auto iter = memoria_static_pointer_cast<SharedIterator>(iterator);
            return ctr_make_shared<multimap::ValuesIteratorImpl<CtrApiTypes, Profile, IteratorPtr>>(iter);
        }
        else {
            MMA_THROW(RuntimeException()) << WhatCInfo("Invalid iterator type");
        }
    }

    Result<CtrSharedPtr<IKeysScanner<CtrApiTypes, Profile>>> keys() const noexcept
    {
        using ResultT = Result<CtrSharedPtr<IKeysScanner<CtrApiTypes, Profile>>>;

        auto& self = this->self();
        auto ii = self.template ctr_seek_stream<0>(0);
        MEMORIA_RETURN_IF_ERROR(ii);

        ii.get()->iter_stream() = 0;
        MEMORIA_RETURN_IF_ERROR_FN(ii.get()->iter_to_structure_stream());

        auto ptr = ctr_make_shared<multimap::KeysIteratorImpl<CtrApiTypes, Profile, IteratorPtr>>(ii.get());

        return ResultT::of(memoria_static_pointer_cast<IKeysScanner<CtrApiTypes, Profile>>(ptr));
    }

    Result<IteratorAPIPtr> find(KeyView key) const noexcept
    {
        using ResultT = Result<IteratorAPIPtr>;

        auto& self = this->self();
        auto ii = self.ctr_multimap_find(key);
        MEMORIA_RETURN_IF_ERROR(ii);

        if (ii.get()->is_found(key))
        {
            auto res = ii.get()->to_values();
            MEMORIA_RETURN_IF_ERROR(res);

            return memoria_static_pointer_cast<MultimapIterator<Key, Value, Profile>>(std::move(ii));
        }
        else {
            return ResultT::of();
        }
    }

    virtual Result<IteratorAPIPtr> iterator() const noexcept {
        return memoria_static_pointer_cast<IteratorAPI>(self().begin());
    }

    Result<CtrSizeT> size() const noexcept
    {
        auto res = self().sizes();
        MEMORIA_RETURN_IF_ERROR(res);

        return Result<CtrSizeT>::of(res.get()[0]);
    }

    Result<IteratorPtr> ctr_multimap_find(KeyView key) const noexcept
    {
        return self().template ctr_find_max_ge<IntList<0, 1>>(0, key);
    }

    Result<IteratorPtr> find_or_create(KeyView key) noexcept
    {
        auto& self = this->self();

        auto iter = self.ctr_multimap_find(key);
        MEMORIA_RETURN_IF_ERROR(iter);

        if (!iter.get()->is_found(key))
        {
            iter.get()->insert_key(key);
        }

        return iter;
    }


    VoidResult append_entries(io::IOVectorProducer& producer) noexcept
    {
        auto ii = self().end();
        MEMORIA_RETURN_IF_ERROR(ii);

        auto res = ii.get()->insert_iovector(producer, 0, std::numeric_limits<int64_t>::max());
        MEMORIA_RETURN_IF_ERROR(res);

        return VoidResult::of();
    }





    VoidResult prepend_entries(io::IOVectorProducer& producer) noexcept
    {
        auto ii = self().begin();
        MEMORIA_RETURN_IF_ERROR(ii);


        auto res = ii.get()->insert_iovector(producer, 0, std::numeric_limits<int64_t>::max());
        MEMORIA_RETURN_IF_ERROR(res);

        return VoidResult::of();
    }




    VoidResult insert_entries(KeyView before, io::IOVectorProducer& producer) noexcept
    {
        auto ii = self().ctr_multimap_find(before);
        MEMORIA_RETURN_IF_ERROR(ii);


        auto res = ii.get()->insert_iovector(producer, 0, std::numeric_limits<int64_t>::max());
        MEMORIA_RETURN_IF_ERROR(res);

        return VoidResult::of();
    }

    BoolResult upsert(KeyView key, io::IOVectorProducer& producer) noexcept
    {
        auto ii = self().ctr_multimap_find(key);
        MEMORIA_RETURN_IF_ERROR(ii);

        if (ii.get()->is_found(key))
        {
            auto res0 = ii.get()->remove(1);
            MEMORIA_RETURN_IF_ERROR(res0);

            auto res1 = ii.get()->insert_iovector(producer, 0, std::numeric_limits<int64_t>::max());
            MEMORIA_RETURN_IF_ERROR(res1);

            return true;
        }

        auto res2 = ii.get()->insert_iovector(producer, 0, std::numeric_limits<int64_t>::max());
        MEMORIA_RETURN_IF_ERROR(res2);

        return false;
    }




    BoolResult contains(const KeyView& key) const noexcept
    {
        MEMORIA_TRY(iter, self().ctr_multimap_find(key));
        return BoolResult::of(iter->is_found(key));
    }

    BoolResult remove(const KeyView& key) noexcept
    {
        MEMORIA_TRY(ii, self().ctr_multimap_find(key));
        if (ii->is_found(key))
        {
            MEMORIA_TRY_VOID(ii.get()->remove(1));
            return BoolResult::of(true);
        }

        return BoolResult::of(false);
    }


    BoolResult remove_all(const KeyView& from, const KeyView& to) noexcept
    {
        auto ii = self().ctr_multimap_find(from);
        MEMORIA_RETURN_IF_ERROR(ii);

        auto jj = self().ctr_multimap_find(to);
        MEMORIA_RETURN_IF_ERROR(jj);

        return ii.get()->iter_remove_all(*jj.get());
    }

    BoolResult remove_from(const KeyView& from) noexcept
    {
        auto ii = self().ctr_multimap_find(from);
        MEMORIA_RETURN_IF_ERROR(ii);

        auto jj = self().end();
        MEMORIA_RETURN_IF_ERROR(jj);

        return ii.get()->iter_remove_all(*jj.get());
    }


    BoolResult remove_before(const KeyView& to) noexcept
    {
        auto ii = self().begin();
        MEMORIA_RETURN_IF_ERROR(ii);

        auto jj = self().ctr_multimap_find(to);
        MEMORIA_RETURN_IF_ERROR(jj);

        return ii.get()->iter_remove_all(*jj.get());
    }


protected:

MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(multimap::CtrApiName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS



#undef M_PARAMS
#undef M_TYPE

}
