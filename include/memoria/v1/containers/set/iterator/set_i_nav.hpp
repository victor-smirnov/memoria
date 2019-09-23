
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

#include <memoria/v1/containers/set/set_names.hpp>
#include <memoria/v1/core/types.hpp>

#include <memoria/v1/core/container/iterator.hpp>
#include <memoria/v1/core/container/macros.hpp>

#include <iostream>

namespace memoria {
namespace v1 {

using bt::StreamTag;

MEMORIA_V1_ITERATOR_PART_BEGIN(set::ItrNavName)

    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::NodeBaseG                                            NodeBaseG;


    typedef typename Base::Container::Types::Key                                Key;
    typedef typename Base::Container::Types::BranchNodeEntry                    BranchNodeEntry;
    typedef typename Base::Container                                            Container;
    typedef typename Base::Container::Position                                  Position;

    using CtrSizeT = typename Container::Types::CtrSizeT;

    using KeyView   = typename DataTypeTraits<Key>::ViewType;
    using KeyV      = typename DataTypeTraits<Key>::ValueType;

    using Profile   = typename Types::Profile;

public:
    void insert(const KeyView& key)
    {
        auto& self = this->self();

        self.ctr().iter_insert_entry(
                self,
                set::KeyEntry<Key, CtrSizeT>(key)
        );

        self.iter_skip_fw(1);
    }


    void remove()
    {
        auto& self = this->self();

        self.ctr().ctr_remove_entry(self);

        if (self.iter_is_end())
        {
            self.iter_btss_skip_fw(0);
        }
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

    auto key() const -> KeyV
    {
        return std::get<0>(self().ctr().template iter_read_leaf_entry<IntList<1>>(self().iter_leaf(), self().iter_local_pos(), 0));
    }

    bool is_found(const KeyView& k) const
    {
        auto& self = this->self();
        return (!self.is_end()) && self.key() == k;
    }

    /*
    template <typename Iterator>
    class EntryAdaptor {
        Iterator& current_;

        KeyV key_;


    public:
        EntryAdaptor(Iterator& current): current_(current) {}

        template <typename V>
        void put(StreamTag<0>, StreamTag<0>, int32_t block, V&& entry) {}

        template <typename V>
        void put(StreamTag<0>, StreamTag<1>, int32_t block, V&& key) {
            key_ = key;
        }

        void next()
        {
            current_ = key_;
            current_++;
        }
    };




    template <typename OutputIterator>
    auto read(OutputIterator& iter, CtrSizeT length)
    {
        auto& self = this->self();

        EntryAdaptor<OutputIterator> adaptor(iter);

        return self.ctr().template ctr_read_entries<0>(self, length, adaptor);
    }

    template <typename OutputIterator>
    auto read(OutputIterator& iter)
    {
        auto& self = this->self();

        return read(iter, self.ctr().size());
    }*/

MEMORIA_V1_ITERATOR_PART_END

#define M_TYPE      MEMORIA_V1_ITERATOR_TYPE(set::ItrNavName)
#define M_PARAMS    MEMORIA_V1_ITERATOR_TEMPLATE_PARAMS


#undef M_TYPE
#undef M_PARAMS

}}
