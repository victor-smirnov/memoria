
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

#include <memoria/containers/map/map_names.hpp>
#include <memoria/containers/map/map_tools.hpp>


#include <memoria/core/types.hpp>

#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/prototypes/bt/bt_iterator.hpp>



#include <iostream>

namespace memoria {

using bt::StreamTag;

MEMORIA_V1_ITERATOR_PART_BEGIN(map::ItrNavMaxName)

    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::NodeBasePtr                                            NodeBasePtr;

    typedef typename Base::Container::Value                                     Value;
    typedef typename Base::Container::Key                                       Key;
    typedef typename Base::Container::BranchNodeEntry                           BranchNodeEntry;
    typedef typename Base::Container                                            Container;
    typedef typename Base::Container::Position                                  Position;

    using KeyView   = typename DataTypeTraits<Key>::ViewType;
    using ValueView = typename DataTypeTraits<Value>::ViewType;

    using CtrSizeT = typename Container::Types::CtrSizeT;

public:
    VoidResult insert(const KeyView& key, const ValueView& value) noexcept
    {
        auto& self = this->self();

        MEMORIA_TRY_VOID(self.ctr().iter_insert_entry(
                self,
                map::KeyValueEntry<KeyView, ValueView, CtrSizeT>(key, value)
        ));
        MEMORIA_TRY_VOID(self.iter_btss_skip_fw(1));

        return VoidResult::of();
    }


    VoidResult remove() noexcept
    {
        auto& self = this->self();

        MEMORIA_TRY_VOID(self.ctr().ctr_remove_entry(self));

        if (self.iter_is_end())
        {
            MEMORIA_TRY_VOID(self.iter_btss_skip_fw(0));
        }

        return VoidResult::of();
    }

    auto remove(CtrSizeT size)
    {
        return Base::remove(size);
    }


    auto findFwGT(int32_t index, KeyView key) noexcept
    {
        return self().template iter_find_fw_gt<IntList<1>>(index, key);
    }

    auto iter_map_find_fw_ge(int32_t index, KeyView key) noexcept
    {
        return self().template iter_find_fw_ge<IntList<1>>(index, key);
    }

    auto iter_map_find_bw_gt(int32_t index, KeyView key)
    {
        return self().template iter_find_bw_gt<IntList<1>>(index, key);
    }

    auto iter_map_find_bw_ge(int32_t index, KeyView key)
    {
        return self().template iter_find_bw_ge<IntList<1>>(index, key);
    }

    auto iter_key() const
    {
        return std::get<0>(self().ctr().template iter_read_leaf_entry<IntList<1>>(self().iter_leaf(), self().iter_local_pos(), 0).get_or_throw());
    }

    auto iter_value() const
    {
        return std::get<0>(self().ctr().template iter_read_leaf_entry<IntList<2>>(self().iter_leaf(), self().iter_local_pos()).get_or_throw());
    }

    virtual Datum<Key> key() const noexcept
    {
        return self().iter_key();
    }

    virtual Datum<Value> value() const noexcept
    {
        using ValueTT = decltype(self().iter_value());
        return map::MapValueHelper<ValueTT>::convert(self().iter_value());
    }

    VoidResult assign(const ValueView& v) noexcept
    {
        return self().ctr().template ctr_update_entry<IntList<0, 2>>(self(), map::ValueBuffer<ValueView>(v));
    }

    bool is_found(const KeyView& k) const noexcept
    {
        auto& self = this->self();
        return (!self.is_end()) && self.key() == k;
    }

MEMORIA_V1_ITERATOR_PART_END

#define M_TYPE      MEMORIA_V1_ITERATOR_TYPE(map::ItrNavName)
#define M_PARAMS    MEMORIA_V1_ITERATOR_TEMPLATE_PARAMS


#undef M_TYPE
#undef M_PARAMS

}
