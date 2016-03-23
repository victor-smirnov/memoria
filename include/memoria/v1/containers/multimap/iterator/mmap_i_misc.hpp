
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#pragma once

#include <memoria/v1/core/types/types.hpp>

#include <memoria/v1/containers/multimap/mmap_names.hpp>
#include <memoria/v1/core/container/iterator.hpp>
#include <memoria/v1/core/container/macros.hpp>

#include <iostream>

namespace memoria {


MEMORIA_ITERATOR_PART_BEGIN(memoria::mmap::ItrMiscName)

    using typename Base::NodeBaseG;
    using typename Base::Container;
    using typename Base::BranchNodeEntry;
    using typename Base::CtrSizeT;
    using typename Base::CtrSizesT;


    using Key       = typename Container::Types::Key;
    using Value     = typename Container::Types::Value;

    using LeafDispatcher = typename Container::Types::Pages::LeafDispatcher;

public:
    Key key() const
    {
        auto& self = this->self();

        MEMORIA_ASSERT(self.stream(), ==, 0);

        return std::get<0>(self.template read_leaf_entry<0, IntList<1>>(self.idx(), 0));
    }

    Value value() const
    {
        auto& self = this->self();

        MEMORIA_ASSERT(self.stream(), ==, 1);

        return std::get<0>(self.template read_leaf_entry<1, IntList<1>>(self.idx(), 0));
    }

    std::vector<Value> read_values(CtrSizeT length = -1)
    {
        auto& self = this->self();

        std::vector<Value> values;

        self.scan_values(length, [&](auto&& value){
            values.push_back(value);
        });

        return values;
    }


    template <typename Fn>
    CtrSizeT scan_values(CtrSizeT length, Fn&& fn)
    {
        auto& self = this->self();

        if (self.stream() == 0) {
            self.toData();
        }

        auto data_size  = self.cache().data_size()[1];
        auto pos        = self.cache().data_pos()[1];

        if (length < 0 || pos + length > data_size)
        {
            length = data_size - pos;
        }

        SubstreamReadLambdaAdapter<Fn> adapter(fn);

        return self.ctr().template read_substream<IntList<1, 1>>(self, 0, length, adapter);
    }

    template <typename Fn>
    CtrSizeT scan_values(Fn&& fn)
    {
        auto& self = this->self();
        return self.scan_values(-1, std::forward<Fn>(fn));
    }

    template <typename Fn>
    CtrSizeT scan_keys(Fn&& fn)
    {
        auto& self = this->self();
        return self.scan_keys(-1, std::forward<Fn>(fn));
    }

    template <typename Fn>
    CtrSizeT scan_keys(CtrSizeT length, Fn&& fn)
    {
        auto& self = this->self();

        if (self.stream() == 0) {
            self.toIndex();
        }

        auto data_size  = self.cache().data_size()[0];
        auto pos        = self.cache().data_pos()[0];

        if (length < 0 || pos + length > data_size)
        {
            length = data_size - pos;
        }

        SubstreamReadLambdaAdapter<Fn> adapter(fn);

        return self.ctr().template read_substream<IntList<0, 0, 1>>(self, 0, length, adapter);
    }


    CtrSizesT remove(CtrSizeT length = 1)
    {
        return self().remove_subtrees(length);
    }

    CtrSizeT values_size() const {
        return self().cache().substream_size();
    }


    template <typename Provider>
    auto bulk_insert(Provider&& provider, const Int total_capacity = 2000)
    {
        auto& self = this->self();

        return self.ctr()._insert(self, std::forward<Provider>(provider), total_capacity);
    }


MEMORIA_ITERATOR_PART_END

#define M_TYPE      MEMORIA_ITERATOR_TYPE(memoria::mmap::ItrMiscName)
#define M_PARAMS    MEMORIA_ITERATOR_TEMPLATE_PARAMS


#undef M_TYPE
#undef M_PARAMS

}
