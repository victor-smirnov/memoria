
// Copyright Victor Smirnov 2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/prototypes/bt/bt_names.hpp>

#include <memoria/v1/core/types/types.hpp>
#include <memoria/v1/core/container/iterator.hpp>
#include <memoria/v1/core/container/macros.hpp>

#include <iostream>

namespace memoria {
namespace v1 {


MEMORIA_V1_ITERATOR_PART_BEGIN(v1::bt::IteratorSkipName)

    typedef typename Base::NodeBaseG                                                NodeBaseG;
    typedef typename Base::Container                                                Container;

    typedef typename Container::Allocator                                           Allocator;
    typedef typename Container::BranchNodeEntry                                         BranchNodeEntry;
    typedef typename Container::Iterator                                            Iterator;

    using CtrSizeT = typename Container::Types::CtrSizeT;



    template <Int Stream>
    auto skip_fw_(CtrSizeT amount)
    {
        if (amount < 0) {
            int a = 0; a++;
        }

        MEMORIA_V1_ASSERT(amount, >=, 0);

        typename Types::template SkipForwardWalker<Types, IntList<Stream>> walker(amount);

        return self().find_fw(walker);
    }

    template <Int Stream>
    auto skip_bw_(CtrSizeT amount)
    {
        MEMORIA_V1_ASSERT(amount, >=, 0);

        typename Types::template SkipBackwardWalker<Types, IntList<Stream>> walker(amount);

        return self().find_bw(walker);
    }

    template <Int Stream>
    auto skip_(CtrSizeT amount)
    {
        auto& self = this->self();

        if (amount >= 0)
        {
            return self.template skip_fw_<Stream>(amount);
        }
        else {
            return self.template skip_bw_<Stream>(-amount);
        }
    }

MEMORIA_V1_ITERATOR_PART_END

#define M_TYPE      MEMORIA_V1_ITERATOR_TYPE(v1::bt::IteratorSkipName)
#define M_PARAMS    MEMORIA_V1_ITERATOR_TEMPLATE_PARAMS




#undef M_PARAMS
#undef M_TYPE

}}