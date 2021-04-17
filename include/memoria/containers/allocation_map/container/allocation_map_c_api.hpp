
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


#include <memoria/containers/allocation_map/allocation_map_names.hpp>
#include <memoria/containers/allocation_map/allocation_map_tools.hpp>
#include <memoria/containers/allocation_map/allocation_map_api_impl.hpp>
#include <memoria/api/allocation_map/allocation_map_api.hpp>

#include <memoria/prototypes/bt/nodes/branch_node_so.hpp>
#include <memoria/prototypes/bt/nodes/leaf_node_so.hpp>

#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>
#include <memoria/core/memory/memory.hpp>

#include <memoria/core/tools/optional.hpp>

#include <vector>

namespace memoria {

MEMORIA_V1_CONTAINER_PART_BEGIN(alcmap::CtrApiName)

    using typename Base::ContainerTypeName;
    using typename Base::BranchNodeExtData;
    using typename Base::LeafNodeExtData;
    using typename Base::Profile;
    using typename Base::ApiProfileT;
    using typename Base::CtrSizeT;
    using typename Base::TreeNodePtr;
    using typename Base::TreePathT;
    using typename Base::Position;

    using Base::LEVELS;

    using CtrSizeTResult = Result<CtrSizeT>;

    using ApiIteratorResult = Result<CtrSharedPtr<AllocationMapIterator<ApiProfileT>>>;

    void configure_types(
        const ContainerTypeName& type_name,
        BranchNodeExtData& branch_node_ext_data,
        LeafNodeExtData& leaf_node_ext_data
    ) noexcept {

    }

    bool ctr_can_merge_nodes(const TreeNodePtr& tgt, const TreeNodePtr& src) noexcept
    {
        return false;
    }

    ApiIteratorResult iterator() noexcept
    {
        auto iter = self().ctr_begin();
        return memoria_static_pointer_cast<AllocationMapIterator<ApiProfileT>>(std::move(iter));
    }


    virtual ApiIteratorResult seek(CtrSizeT position) noexcept {
        auto iter = self().ctr_seek(position);
        return memoria_static_pointer_cast<AllocationMapIterator<ApiProfileT>>(std::move(iter));
    }


    struct ExpandBitmapFn {
        template <typename T>
        CtrSizeTResult treeNode(T&& node_so, CtrSizeT size) const noexcept
        {
            auto sizes_stream  = node_so.template substream_by_idx<0>();
            auto bitmap_stream = node_so.template substream_by_idx<1>();

            CtrSizeT space = bitmap_stream.data()->available_space();

            CtrSizeT inserted;

            if (size > space) {
                inserted = space;
            }
            else {
                inserted = size;
            }

            MEMORIA_TRY_VOID(bitmap_stream.data()->enlarge(static_cast<int32_t>(inserted)));

            sizes_stream.data()->size() = bitmap_stream.data()->size();

            return CtrSizeTResult::of(inserted);
        }
    };

    CtrSizeTResult ctr_enlarge_leaf(TreeNodePtr& node, CtrSizeT l0_size, bool update_path = true) noexcept
    {
        return self().leaf_dispatcher().dispatch(node, ExpandBitmapFn(), l0_size);
    }

    virtual Result<CtrSizeT> expand(CtrSizeT l0_size) noexcept
    {
        auto& self = this->self();
        CtrSizeT accum{};

        if (MMA_LIKELY(l0_size > 0))
        {
            MEMORIA_TRY(ii, self.ctr_end());

            TreePathT& path = ii->path();

            MEMORIA_TRY_VOID(self.ctr_cow_clone_path(path, 0));
            MEMORIA_TRY(inserted, self.ctr_enlarge_leaf(path.leaf(), l0_size));

            if (inserted > 0) {
                MEMORIA_TRY_VOID(self.ctr_update_path(path, 0));
            }

            accum += inserted;

            while (accum < l0_size)
            {
                MEMORIA_TRY(leaf_inserted, self.ctr_append_leaf(path, l0_size - accum));
                accum += leaf_inserted;
            }
        }

        return Result<CtrSizeT>::of(accum);
    }

    virtual VoidResult shrink(CtrSizeT up_to) noexcept {
        auto& self = this->self();

        MEMORIA_TRY(ii_start, self.ctr_seek(up_to));
        return self.ctr_trim_tree(ii_start->path());
    }

    virtual CtrSizeTResult rank(CtrSizeT pos) noexcept {
        return CtrSizeTResult::of();
    }

    virtual VoidResult find_unallocated(
        CtrSizeT from,
        int32_t level,
        CtrSizeT required,
        ArenaBuffer<AllocationMetadata<ApiProfileT>>& buffer
    ) noexcept
    {
        auto& self = this->self();
        auto ii = self.template ctr_select<IntList<0, 1>>(level, 1);
        MEMORIA_RETURN_IF_ERROR(ii);

        CtrSizeT sum{};
        while (!ii.get()->is_end())
        {
            MEMORIA_TRY(level0_pos, ii.get()->level0_pos());

            MEMORIA_TRY(available, ii.get()->count_fw());
            sum += available;

            buffer.append_value(AllocationMetadata<ApiProfileT>{level0_pos, available, level});

            if (sum < required)
            {
                auto res = ii.get()->template iter_select_fw<IntList<0, 1>>(level, 1);
                MEMORIA_RETURN_IF_ERROR(res);
            }
            else {
                break;
            }
        }

        if (sum >= required) {
            return VoidResult::of();
        }
        else {
            return MEMORIA_MAKE_GENERIC_ERROR(
                "No enought unallocated blocks found at the level {}: avaliable={}, required={}",
                level, sum, required
            );
        }
    }

    struct LayoutLeafNodeFn {
        template <typename T>
        VoidResult treeNode(T&& node_so) const noexcept
        {
            MEMORIA_TRY_VOID(node_so.layout(-1ull));

            auto sizes_stream  = node_so.template substream_by_idx<0>();
            auto bitmap_stream = node_so.template substream_by_idx<1>();

            sizes_stream.data()->size() = bitmap_stream.data()->size();

            return VoidResult::of();
        }
    };


    VoidResult ctr_layout_leaf_node(TreeNodePtr& node, const Position& sizes) const noexcept
    {
        return self().leaf_dispatcher().dispatch(node, LayoutLeafNodeFn());
    }


    CtrSizeTResult ctr_append_leaf(TreePathT& path, CtrSizeT size) noexcept
    {
        auto& self = this->self();

        if (MMA_LIKELY(size > 0))
        {
            if (path.size() == 1)
            {
                MEMORIA_TRY_VOID(self.ctr_create_new_root_block(path));
            }

            MEMORIA_TRY(new_leaf, self.ctr_create_node(0, false, true));

            MEMORIA_TRY(leaf_inserted, self.ctr_enlarge_leaf(new_leaf, size));

            MEMORIA_TRY(leaf_max, self.ctr_get_node_max_keys(new_leaf));

            MEMORIA_TRY(branch_capacity, self.ctr_get_branch_node_capacity(path[1], -1ull));

            int32_t parent_idx;
            MEMORIA_TRY(branch_size, self.ctr_get_branch_node_size(path[1]));
            if (MMA_UNLIKELY(branch_capacity == 0))
            {
                MEMORIA_TRY_VOID(self.ctr_split_path(path, 1, branch_size));
                MEMORIA_TRY_VOID(self.ctr_expect_next_node(path, 1));

                parent_idx = 0;
            }
            else {
                parent_idx = branch_size;
            }

            MEMORIA_TRY_VOID(self.ctr_insert_to_branch_node(path, 1, parent_idx, leaf_max, new_leaf->id()));

            path[0] = new_leaf;
            MEMORIA_TRY_VOID(self.ctr_ref_block(new_leaf->id()));
            return CtrSizeTResult::of(leaf_inserted);
        }
        else {
            return CtrSizeTResult::of(0);
        }
    }


    VoidResult ctr_trim_tree(TreePathT& start_path) noexcept
    {
        auto& self = this->self();

        if (start_path.size() > 1)
        {
            MEMORIA_TRY_VOID(self.ctr_cow_clone_path(start_path, 0));
            MEMORIA_TRY(parent_idx, self.ctr_get_parent_idx(start_path, 0));
            MEMORIA_TRY_VOID(self.ctr_remove_branch_nodes_at_end(start_path, 1, parent_idx + 1));
            return self.ctr_remove_redundant_root(start_path, 0);
        }

        return VoidResult::of();
    }


    virtual Result<CtrSizeT> setup_bits(Span<const AllocationMetadata<ApiProfileT>> allocations, bool set_bits) noexcept
    {
        using ResultT = Result<CtrSizeT>;
        auto& self = this->self();

        if (allocations.size() > 0)
        {
            CtrSizeT global_size{};

            auto alc_ii = allocations.begin();
            const auto& alloc0 = *alc_ii;

            CtrSizeT global_pos = alloc0.position();
            MEMORIA_TRY(ii, self.ctr_seek(global_pos));

            if (ii->iter_is_end()) {
                return ResultT::of();
            }

            CtrSizeT size = alloc0.size();
            int32_t level = alloc0.level();

            MEMORIA_TRY(processed, ii->iter_setup_bits(level, size, set_bits));

            global_size += processed << level;
            global_pos  += processed << level;

            alc_ii++;

            for (;alc_ii != allocations.end(); alc_ii++)
            {
                const auto& alc = *alc_ii;

                CtrSizeT next_pos = alc.position();
                CtrSizeT skip_size = next_pos - global_pos;

                MEMORIA_TRY_VOID(ii->skip(skip_size));

                if (ii->iter_is_end()) {
                    break;
                }

                global_pos += skip_size;

                CtrSizeT size_ii = alc.size();
                int32_t level_ii = alc.level();

                MEMORIA_TRY(processed_ii, ii->iter_setup_bits(level_ii, size_ii, set_bits));

                global_size += processed_ii << level_ii;
                global_pos  += (processed_ii << level_ii);
            }

            return ResultT::of(global_size);
        }
        else {
            return ResultT::of();
        }
    }


    virtual Result<CtrSizeT> touch_bits(Span<const AllocationMetadata<ApiProfileT>> allocations) noexcept
    {
        using ResultT = Result<CtrSizeT>;
        auto& self = this->self();

        if (allocations.size() > 0)
        {
            CtrSizeT global_size{};

            auto alc_ii = allocations.begin();
            const auto& alloc0 = *alc_ii;

            CtrSizeT global_pos = alloc0.position();
            MEMORIA_TRY(ii, self.ctr_seek(global_pos));

            if (ii->iter_is_end()) {
                return ResultT::of();
            }

            CtrSizeT size = alloc0.size();
            int32_t level = alloc0.level();

            MEMORIA_TRY(processed, ii->iter_touch_bits(level, size));

            global_size += processed << level;
            global_pos  += processed << level;

            alc_ii++;

            for (;alc_ii != allocations.end(); alc_ii++)
            {
                const auto& alc = *alc_ii;

                CtrSizeT next_pos = alc.position();
                CtrSizeT skip_size = next_pos - global_pos;

                MEMORIA_TRY_VOID(ii->skip(skip_size));

                if (ii->iter_is_end()) {
                    break;
                }

                global_pos += skip_size;

                CtrSizeT size_ii = alc.size();
                int32_t level_ii = alc.level();

                MEMORIA_TRY(processed_ii, ii->iter_touch_bits(level_ii, size_ii));

                global_size += processed_ii << level_ii;
                global_pos  += (processed_ii << level_ii);
            }

            return ResultT::of(global_size);
        }
        else {
            return ResultT::of();
        }
    }


    virtual Result<CtrSizeT> mark_allocated(CtrSizeT pos, int32_t level, CtrSizeT size) noexcept
    {
        auto& self = this->self();

        MEMORIA_TRY(ii, self.ctr_seek(pos << level));

        return ii->iter_setup_bits(level, size, true);
    }

    virtual Result<CtrSizeT> mark_unallocated(CtrSizeT pos, int32_t level, CtrSizeT size) noexcept
    {
        auto& self = this->self();

        MEMORIA_TRY(ii, self.ctr_seek(pos << level));

        return ii->iter_setup_bits(level, size, false);
    }



    struct UnallocatedFn {
        template <typename CtrT, typename BranchNode>
        CtrSizeTResult treeNode(const BranchNodeSO<CtrT, BranchNode>& node_so, int32_t level) noexcept
        {
            auto ss = node_so.template substream<IntList<0, 1>>();
            return CtrSizeTResult::of(ss.sum(level));
        }

        template <typename CtrT, typename BranchNode>
        VoidResult treeNode(const BranchNodeSO<CtrT, BranchNode>& node_so, Span<CtrSizeT>& ranks) noexcept
        {
            auto ss = node_so.template substream<IntList<0, 1>>();

            for (int32_t ll = 0; ll < LEVELS; ll++) {
                ranks[ll] = ss.sum(ll);
            }

            return VoidResult::of();
        }

        template <typename CtrT, typename BranchNode>
        CtrSizeTResult treeNode(const LeafNodeSO<CtrT, BranchNode>& node_so, int32_t level) noexcept
        {
            auto ss = node_so.template substream<IntList<0, 1>>();
            return CtrSizeTResult::of(ss.sum(level));
        }

        template <typename CtrT, typename BranchNode>
        VoidResult treeNode(const LeafNodeSO<CtrT, BranchNode>& node_so, Span<CtrSizeT>& ranks) noexcept
        {
            auto ss = node_so.template substream<IntList<0, 1>>();

            for (int32_t ll = 0; ll < LEVELS; ll++) {
                ranks[ll] = ss.sum(ll);
            }

            return VoidResult::of();
        }
    };

    virtual CtrSizeTResult unallocated_at(int32_t level) noexcept
    {
        auto& self = this->self();
        MEMORIA_TRY(root_node, self.ctr_get_root_node());
        return self.node_dispatcher().dispatch(root_node, UnallocatedFn(), level);
    }

    virtual VoidResult unallocated(Span<CtrSizeT> ranks) noexcept
    {
        auto& self = this->self();
        MEMORIA_TRY(root_node, self.ctr_get_root_node());
        return self.node_dispatcher().dispatch(root_node, UnallocatedFn(), ranks);
    }

    virtual Result<Optional<AllocationMapEntryStatus>> get_allocation_status(int32_t level, CtrSizeT position) noexcept
    {
        using ResultT = Result<Optional<AllocationMapEntryStatus>>;
        auto& self = this->self();

        MEMORIA_TRY(ii, self.ctr_seek(position << level));

        if (!ii->is_end())
        {
            MEMORIA_TRY(bit_status, ii->iter_get_bit(level));
            AllocationMapEntryStatus status = static_cast<AllocationMapEntryStatus>(bit_status);
            return ResultT::of(status);
        }
        else {
            return ResultT::of(Optional<AllocationMapEntryStatus>{});
        }

        return ResultT::of();
    }

MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(alcmap::CtrApiName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS

#undef M_PARAMS
#undef M_TYPE

}
