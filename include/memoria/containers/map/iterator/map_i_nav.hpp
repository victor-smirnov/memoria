
// Copyright 2014 Victor Smirnov
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
#include <memoria/core/types.hpp>

#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <iostream>

namespace memoria {

MEMORIA_V1_ITERATOR_PART_BEGIN(map::ItrNavName)
public:
    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::TreeNodePtr                                            TreeNodePtr;

    typedef typename Base::Container::Value                                     Value;
    typedef typename Base::Container::Key                                       Key;
    typedef typename Base::Container::BranchNodeEntry                           BranchNodeEntry;
    typedef typename Base::Container                                            Container;
    typedef typename Base::Container::Position                                  Position;

    using CtrSizeT = typename Container::Types::CtrSizeT;

    template <int32_t Stream>
    using InputTupleAdapter = typename Container::Types::template InputTupleAdapter<Stream>;


    void iter_insert_entry(Key key, Value value)
    {
        auto& self = this->self();

        auto delta = key - self.prefix();

        //MEMORIA_V1_ASSERT_TRUE(delta > 0);

        self.ctr().iter_insert_entry(
                self,
                InputTupleAdapter<0>::convert(0, core::StaticVector<Key, 1>({delta}), core::StaticVector<Value, 1>{value})
        );

        self.iter_skip_fw(1);

        if (!self.iter_is_end())
        {
            auto k = self.iter_raw_key();

            //MEMORIA_V1_ASSERT_TRUE((k - StaticVector<int64_t, 1>(delta))[0] >= 0);

            return self.ctr().template ctr_update_entry<IntList<0, 1>>(self, std::make_tuple(k - StaticVector<int64_t, 1>(delta)));
        }
    }

    void iter_remove_entry()
    {
        auto& self = this->self();

        auto k = self.iter_raw_key();

        self.ctr().ctr_remove_entry(self);

        if (self.iter_is_end())
        {
            self.iter_skip_fw(0);
        }

        if (!self.iter_is_end())
        {
            auto kk = self.iter_raw_key();
            return self.ctr().template ctr_update_entry<IntList<0, 0>>(self, std::make_tuple(k + kk));
        }
    }



    auto iter_map_find_fw_gt(int32_t index, Key key)
    {
        return self().template iter_find_fw_gt<IntList<0>>(index, key);
    }

    auto iter_map_find_fw_ge(int32_t index, Key key)
    {
        return self().template iter_find_fw_ge<IntList<0>>(index, key);
    }

    auto iter_map_find_bw_gt(int32_t index, Key key)
    {
        return self().template iter_find_bw_gt<IntList<0>>(index, key);
    }

    auto iter_map_find_bw_ge(int32_t index, Key key)
    {
        return self().template iter_find_bw_ge<IntList<0>>(index, key);
    }

    Key iter_map_prefix() const
    {
        auto& self = this->self();
        auto& iter_cache = self.iter_cache();

        auto leaf_sum = self.ctr().template ctr_leaf_sums<IntList<0, 0, 1>>(self.iter_leaf(), 0, 0, self.iter_local_pos());

        return bt::Path<0, 0>::get(iter_cache.prefixes())[0] + leaf_sum;
    }


    auto iter_raw_key() const
    {
        return std::get<0>(self().ctr().template iter_read_leaf_entry<IntList<1>>(self().iter_leaf(), self().iter_local_pos()));
    }

    auto key() const -> Key
    {
        return self().iter_raw_key(0) + self().prefix();
    }

    auto iter_raw_key(int32_t index) const
    {
        return std::get<0>(self().ctr().template iter_read_leaf_entry<IntList<1>>(self().iter_leaf(), self().iter_local_pos(), index));
    }

    auto value() const
    {
        return std::get<0>(self().ctr().template iter_read_leaf_entry<IntList<2>>(self().iter_leaf(), self().iter_local_pos()));
    }

    void assign(const Value& v)
    {
        self().ctr().template ctr_update_entry<IntList<0, 1>>(self(), std::make_tuple(v));
    }

    bool is_found(const Key& k) const noexcept
    {
        auto& self = this->self();
        return (!self.iter_is_end()) && self.key() == k;
    }

MEMORIA_V1_ITERATOR_PART_END

#define M_TYPE      MEMORIA_V1_ITERATOR_TYPE(map::ItrNavName)
#define M_PARAMS    MEMORIA_V1_ITERATOR_TEMPLATE_PARAMS



#undef M_TYPE
#undef M_PARAMS

}
