
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

#include <memoria/prototypes/bt/bt_names.hpp>

#include <memoria/core/types.hpp>
#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <iostream>

namespace memoria {

MEMORIA_V1_ITERATOR_PART_BEGIN(bt::IteratorRankName)

    using typename Base::CtrSizeT;

    template <typename LeafPath>
    auto iter_rank_fw(int32_t index, CtrSizeT pos)
    {
        MEMORIA_ASSERT(pos, >=, 0);
        MEMORIA_ASSERT(index, >=, 0);

        typename Types::template RankForwardWalker<Types, LeafPath> walker(index, pos);

        return self().iter_find_fw(walker);
    }

    template <typename LeafPath>
    auto iter_rank_bw(int32_t index, CtrSizeT pos)
    {
        MEMORIA_ASSERT(pos, >=, 0);
        MEMORIA_ASSERT(index, >=, 0);

        typename Types::template RankBackwardWalker<Types, LeafPath> walker(index, pos);

        return self().iter_find_bw(walker);
    }

    template <typename LeafPath>
    auto iter_rank(int32_t index, CtrSizeT pos)
    {
        if (pos >= 0)
        {
            return self().template iter_rank_fw<LeafPath>(index, pos);
        }
        else {
            return self().template iter_rank_bw<LeafPath>(index, -pos);
        }
    }


MEMORIA_V1_ITERATOR_PART_END

#define M_TYPE      MEMORIA_V1_ITERATOR_TYPE(bt::IteratorRankName)
#define M_PARAMS    MEMORIA_V1_ITERATOR_TEMPLATE_PARAMS





#undef M_PARAMS
#undef M_TYPE

}
