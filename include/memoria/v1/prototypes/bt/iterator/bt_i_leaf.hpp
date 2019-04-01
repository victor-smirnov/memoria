
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


MEMORIA_V1_ITERATOR_PART_BEGIN(bt::IteratorLeafName)

    typedef typename Base::NodeBaseG                                                NodeBaseG;
    typedef typename Base::Container                                                Container;

    typedef typename Container::Allocator                                           Allocator;
    typedef typename Container::BranchNodeEntry                                         BranchNodeEntry;
    typedef typename Container::Iterator                                            Iterator;

    using LeafDispatcher = typename Container::Types::Blocks::LeafDispatcher;

    using CtrSizeT = typename Container::Types::CtrSizeT;

public:
    bool nextLeaf()
    {
        auto& self = this->self();

        auto current_leaf = self.leaf();
        auto next_leaf    = self.ctr().getNextNodeP(self.leaf());

        if (next_leaf.isSet())
        {
            typename Types::template NextLeafWalker<Types> walker;

            walker.prepare(self);

            self.leaf().assign(next_leaf);

            LeafDispatcher::dispatch(current_leaf, walker, WalkCmd::FIRST_LEAF, 0, 0);

            walker.finish(self, 0, WalkCmd::NONE);

            return true;
        }
        else {
            self.leaf().assign(current_leaf);
            return false;
        }
    }


    bool prevLeaf()
    {
        auto& self = this->self();

        auto current_leaf = self.leaf();
        auto prev_leaf    = self.ctr().getPrevNodeP(self.leaf());

        if (prev_leaf.isSet())
        {
            typename Types::template PrevLeafWalker<Types> walker;

            walker.prepare(self);

            self.leaf().assign(prev_leaf);

            LeafDispatcher::dispatch(current_leaf, walker, WalkCmd::LAST_LEAF, 0, 0);

            walker.finish(self, 0, WalkCmd::NONE);

            return true;
        }
        else {
            self.leaf().assign(current_leaf);
            return false;
        }
    }



MEMORIA_V1_ITERATOR_PART_END

#define M_TYPE      MEMORIA_V1_ITERATOR_TYPE(bt::IteratorLeafName)
#define M_PARAMS    MEMORIA_V1_ITERATOR_TEMPLATE_PARAMS


#undef M_PARAMS
#undef M_TYPE

}}
