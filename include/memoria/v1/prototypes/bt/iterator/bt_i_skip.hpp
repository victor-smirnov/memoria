
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

#include <memoria/v1/prototypes/bt/bt_names.hpp>

#include <memoria/v1/core/types.hpp>
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



    template <int32_t Stream>
    auto skip_fw_(CtrSizeT amount)
    {
        MEMORIA_V1_ASSERT(amount, >=, 0);

        typename Types::template SkipForwardWalker<Types, IntList<Stream>> walker(amount);

        return self().find_fw(walker);
    }

    template <int32_t Stream>
    auto skip_bw_(CtrSizeT amount)
    {
        MEMORIA_V1_ASSERT(amount, >=, 0);

        typename Types::template SkipBackwardWalker<Types, IntList<Stream>> walker(amount);

        return self().find_bw(walker);
    }

    template <int32_t Stream>
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
