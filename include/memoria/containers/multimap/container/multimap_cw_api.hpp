
// Copyright 2015-2022 Victor Smirnov
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
#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/api/multimap/multimap_input.hpp>
#include <memoria/core/tools/static_array.hpp>

#include <memoria/api/multimap/multimap_api.hpp>

#include <memoria/containers/multimap/multimap_keys_chunk_impl.hpp>
#include <memoria/containers/multimap/multimap_values_chunk_impl.hpp>

#include <vector>

namespace memoria {

MEMORIA_V1_CONTAINER_PART_BEGIN(multimap::CtrWApiName)
public:
    using Types = typename Base::Types;

protected:
    using typename Base::Profile;
    using typename Base::TreeNodePtr;
    using typename Base::Position;
    using typename Base::BranchNodeEntry;
    using typename Base::CtrSizeT;
    using typename Base::CtrSizesT;
    using typename Base::ApiProfileT;
    using typename Base::ShuttleTypes;

    using Key   = typename Types::Key;
    using Value = typename Types::Value;

    using KeyView   = typename DataTypeTraits<Key>::ViewType;
    using ValueView = typename DataTypeTraits<Value>::ViewType;

    using CtrApiTypes = ICtrApiTypes<typename Types::ContainerTypeName, ApiProfileT>;

    using typename Base::KeysChunkT;
    using typename Base::KeysChunkPtrT;

    using typename Base::MultimapChunkTypes;


    using typename Base::KeysChunkImplT;
    using typename Base::KeysChunkImplPtrT;

    using typename Base::KeysPath;

    template <typename ShuttleTypes>
    using FindShuttle = bt::FindForwardShuttle<ShuttleTypes, KeysPath, KeysChunkImplT>;

public:





    void append_entries(io::IOVectorProducer& producer)
    {
        auto& self = this->self();
        auto ii = self.ctr_seek_key(self.size());
        self.ctr_insert_iovector(std::move(ii), producer, 0, std::numeric_limits<int64_t>::max());
    }

    void prepend_entries(io::IOVectorProducer& producer)
    {
        auto& self = this->self();
        auto ii = self.ctr_seek_key(CtrSizeT{});
        self.ctr_insert_iovector(std::move(ii), producer, 0, std::numeric_limits<int64_t>::max());
    }




    void insert_entries(KeyView before, io::IOVectorProducer& producer)
    {
        auto& self = this->self();
        auto ii = self.ctr_map_find(before);
        self.ctr_insert_iovector(std::move(ii), producer, 0, std::numeric_limits<int64_t>::max());
    }

    bool upsert(KeyView key, io::IOVectorProducer& producer)
    {
        auto& self = this->self();
        auto ii = self.ctr_map_find(key);

        if (ii->is_found(key))
        {
            auto jj = ii->iter_next(1);
            auto kk = self.ctr_remove_range(std::move(ii), std::move(jj));

            self.ctr_insert_iovector(std::move(kk), producer, 0, std::numeric_limits<int64_t>::max());
            return true;
        }

        self.ctr_insert_iovector(std::move(ii), producer, 0, std::numeric_limits<int64_t>::max());
        return false;
    }





    bool remove(const KeyView& key)
    {
        auto& self = this->self();
        auto ii = self.ctr_map_find(key);
        if (ii->is_found(key))
        {
            auto jj = ii->iter_next(1);
            self.ctr_remove_range(std::move(ii), std::move(jj));
            return true;
        }

        return false;
    }


    bool remove_all(const KeyView& from, const KeyView& to)
    {
        auto& self = this->self();
        auto ii = self.ctr_map_find(from);
        auto jj = self.ctr_map_find(to);

        self.ctr_remove_range(std::move(ii), std::move(jj));

        return true;
    }

    bool remove_from(const KeyView& from)
    {
        auto& self = this->self();
        auto ii = self.ctr_map_find(from);
        auto jj = self.ctr_seek_key(self.size());

        self.ctr_remove_range(std::move(ii), std::move(jj));
        return true;
    }


    bool remove_before(const KeyView& to)
    {
        auto& self = this->self();
        auto ii = self.ctr_seek_key(0);
        auto jj = self.ctr_map_find(to);

        self.ctr_remove_range(std::move(ii), std::move(jj));
        return true;
    }


protected:

MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(multimap::CtrWApiName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS



#undef M_PARAMS
#undef M_TYPE

}
