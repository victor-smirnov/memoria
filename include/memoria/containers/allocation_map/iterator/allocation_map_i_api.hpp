
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

    using ProfileT = typename Base::Container::Types::Profile;
    using typename Base::CtrSizeT;

    using ApiProfileT = ApiProfile<ProfileT>;
    using AllocationMetadataT = AllocationMetadata<ApiProfileT>;


    CtrSizeT count_fw()
    {
        typename Types::template CountForwardWalker<Types, IntList<0, 1>> walker(0);
        auto res = self().iter_find_fw(walker);
        return res;
    }



    struct SetClearBitsFn {
        template <typename T>
        CtrSizeT treeNode(T&& node_so, size_t start, size_t level, CtrSizeT size, bool set_bits) const noexcept
        {
            auto bitmap = node_so.template substream_by_idx<1>();
            size_t bm_size = bitmap.data()->size(level);
            size_t limit = (start + size) < bm_size ? (start + size) : bm_size;

            if (set_bits) {
                bitmap.data()->set_bits(level, start, limit - start);
            }
            else {
                bitmap.data()->clear_bits(level, start, limit - start);
            }

            bitmap.data()->reindex();

            return CtrSizeT(limit - start);
        }
    };

    CtrSizeT iter_setup_bits(size_t level, CtrSizeT size, bool set_bits)
    {
        auto& self = this->self();

        size_t local_pos = self.iter_local_pos() >> level;

        CtrSizeT accum{};

        while (accum < size)
        {
            self.ctr().ctr_cow_clone_path(self.path(), 0);

            CtrSizeT remainder = size - accum;
            auto processed = self.ctr().leaf_dispatcher().dispatch(self.path().leaf(), SetClearBitsFn(), local_pos, level, remainder, set_bits);

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
        CtrSizeT treeNode(T&& node_so, int32_t start, int32_t level, CtrSizeT size) const noexcept
        {
            auto bitmap = node_so.template substream_by_idx<1>();
            size_t bm_size = bitmap.data()->size(level);
            size_t limit = (start + size) < bm_size ? (start + size) : bm_size;
            return limit - start;
        }
    };


    CtrSizeT iter_touch_bits(int32_t level, CtrSizeT size)
    {
        auto& self = this->self();

        size_t local_pos = self.iter_local_pos() >> level;

        CtrSizeT accum{};
        CtrSizeT remainder = size;
        while (accum < size)
        {
            self.ctr().ctr_cow_clone_path(self.path(), 0);

            auto processed = self.ctr().leaf_dispatcher().dispatch(self.path().leaf(), TouchBitsFn(), local_pos, level, remainder);

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

    void iter_clone_path()
    {
        auto& self = the_self();
        self.ctr().ctr_cow_clone_path(self.path(), 0);
    }

    struct PopulateLeafFn {
        template <typename T>
        void treeNode(T&& node_so, Span<const AllocationMetadataT> leaf_allocations, bool set_bits, CtrSizeT base) const noexcept
        {
            auto bitmap = node_so.template substream_by_idx<1>();

            if (set_bits) {
                for (auto alc: leaf_allocations) {
                    int32_t pos = (alc.position() - base) >> alc.level();
                    bitmap.set_bits(alc.level(), pos, alc.size_at_level());
                }
            }
            else {
                constexpr size_t Levels = std::remove_pointer_t<decltype(bitmap)>::LEVELS;
                bool levels[Levels]{false,};

                for (auto alc: leaf_allocations) {
                    int32_t pos = (alc.position() - base) >> alc.level();
                    bitmap.clear_bits_opt(alc.level(), pos, alc.size_at_level());
                    levels[alc.level()] = true;
                }

                for (size_t ll = 0; ll < Levels; ll++) {
                    if (levels[ll]) {
                        bitmap.rebuild_bitmaps(ll);
                    }
                }
            }

            return bitmap.reindex(false);
        }
    };

    void iter_populate_leaf(Span<const AllocationMetadataT> leaf_allocations, bool set_bits, CtrSizeT leaf_base)
    {
        auto& self = the_self();
        self.ctr().leaf_dispatcher().dispatch(self.path().leaf(), PopulateLeafFn(), leaf_allocations, set_bits, leaf_base);
        self.ctr().ctr_update_path(self.path(), 0);
    }


    struct PosWalker {
        CtrSizeT prefix_{};

        template <typename CtrT, typename NodeT>
        void treeNode(const BranchNodeSO<CtrT, NodeT>& node, WalkCmd cmd, int32_t start, int32_t end)
        {
            auto stream = node.template substream<IntList<0>>();
            prefix_ += stream.sum(0, start, end);
        }


        template <typename CtrT, typename NodeT>
        void treeNode(const LeafNodeSO<CtrT, NodeT>& node, WalkCmd cmd, int32_t start, int32_t end) {
            prefix_ += end - start;
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
        size_t treeNode(T&& node_so, int32_t level, int32_t pos) const noexcept
        {
            auto bitmap = node_so.template substream_by_idx<1>();
            size_t bm_size = bitmap.data()->size(level);
            (void)bm_size;
            MEMORIA_ASSERT(pos, <, bm_size);
            MEMORIA_ASSERT(pos, >=, 0);

            return bitmap.data()->get_bit(level, pos);
        }
    };

    int32_t iter_get_bit(int32_t level, int32_t pos) const
    {
        auto& self = this->self();
        return self.ctr().leaf_dispatcher().dispatch(self.leaf(), GetBitsFn(), level, pos);
    }

    int32_t iter_get_bit(int32_t level) const
    {
        auto& self = this->self();
        return self.ctr().leaf_dispatcher().dispatch(self.path().leaf(), GetBitsFn(), level, self.iter_local_pos() >> level);
    }

    virtual int32_t leaf_size() const {
        return self().iter_leaf_size();
    }

    struct GetBitmapFn {
        template <typename T>
        Result<const PkdAllocationMap<PkdAllocationMapTypes>*> treeNode(T&& node_so) const noexcept
        {
            using ResultT = Result<const PkdAllocationMap<PkdAllocationMapTypes>*>;
            return ResultT::of(node_so.template substream_by_idx<1>().data());
        }
    };

    virtual const PkdAllocationMap<PkdAllocationMapTypes>* bitmap() const
    {
        auto& self = the_self();
        return self.ctr().leaf_dispatcher().dispatch(
                    self.path().leaf(),
                    GetBitmapFn()
        ).get_or_throw();
    }

MEMORIA_V1_ITERATOR_PART_END

#define M_TYPE      MEMORIA_V1_ITERATOR_TYPE(alcmap::ItrApiName)
#define M_PARAMS    MEMORIA_V1_ITERATOR_TEMPLATE_PARAMS



#undef M_TYPE
#undef M_PARAMS

}
