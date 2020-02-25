
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
    using typename Base::CtrSizeT;
    using typename Base::NodeBaseG;
    using typename Base::TreePathT;
    using typename Base::Position;

    using ApiIteratorResult = Result<CtrSharedPtr<AllocationMapIterator<Profile>>>;

    void configure_types(
        const ContainerTypeName& type_name,
        BranchNodeExtData& branch_node_ext_data,
        LeafNodeExtData& leaf_node_ext_data
    ) noexcept {

    }

    bool ctr_can_merge_nodes(const NodeBaseG& tgt, const NodeBaseG& src) noexcept
    {
        return false;
    }

    ApiIteratorResult iterator() noexcept
    {
        auto iter = self().ctr_begin();
        return memoria_static_pointer_cast<AllocationMapIterator<Profile>>(std::move(iter));
    }


    virtual ApiIteratorResult seek(CtrSizeT position) noexcept {
        auto iter = self().ctr_seek(position);
        return memoria_static_pointer_cast<AllocationMapIterator<Profile>>(std::move(iter));
    }



    virtual Result<CtrSizeT> expand(CtrSizeT l0_size) noexcept
    {
        auto& self = this->self();
        MEMORIA_TRY(ii, self.ctr_end());

        CtrSizeT accum{};

        TreePathT& path = ii->path();

        while (accum < l0_size)
        {
            MEMORIA_TRY_VOID(self.ctr_append_leaf(path));
            MEMORIA_TRY(node_size, self.template ctr_get_leaf_stream_size<0>(path.leaf()));
            accum += node_size;
        }

        return Result<CtrSizeT>::of(accum);
    }

    virtual VoidResult shrink(CtrSizeT up_to) noexcept {
        auto& self = this->self();

        MEMORIA_TRY(ii_start, self.ctr_seek(up_to));
        return self.ctr_trim_tree(ii_start->path());
    }

    virtual Result<CtrSizeT> rank(CtrSizeT pos) noexcept {
        return Result<CtrSizeT>::of();
    }

    virtual VoidResult find_unallocated(
        CtrSizeT from,
        int32_t level,
        CtrSizeT required,
        CtrSizeT prefetch,
        ArenaBuffer<AllocationMetadata<Profile>>& buffer
    ) noexcept
    {
        auto& self = this->self();
        auto ii = self.template ctr_select<IntList<0, 1>>(level, 1);
        MEMORIA_RETURN_IF_ERROR(ii);

        return VoidResult::of();
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


    VoidResult ctr_layout_leaf_node(NodeBaseG& node, const Position& sizes) const noexcept
    {
        return self().leaf_dispatcher().dispatch(node, LayoutLeafNodeFn());
    }


    VoidResult ctr_append_leaf(TreePathT& path) noexcept
    {
        auto& self = this->self();

        if (path.size() == 1)
        {
            MEMORIA_TRY_VOID(self.ctr_create_new_root_block(path));
        }

        MEMORIA_TRY(new_leaf, self.ctr_create_node(0, false, true));
        MEMORIA_TRY(leaf_max, self.ctr_get_node_max_keys(new_leaf));

        MEMORIA_TRY(branch_capacity, self.ctr_get_branch_node_capacity(path[1], -1ull));

        int32_t parent_idx;
        MEMORIA_TRY(branch_size, self.ctr_get_branch_node_size(path[1]));
        if (MMA_UNLIKELY(branch_capacity == 0))
        {
            MEMORIA_TRY_VOID(self.ctr_split_path(path, 1, branch_size));
            parent_idx = 0;
        }
        else {
            parent_idx = branch_size;
        }

        MEMORIA_TRY_VOID(self.ctr_insert_to_branch_node(path, 1, parent_idx, leaf_max, new_leaf->id()));

        path[0] = new_leaf;
        MEMORIA_TRY_VOID(self.ctr_ref_block(new_leaf->id()));
        return VoidResult::of();
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


    virtual Result<CtrSizeT> setup_bits(Span<const AllocationMetadata<Profile>> allocations, bool set_bits) noexcept
    {
        using ResultT = Result<CtrSizeT>;
        auto& self = this->self();

        if (allocations.size() > 0)
        {
            CtrSizeT global_size{};

            auto alc_ii = allocations.begin();
            const auto& alloc0 = *alc_ii;

            CtrSizeT global_pos = alloc0.level0_position();
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
                CtrSizeT next_pos = alc.level0_position();
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

MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(alcmap::CtrApiName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS

#undef M_PARAMS
#undef M_TYPE

}
