
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


MEMORIA_V1_ITERATOR_PART_BEGIN(v1::bt::IteratorLeafName)

    typedef typename Base::NodeBaseG                                                NodeBaseG;
    typedef typename Base::Container                                                Container;

    typedef typename Container::Allocator                                           Allocator;
    typedef typename Container::BranchNodeEntry                                         BranchNodeEntry;
    typedef typename Container::Iterator                                            Iterator;

    using LeafDispatcher = typename Container::Types::Pages::LeafDispatcher;

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

            self.leaf() = next_leaf;

            LeafDispatcher::dispatch(current_leaf, walker, WalkCmd::FIRST_LEAF, 0, 0);

            walker.finish(self, 0, WalkCmd::NONE);

            return true;
        }
        else {
            self.leaf() = current_leaf;
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

            self.leaf() = prev_leaf;

            LeafDispatcher::dispatch(current_leaf, walker, WalkCmd::LAST_LEAF, 0, 0);

            walker.finish(self, 0, WalkCmd::NONE);

            return true;
        }
        else {
            self.leaf() = current_leaf;
            return false;
        }
    }



MEMORIA_V1_ITERATOR_PART_END

#define M_TYPE      MEMORIA_V1_ITERATOR_TYPE(v1::bt::IteratorLeafName)
#define M_PARAMS    MEMORIA_V1_ITERATOR_TEMPLATE_PARAMS


#undef M_PARAMS
#undef M_TYPE

}}