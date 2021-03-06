
// Copyright 2020 Victor Smirnov
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

#include <memoria/prototypes/bt/nodes/branch_node_so.hpp>
#include <memoria/prototypes/bt/nodes/leaf_node_so.hpp>

#include <memoria/containers/allocation_map/allocation_map_names.hpp>
#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>
#include <memoria/core/tools/assert.hpp>


#include <iostream>

namespace memoria {

MEMORIA_V1_ITERATOR_PART_BEGIN(alcmap::ItrApiName)

    using typename Base::CtrSizeT;

    CtrSizeT count_fw()
    {
        typename Types::template CountForwardWalker<Types, IntList<0, 1>> walker(0);
        auto res = self().iter_find_fw(walker);
        return res;
    }



    struct SetClearBitsFn {
        template <typename T>
        Result<CtrSizeT> treeNode(T&& node_so, int32_t start, int32_t level, CtrSizeT size, bool set_bits) const noexcept
        {
            using ResultT = Result<CtrSizeT>;
            auto bitmap = node_so.template substream_by_idx<1>();
            int32_t bm_size = bitmap.data()->size(level);

            int32_t limit = (start + size) < bm_size ? (start + size) : bm_size;

            if (set_bits) {
                MEMORIA_TRY_VOID(bitmap.data()->set_bits(level, start, limit - start));
            }
            else {
                MEMORIA_TRY_VOID(bitmap.data()->clear_bits(level, start, limit - start));
            }

            MEMORIA_TRY_VOID(bitmap.data()->reindex());

            return ResultT::of(limit - start);
        }
    };

    CtrSizeT iter_setup_bits(int32_t level, CtrSizeT size, bool set_bits)
    {
        auto& self = this->self();

        int32_t local_pos = self.iter_local_pos() >> level;

        CtrSizeT accum{};
        CtrSizeT remainder = size;
        while (accum < size)
        {
            self.ctr().ctr_cow_clone_path(self.path(), 0);

            auto processed = self.ctr().leaf_dispatcher().dispatch(self.path().leaf(), SetClearBitsFn(), local_pos, level, remainder, set_bits).get_or_throw();

            accum += processed;

            self.ctr().ctr_update_path(self.path(), 0);

            if (accum < size)
            {
                auto has_next = self.next_leaf();
                if (!has_next) {
                    break;
                }

                self.iter_local_pos() = local_pos = 0;
            }
            else {
                self.iter_local_pos() += processed << level;
            }
        }

        auto leaf_size = self.iter_leaf_size();

        if (leaf_size == self.iter_local_pos())
        {
            self.next_leaf();
        }

        return accum;
    }


    struct TouchBitsFn {
        template <typename T>
        Result<CtrSizeT> treeNode(T&& node_so, int32_t start, int32_t level, CtrSizeT size) const noexcept
        {
            using ResultT = Result<CtrSizeT>;
            auto bitmap = node_so.template substream_by_idx<1>();
            int32_t bm_size = bitmap.data()->size(level);

            int32_t limit = (start + size) < bm_size ? (start + size) : bm_size;

            return ResultT::of(limit - start);
        }
    };


    CtrSizeT iter_touch_bits(int32_t level, CtrSizeT size)
    {
        auto& self = this->self();

        int32_t local_pos = self.iter_local_pos() >> level;

        CtrSizeT accum{};
        CtrSizeT remainder = size;
        while (accum < size)
        {
            self.ctr().ctr_cow_clone_path(self.path(), 0);

            auto processed = self.ctr().leaf_dispatcher().dispatch(self.path().leaf(), TouchBitsFn(), local_pos, level, remainder).get_or_throw();

            accum += processed;

            if (accum < size)
            {
                auto has_next = self.next_leaf();
                if (!has_next) {
                    break;
                }

                self.iter_local_pos() = local_pos = 0;
            }
            else {
                self.iter_local_pos() += processed << level;
            }
        }

        auto leaf_size = self.iter_leaf_size();

        if (leaf_size == self.iter_local_pos())
        {
            self.next_leaf();
        }

        return accum;
    }



    struct PosWalker {
        CtrSizeT prefix_{};

        template <typename CtrT, typename NodeT>
        VoidResult treeNode(const BranchNodeSO<CtrT, NodeT>& node, WalkCmd cmd, int32_t start, int32_t end) noexcept
        {
            auto stream = node.template substream<IntList<0>>();
            prefix_ += stream.sum(0, start, end);

            return VoidResult::of();
        }


        template <typename CtrT, typename NodeT>
        VoidResult treeNode(const LeafNodeSO<CtrT, NodeT>& node, WalkCmd cmd, int32_t start, int32_t end) noexcept {
            prefix_ += end - start;
            return VoidResult::of();
        }
    };


    CtrSizeT level0_pos() const
    {
        auto& self = this->self();

        PosWalker walker{};
        self.iter_walk_up_for_refresh(self.path(), 0, self.iter_local_pos(), walker);

        return walker.prefix_;
    }


    struct GetBitsFn {
        template <typename T>
        Int32Result treeNode(T&& node_so, int32_t level, int32_t pos) const noexcept
        {
            auto bitmap = node_so.template substream_by_idx<1>();
            int32_t bm_size = bitmap.data()->size(level);
            (void)bm_size;
            MEMORIA_ASSERT_RTN(pos, <, bm_size);
            MEMORIA_ASSERT_RTN(pos, >=, 0);

            return Int32Result::of(bitmap.data()->get_bit(level, pos));
        }
    };

    int32_t iter_get_bit(int32_t level, int32_t pos) const
    {
        auto& self = this->self();
        return self.ctr().leaf_dispatcher().dispatch(self.leaf(), GetBitsFn(), level, pos).get_or_throw();
    }

    int32_t iter_get_bit(int32_t level) const
    {
        auto& self = this->self();
        return self.ctr().leaf_dispatcher().dispatch(self.path().leaf(), GetBitsFn(), level, self.iter_local_pos() >> level).get_or_throw();
    }

MEMORIA_V1_ITERATOR_PART_END

#define M_TYPE      MEMORIA_V1_ITERATOR_TYPE(alcmap::ItrApiName)
#define M_PARAMS    MEMORIA_V1_ITERATOR_TEMPLATE_PARAMS



#undef M_TYPE
#undef M_PARAMS

}
