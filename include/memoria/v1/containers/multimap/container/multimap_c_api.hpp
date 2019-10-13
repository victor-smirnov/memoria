
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


#include <memoria/v1/containers/multimap/multimap_names.hpp>
#include <memoria/v1/containers/multimap/multimap_tools.hpp>
#include <memoria/v1/containers/multimap/multimap_output_entries.hpp>
#include <memoria/v1/containers/multimap/multimap_output_values.hpp>
#include <memoria/v1/containers/multimap/multimap_output_keys.hpp>
#include <memoria/v1/core/container/container.hpp>
#include <memoria/v1/core/container/macros.hpp>

#include <memoria/v1/api/multimap/multimap_input.hpp>
#include <memoria/v1/core/tools/static_array.hpp>

#include <memoria/v1/api/multimap/multimap_api.hpp>

#include <vector>

namespace memoria {
namespace v1 {

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

    using IteratorAPIPtr = CtrSharedPtr<MultimapIterator<Key, Value, Profile>>;

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

    IteratorPtr begin() const {
        return self().template ctr_seek_stream<0>(0);
    }

    IteratorPtr end() const {
        auto& self = this->self();
        auto ii = self.template ctr_seek_stream<1>(self.sizes()[1]);

        ii->iter_stream() = 1;
        ii->iter_to_structure_stream();

        return ii;
    }

    IteratorPtr ctr_seek(CtrSizeT idx) const
    {
        auto& self = this->self();
        auto ii = self.template ctr_seek_stream<0>(idx);

        ii->iter_stream() = 0;

        ii->iter_to_structure_stream();

        return ii;
    }

    IteratorAPIPtr seek(CtrSizeT idx) const
    {
        auto& self = this->self();
        auto ii = self.template ctr_seek_stream<0>(idx);

        ii->iter_stream() = 0;

        ii->iter_to_structure_stream();

        return ii;
    }

    template <typename ScannerApi, typename ScannerImpl>
    CtrSharedPtr<ScannerApi> as_scanner(IteratorAPIPtr iterator) const
    {
        if (MMA1_UNLIKELY(!iterator)) {
            return CtrSharedPtr<ScannerApi>{};
        }
        else if (iterator->cxx_type() == typeid(typename Base::Iterator))
        {
            auto iter = memoria_static_pointer_cast<SharedIterator>(iterator);
            return ctr_make_shared<ScannerImpl>(iter);
        }
        else {
            MMA1_THROW(RuntimeException()) << WhatCInfo("Invalid iterator type");
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
        if (MMA1_UNLIKELY(!iterator)) {
            return CtrSharedPtr<IValuesScanner<CtrApiTypes, Profile>>{};
        }
        else if (iterator->cxx_type() == typeid(typename Base::Iterator))
        {
            auto iter = memoria_static_pointer_cast<SharedIterator>(iterator);
            return ctr_make_shared<multimap::ValuesIteratorImpl<CtrApiTypes, Profile, IteratorPtr>>(iter);
        }
        else {
            MMA1_THROW(RuntimeException()) << WhatCInfo("Invalid iterator type");
        }
    }

    CtrSharedPtr<IKeysScanner<CtrApiTypes, Profile>> keys() const
    {
        auto& self = this->self();
        auto ii = self.template ctr_seek_stream<0>(0);

        ii->iter_stream() = 0;
        ii->iter_to_structure_stream();

        auto ptr = ctr_make_shared<multimap::KeysIteratorImpl<CtrApiTypes, Profile, IteratorPtr>>(ii);

        return memoria_static_pointer_cast<IKeysScanner<CtrApiTypes, Profile>>(ptr);
    }

    IteratorAPIPtr find(KeyView key) const
    {
        auto& self = this->self();
        auto ii = self.ctr_multimap_find(key);

        if (ii->is_found(key))
        {
            ii->to_values();
            return ii;
        }
        else {
            return IteratorAPIPtr{};
        }
    }

    virtual IteratorAPIPtr iterator() const {
        return self().begin();
    }

    CtrSizeT size() const {
        return self().sizes()[0];
    }

    IteratorPtr ctr_multimap_find(KeyView key) const
    {
        return self().template ctr_find_max_ge<IntList<0, 1>>(0, key);
    }

    IteratorPtr find_or_create(KeyView key)
    {
        auto& self = this->self();

        auto iter = self.ctr_multimap_find(key);

        if (!iter->is_found(key))
        {
            iter->insert_key(key);
        }

        return iter;
    }


    void append_entries(io::IOVectorProducer& producer)
    {
        self().end()->insert_iovector(producer, 0, std::numeric_limits<int64_t>::max());
    }





    void prepend_entries(io::IOVectorProducer& producer)
    {
        self().begin()->insert_iovector(producer, 0, std::numeric_limits<int64_t>::max());
    }




    void insert_entries(KeyView before, io::IOVectorProducer& producer)
    {
        auto ii = self().ctr_multimap_find(before);
        ii->insert_iovector(producer, 0, std::numeric_limits<int64_t>::max());
    }

    bool upsert(KeyView key, io::IOVectorProducer& producer)
    {
        auto ii = self().ctr_multimap_find(key);

        if (ii->is_found(key))
        {
            ii->remove(1);
            ii->insert_iovector(producer, 0, std::numeric_limits<int64_t>::max());

            return true;
        }

        ii->insert_iovector(producer, 0, std::numeric_limits<int64_t>::max());

        return false;
    }




    bool contains(const KeyView& key) const
    {
        auto iter = self().ctr_multimap_find(key);
        return iter->is_found(key);
    }

    bool remove(const KeyView& key)
    {
        auto ii = self().ctr_multimap_find(key);

        if (ii->is_found(key)) {
            ii->remove(1);
            return true;
        }

        return false;
    }


    bool remove_all(const KeyView& from, const KeyView& to)
    {
        auto ii = self().ctr_multimap_find(from);
        auto jj = self().ctr_multimap_find(to);

        return ii->iter_remove_all(*jj.get());
    }

    bool remove_from(const KeyView& from)
    {
        auto ii = self().ctr_multimap_find(from);
        auto jj = self().end();

        return ii->iter_remove_all(*jj.get());
    }


    bool remove_before(const KeyView& to)
    {
        auto ii = self().begin();
        auto jj = self().ctr_multimap_find(to);

        return ii->iter_remove_all(*jj.get());
    }


protected:

MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(multimap::CtrApiName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS



#undef M_PARAMS
#undef M_TYPE

}}
