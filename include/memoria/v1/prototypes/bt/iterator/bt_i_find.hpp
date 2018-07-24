
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

#include <memoria/v1/prototypes/bt/bt_names.hpp>

#include <memoria/v1/core/types.hpp>
#include <memoria/v1/core/container/iterator.hpp>
#include <memoria/v1/core/container/macros.hpp>

#include <iostream>

namespace memoria {
namespace v1 {


MEMORIA_V1_ITERATOR_PART_BEGIN(bt::IteratorFindName)

    typedef typename Base::NodeBaseG                                                NodeBaseG;
    typedef typename Base::Container                                                Container;

    typedef typename Container::Allocator                                           Allocator;
    typedef typename Container::BranchNodeEntry                                         BranchNodeEntry;
    typedef typename Container::Iterator                                            Iterator;

    using CtrSizeT = typename Container::Types::CtrSizeT;

    template <typename LeafPath>
    using TargetType = typename Container::Types::template TargetType<LeafPath>;

    template <typename Walker>
    auto find_fw(Walker&& walker)
    {
        auto& self = this->self();

        walker.prepare(self);

        typename Container::NodeChain node_chain(self.leaf(), self.key_idx());

        auto result = self.ctr().find_fw(node_chain, walker);

        self.leaf() = result.node;
        self.idx()  = result.idx;

        walker.finish(self, result.idx, result.cmd);

        return walker.result();
    }

    template <typename Walker>
    auto find_bw(Walker&& walker)
    {
        auto& self = this->self();

        walker.prepare(self);

        typename Container::NodeChain node_chain(self.leaf(), self.key_idx());

        auto result = self.ctr().find_bw(node_chain, walker);

        self.leaf() = result.node;
        self.idx()  = result.idx;

        walker.finish(self, result.idx, result.cmd);

        return walker.result();
    }



    template <typename LeafPath>
    auto find_fw_gt(int32_t index, TargetType<LeafPath> key)
    {
        MEMORIA_V1_ASSERT(index, >=, 0);
        MEMORIA_V1_ASSERT(key, >=, 0);

        typename Types::template FindGTForwardWalker<Types, LeafPath> walker(index, key);

        return self().find_fw(walker);
    }

    template <typename LeafPath>
    auto find_fw_ge(int32_t index, TargetType<LeafPath> key)
    {
        MEMORIA_V1_ASSERT(index, >=, 0);
        MEMORIA_V1_ASSERT(key, >=, 0);

        typename Types::template FindGEForwardWalker<Types, LeafPath> walker(index, key);

        return self().find_fw(walker);
    }


    template <typename LeafPath>
    auto find_bw_gt(int32_t index, TargetType<LeafPath> key)
    {
        MEMORIA_V1_ASSERT(index, >=, 0);
        MEMORIA_V1_ASSERT(key, >=, 0);

        typename Types::template FindGTBackwardWalker<Types, LeafPath> walker(index, key);

        return self().find_bw(walker);
    }

    template <typename LeafPath>
    auto find_bw_ge(int32_t index, TargetType<LeafPath> key)
    {
        MEMORIA_V1_ASSERT(index, >=, 0);
        MEMORIA_V1_ASSERT(key, >=, 0);

        typename Types::template FindGEBackwardWalker<Types, LeafPath> walker(index, key);

        return self().find_bw(walker);
    }

MEMORIA_V1_ITERATOR_PART_END

#define M_TYPE      MEMORIA_V1_ITERATOR_TYPE(bt::IteratorFindName)
#define M_PARAMS    MEMORIA_V1_ITERATOR_TEMPLATE_PARAMS


#undef M_PARAMS
#undef M_TYPE

}}
