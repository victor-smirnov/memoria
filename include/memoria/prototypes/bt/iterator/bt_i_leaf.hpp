
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

MEMORIA_V1_ITERATOR_PART_BEGIN(bt::IteratorLeafName)

    typedef typename Base::NodeBaseG                                                NodeBaseG;
    typedef typename Base::Container                                                Container;

    typedef typename Container::Allocator                                           Allocator;
    typedef typename Container::BranchNodeEntry                                         BranchNodeEntry;
    typedef typename Container::Iterator                                            Iterator;

    using CtrSizeT = typename Container::Types::CtrSizeT;

public:
    BoolResult iter_next_leaf() noexcept
    {
        auto& self = this->self();

        auto current_leaf = self.iter_leaf();
        MEMORIA_TRY(has_next_leaf, self.ctr().ctr_get_next_node(self.path(), 0));

        if (has_next_leaf)
        {
            typename Types::template NextLeafWalker<Types> walker;

            walker.prepare(self);

            // FIXME need to refresh iterator's iovector view
            self.ctr().leaf_dispatcher().dispatch(current_leaf, walker, WalkCmd::FIRST_LEAF, 0, 0);

            walker.finish(self, 0, WalkCmd::NONE);

            return BoolResult::of(true);
        }
        else {
            return BoolResult::of(false);
        }
    }


    BoolResult iter_prev_leaf() noexcept
    {
        auto& self = this->self();

        auto current_leaf = self.iter_leaf();
        MEMORIA_TRY(has_prev_leaf, self.ctr().ctr_get_prev_node(self.path(), 0));

        if (has_prev_leaf)
        {
            typename Types::template PrevLeafWalker<Types> walker;

            walker.prepare(self);

            // FIXME need to refresh iterator's iovector view
            self().leaf_dispatcher().dispatch(current_leaf, walker, WalkCmd::LAST_LEAF, 0, 0);

            walker.finish(self, 0, WalkCmd::NONE);

            return BoolResult::of(true);
        }
        else {
            return BoolResult::of(false);
        }
    }



MEMORIA_V1_ITERATOR_PART_END

#define M_TYPE      MEMORIA_V1_ITERATOR_TYPE(bt::IteratorLeafName)
#define M_PARAMS    MEMORIA_V1_ITERATOR_TEMPLATE_PARAMS


#undef M_PARAMS
#undef M_TYPE

}
