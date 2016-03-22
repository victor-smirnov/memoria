
// Copyright Victor Smirnov 2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/containers/map/map_names.hpp>
#include <memoria/core/types/types.hpp>

#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <iostream>

namespace memoria    {

using bt::StreamTag;

MEMORIA_ITERATOR_PART_BEGIN(memoria::map::ItrNavMaxName)

    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::NodeBaseG                                            NodeBaseG;

    typedef typename Base::Container::Value                                     Value;
    typedef typename Base::Container::Key                                       Key;
    typedef typename Base::Container::BranchNodeEntry                           BranchNodeEntry;
    typedef typename Base::Container                                            Container;
    typedef typename Base::Container::Position                                  Position;

    using CtrSizeT = typename Container::Types::CtrSizeT;

public:
    void insert(const Key& key, const Value& value)
    {
        auto& self = this->self();

        self.ctr().insert_entry(
                self,
                map::KeyValueEntry<Key, Value, CtrSizeT>(key, value)
        );


        self.skipFw(1);
    }

    template <typename InputIterator>
    auto bulk_insert(InputIterator&& start, InputIterator&& end, Int ib_capacity = 10000)
    {
        using InputProvider = map::MapEntryIteratorInputProvider<Container, InputIterator>;

        auto bulk = std::make_unique<InputProvider>(self().ctr(), start, end, ib_capacity);

        return Base::bulk_insert(*bulk.get());
    }


    void remove()
    {
        auto& self = this->self();

        self.ctr().removeEntry(self);

        if (self.isEnd())
        {
            self.skipFw(0);
        }
    }

    auto remove(CtrSizeT size)
    {
        return Base::remove(size);
    }


    auto findFwGT(Int index, Key key)
    {
        return self().template find_fw_gt<IntList<1>>(index, key);
    }

    auto findFwGE(Int index, Key key)
    {
        return self().template find_fw_ge<IntList<1>>(index, key);
    }

    auto findBwGT(Int index, Key key)
    {
        return self().template find_bw_gt<IntList<1>>(index, key);
    }

    auto findBwGE(Int index, Key key)
    {
        return self().template find_bw_ge<IntList<1>>(index, key);
    }

    auto key() const -> Key
    {
        return std::get<0>(self().ctr().template read_leaf_entry<IntList<1>>(self().leaf(), self().idx(), 0));
    }

    auto value() const
    {
        return std::get<0>(self().ctr().template read_leaf_entry<IntList<2>>(self().leaf(), self().idx()));
    }

    void assign(const Value& v)
    {
        self().ctr().template update_entry<IntList<2>>(self(), map::ValueBuffer<Value>(v));
    }

    bool is_found(const Key& k) const
    {
        auto& self = this->self();
        return (!self.is_end()) && self.key() == k;
    }

    template <typename Iterator>
    class EntryAdaptor {
        Iterator& current_;

        Key key_;
        Value value_;

    public:
        EntryAdaptor(Iterator& current): current_(current) {}

        template <typename V>
        void put(StreamTag<0>, StreamTag<0>, Int block, V&& entry) {}

        template <typename V>
        void put(StreamTag<0>, StreamTag<1>, Int block, V&& key) {
            key_ = key;
        }

        template <typename V>
        void put(StreamTag<0>, StreamTag<2>, Int block, V&& value) {
            value_ = value;
        }

        void next()
        {
            current_ = std::make_pair(key_, value_);
            current_++;
        }
    };




    template <typename OutputIterator>
    auto read(OutputIterator& iter, CtrSizeT length)
    {
        auto& self = this->self();

        EntryAdaptor<OutputIterator> adaptor(iter);

        return self.ctr().template read_entries<0>(self, length, adaptor);
    }

    template <typename OutputIterator>
    auto read(OutputIterator& iter)
    {
        auto& self = this->self();

        return read(iter, self.ctr().size());
    }

MEMORIA_ITERATOR_PART_END

#define M_TYPE      MEMORIA_ITERATOR_TYPE(memoria::map::ItrNavName)
#define M_PARAMS    MEMORIA_ITERATOR_TEMPLATE_PARAMS




}

#undef M_TYPE
#undef M_PARAMS
