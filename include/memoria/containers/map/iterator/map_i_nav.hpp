
// Copyright Victor Smirnov 2014+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_MAPX_ITER_NAV_HPP
#define _MEMORIA_CONTAINERS_MAPX_ITER_NAV_HPP

#include <memoria/containers/map/map_names.hpp>
#include <memoria/core/types/types.hpp>

#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <iostream>

namespace memoria    {


MEMORIA_ITERATOR_PART_BEGIN(memoria::map::ItrNavName)

    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::NodeBaseG                                            NodeBaseG;

    typedef typename Base::Container::Value                                     Value;
    typedef typename Base::Container::Key                                       Key;
    typedef typename Base::Container::BranchNodeEntry                               BranchNodeEntry;
    typedef typename Base::Container                                            Container;
    typedef typename Base::Container::Position                                  Position;

    using CtrSizeT = typename Container::Types::CtrSizeT;

    template <Int Stream>
    using InputTupleAdapter = typename Container::Types::template InputTupleAdapter<Stream>;


    void insert_(Key key, Value value)
    {
        auto& self = this->self();

        auto delta = key - self.prefix();

        MEMORIA_ASSERT_TRUE(delta > 0);

        self.ctr().insert_entry(
                self,
                InputTupleAdapter<0>::convert(0, core::StaticVector<Key, 1>({delta}), core::StaticVector<Value, 1>{value})
        );

        self.skipFw(1);

        if (!self.isEnd())
        {
            auto k = self.raw_key();

            MEMORIA_ASSERT_TRUE((k - StaticVector<BigInt, 1>(delta))[0] >= 0);

            self.ctr().template update_entry<IntList<1>>(self, std::make_tuple(k - StaticVector<BigInt, 1>(delta)));
        }
    }

    template <typename InputIterator>
    void insert(InputIterator&&, InputIterator&&) {}

    template <typename Provider>
    void insert(Provider&&) {}

    void remove() {
        auto& self = this->self();

        auto k = self.raw_key();

        self.ctr().removeEntry(self);

        if (self.isEnd())
        {
            self.skipFw(0);
        }

        if (!self.isEnd()) {

            auto kk = self.raw_key();

            self.ctr().template update_entry<IntList<0>>(self, std::make_tuple(k + kk));
        }
    }



    auto findFwGT(Int index, Key key)
    {
        return self().template find_fw_gt<IntList<0>>(index, key);
    }

    auto findFwGE(Int index, Key key)
    {
        return self().template find_fw_ge<IntList<0>>(index, key);
    }

    auto findBwGT(Int index, Key key)
    {
        return self().template find_bw_gt<IntList<0>>(index, key);
    }

    auto findBwGE(Int index, Key key)
    {
        return self().template find_bw_ge<IntList<0>>(index, key);
    }

    Key prefix() const
    {
        auto& self = this->self();
        auto& cache = self.cache();

        auto leaf_sum = self.ctr().template leaf_sums<IntList<0, 0, 1>>(self.leaf(), 0, 0, self.idx());

        return bt::Path<0, 0>::get(cache.prefixes())[0] + leaf_sum;
    }


    auto raw_key() const
    {
        return std::get<0>(self().ctr().template read_leaf_entry<IntList<1>>(self().leaf(), self().idx()));
    }

    auto key() const -> Key
    {
        return self().raw_key(0) + self().prefix();
    }

    auto raw_key(Int index) const
    {
        return std::get<0>(self().ctr().template read_leaf_entry<IntList<1>>(self().leaf(), self().idx(), index));
    }

    auto value() const
    {
        return std::get<0>(self().ctr().template read_leaf_entry<IntList<2>>(self().leaf(), self().idx()));
    }

    void setValue(const Value& v)
    {
        self().ctr().template update_entry<IntList<1>>(self(), std::make_tuple(v));
    }

    bool isFound(const Key& k) const
    {
        auto& self = this->self();

        return (!self.isEnd()) && self.key() == k;
    }

MEMORIA_ITERATOR_PART_END

#define M_TYPE      MEMORIA_ITERATOR_TYPE(memoria::map::ItrNavName)
#define M_PARAMS    MEMORIA_ITERATOR_TEMPLATE_PARAMS




}

#undef M_TYPE
#undef M_PARAMS


#endif
