
// Copyright 2013 Victor Smirnov
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


MEMORIA_V1_ITERATOR_PART_BEGIN(bt::IteratorFindName)



    using typename Base::CtrSizeT;
    using typename Base::Position;
    using typename Base::TreePathT;
    using typename Base::TreeNodePtr;
    using typename Base::TreeNodeConstPtr;
    using typename Base::NodeChain;


    template <typename LeafPath>
    using TargetType = typename Base::Types::template TargetType<LeafPath>;

    template <typename Walker>
    auto iter_find_fw(Walker&& walker)
    {
        auto& self = this->self();

        walker.prepare(self);

        NodeChain node_chain(self.ctr(), self.iter_leaf(), self.iter_local_pos());

        TreePathT end_path = self.path();

        auto result = self.ctr().ctr_find_fw(self.path(), end_path, 0, node_chain, walker);

        if (end_path.leaf() != self.path().leaf())
        {
            self.path() = end_path;
            self.iter_leaf().assign(end_path.leaf());
        }

        self.iter_local_pos()  = result.idx;

        walker.finish(self, result.idx, result.cmd);

        return walker.result();
    }

    template <typename Walker>
    auto iter_find_bw(Walker&& walker)
    {
        auto& self = this->self();

        walker.prepare(self);

        NodeChain node_chain(self.ctr(), self.iter_leaf(), self.iter_local_pos());

        TreePathT end_path = self.path();

        auto result = self.ctr().ctr_find_bw(self.path(), end_path, 0, node_chain, walker);

        if (end_path.leaf() != self.path().leaf())
        {
            self.path() = end_path;
            self.iter_leaf().assign(end_path.leaf());
        }

        self.iter_local_pos() = result.idx;

        walker.finish(self, result.idx, result.cmd);

        return walker.result();
    }



    template <typename LeafPath>
    auto iter_find_fw_gt(int32_t index, TargetType<LeafPath> key)
    {
        MEMORIA_ASSERT(index, >=, 0);
        MEMORIA_ASSERT(key, >=, 0);

        typename Types::template FindGTForwardWalker<Types, LeafPath> walker(index, key);

        return self().iter_find_fw(walker);
    }

    template <typename LeafPath>
    auto iter_find_fw_ge(int32_t index, TargetType<LeafPath> key)
    {
        MEMORIA_ASSERT(index, >=, 0);
        MEMORIA_ASSERT(key, >=, 0);

        typename Types::template FindGEForwardWalker<Types, LeafPath> walker(index, key);

        return self().iter_find_fw(walker);
    }


    template <typename LeafPath>
    auto iter_find_bw_gt(int32_t index, TargetType<LeafPath> key)
    {
        MEMORIA_ASSERT(index, >=, 0);
        MEMORIA_ASSERT(key, >=, 0);

        typename Types::template FindGTBackwardWalker<Types, LeafPath> walker(index, key);

        return self().iter_find_bw(walker);
    }

    template <typename LeafPath>
    auto iter_find_bw_ge(int32_t index, TargetType<LeafPath> key)
    {
        MEMORIA_ASSERT(index, >=, 0);
        MEMORIA_ASSERT(key, >=, 0);

        typename Types::template FindGEBackwardWalker<Types, LeafPath> walker(index, key);

        return self().iter_find_bw(walker);
    }

MEMORIA_V1_ITERATOR_PART_END

#define M_TYPE      MEMORIA_V1_ITERATOR_TYPE(bt::IteratorFindName)
#define M_PARAMS    MEMORIA_V1_ITERATOR_TEMPLATE_PARAMS


#undef M_PARAMS
#undef M_TYPE

}
