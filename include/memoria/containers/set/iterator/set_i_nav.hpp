
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
#include <memoria/core/types.hpp>

#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/containers/set/set_tools.hpp>

#include <iostream>

namespace memoria {

using bt::StreamTag;

MEMORIA_V1_ITERATOR_PART_BEGIN(set::ItrNavName)

    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::NodeBaseG                                            NodeBaseG;


    typedef typename Base::Container::Types::Key                                Key;
    typedef typename Base::Container::Types::BranchNodeEntry                    BranchNodeEntry;
    typedef typename Base::Container                                            Container;
    typedef typename Base::Container::Position                                  Position;

    using CtrSizeT  = typename Container::Types::CtrSizeT;

    using KeyView   = typename DataTypeTraits<Key>::ViewType;

    using Profile   = typename Types::Profile;

public:
    VoidResult insert(const KeyView& key) noexcept
    {
        auto& self = this->self();

        auto res0 = self.ctr().iter_insert_entry(
                self,
                set::KeyEntry<KeyView, CtrSizeT>(key)
        );
        MEMORIA_RETURN_IF_ERROR(res0);

        auto res1 = self.iter_btss_skip_fw(1);
        MEMORIA_RETURN_IF_ERROR(res1);

        return VoidResult::of();
    }


    VoidResult remove() noexcept
    {
        auto& self = this->self();

        auto res = self.ctr().ctr_remove_entry(self);
        MEMORIA_RETURN_IF_ERROR(res);

        if (self.iter_is_end())
        {
            auto res = self.iter_btss_skip_fw(0);
            MEMORIA_RETURN_IF_ERROR(res);
        }

        return VoidResult::of();
    }

    auto remove(CtrSizeT size)
    {
        return Base::remove(size);
    }


    auto findFwGT(int32_t index, KeyView key)
    {
        return self().template iter_find_fw_gt<IntList<1>>(index, key);
    }

    auto findFwGE(int32_t index, KeyView key)
    {
        return self().template iter_find_fw_ge<IntList<1>>(index, key);
    }

    auto findBwGT(int32_t index, KeyView key)
    {
        return self().template iter_find_bw_gt<IntList<1>>(index, key);
    }

    auto iter_map_find_bw_ge(int32_t index, KeyView key)
    {
        return self().template iter_find_bw_ge<IntList<1>>(index, key);
    }

    auto key() const noexcept -> Datum<Key>
    {
        return std::get<0>(self().ctr().template iter_read_leaf_entry<IntList<1>>(self().iter_leaf(), self().iter_local_pos(), 0));
    }

    bool is_found(const KeyView& k) const noexcept
    {
        auto& self = this->self();
        return (!self.is_end()) && self.key() == k;
    }

MEMORIA_V1_ITERATOR_PART_END

#define M_TYPE      MEMORIA_V1_ITERATOR_TYPE(set::ItrNavName)
#define M_PARAMS    MEMORIA_V1_ITERATOR_TEMPLATE_PARAMS


#undef M_TYPE
#undef M_PARAMS

}