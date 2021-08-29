
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

    using ALCMeta           = AllocationMetadata<ApiProfileT>;
    using ApiIteratorT      = CtrSharedPtr<AllocationMapIterator<ApiProfileT>>;
    using CtrSizeTResult    = Result<CtrSizeT>;


    void configure_types(
        const ContainerTypeName& type_name,
        BranchNodeExtData& branch_node_ext_data,
        LeafNodeExtData& leaf_node_ext_data
    ) noexcept {

    }

    bool ctr_can_merge_nodes(const TreeNodePtr& tgt, const TreeNodePtr& src) noexcept {
        return false;
    }

    ApiIteratorT iterator()
    {
        auto iter = self().ctr_begin();
        return memoria_static_pointer_cast<AllocationMapIterator<ApiProfileT>>(std::move(iter));
    }


    virtual ApiIteratorT seek(CtrSizeT position) noexcept {
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

    CtrSizeT ctr_enlarge_leaf(const TreeNodePtr& node, CtrSizeT l0_size, bool update_path = true)
    {
        return self().leaf_dispatcher().dispatch(node, ExpandBitmapFn(), l0_size).get_or_throw();
    }

    virtual CtrSizeT expand(CtrSizeT l0_size)
    {
        auto& self = this->self();
        CtrSizeT accum{};

        if (MMA_LIKELY(l0_size > 0))
        {
            auto ii = self.ctr_end();

            TreePathT& path = ii->path();

            self.ctr_cow_clone_path(path, 0);
            auto inserted = self.ctr_enlarge_leaf(path.leaf().as_mutable(), l0_size);

            if (inserted > 0) {
                self.ctr_update_path(path, 0);
            }

            accum += inserted;

            while (accum < l0_size)
            {
                auto leaf_inserted = self.ctr_append_leaf(path, l0_size - accum);
                accum += leaf_inserted;
            }
        }

        return accum;
    }

    virtual void shrink(CtrSizeT up_to) {
        auto& self = this->self();

        auto ii_start = self.ctr_seek(up_to);
        return self.ctr_trim_tree(ii_start->path());
    }

    virtual CtrSizeT rank(CtrSizeT pos) {
        return CtrSizeT{};
    }

    virtual CtrSizeT find_unallocated(
        int32_t level,
        CtrSizeT required,
        ArenaBuffer<ALCMeta>& buffer
    )
    {
        auto& self = this->self();
        auto ii = self.template ctr_select<IntList<0, 1>>(level, 1);

        CtrSizeT sum{};
        while (!ii->is_end())
        {
            auto level0_pos = ii.get()->level0_pos();
            auto available = ii.get()->count_fw();

            auto len = available / (1 << level);            


            if (sum + len < required)
            {
                buffer.append_value(AllocationMetadata<ApiProfileT>{level0_pos, len, level});
                ii.get()->template iter_select_fw<IntList<0, 1>>(level, 1);
                sum += len;
            }
            else {
                auto remainder = required - sum;
                buffer.append_value(AllocationMetadata<ApiProfileT>{level0_pos, remainder, level});
                sum += remainder;
                break;
            }
        }

        return sum;
    }

    virtual CtrSizeT allocate(
            int32_t level,
            CtrSizeT required,
            ArenaBuffer<ALCMeta>& buffer
    ) {
        auto size0 = buffer.size();
        auto total = find_unallocated(level, required, buffer);

        if (total >= required) {
            setup_bits(buffer.span(size0), true); // mark all regions as allocated
        }

        return total;
    }

    struct ScanUnallocatedFn {
        template <typename T>
        VoidResult treeNode(T&& node_so, ArenaBuffer<ALCMeta>& arena, CtrSizeT offset) const noexcept
        {
            auto bitmap_stream_so = node_so.template substream_by_idx<1>();
            const auto bitmaps = bitmap_stream_so.data();

            for (int32_t lvl = LEVELS - 1; lvl >= 0; lvl--)
            {
                ALCMeta meta;
                int32_t size = bitmaps->size(lvl);
                int32_t rank = bitmaps->sum(lvl);

                if (rank > 0) {
                    // There are unallocated blocks, scan the bitmap for them.
                    for (int32_t c = 0; c < size; c++)
                    {
                        bool upper_size_is_occupied = true;
                        if (lvl < LEVELS - 1) {
                            upper_size_is_occupied = bitmaps->get_bit(lvl + 1, c / 2);
                        }

                        if (upper_size_is_occupied && !bitmaps->get_bit(lvl, c))
                        {
                            if (!meta.size()) {
                                meta = ALCMeta{(c << lvl) + offset, 1, lvl};
                            }
                            else {
                                meta.enlarge(1);
                            }
                        }
                        else if (meta.size()) {
                            arena.append_value(meta);
                            meta = ALCMeta{0, 0, 0};
                        }
                    }

                    if (meta.size()) {
                        arena.append_value(meta);
                    }
                }
            }

            return VoidResult::of();
        }
    };


    virtual void scan(const std::function<bool (Span<ALCMeta>)>& fn)
    {
        auto& self = this->self();

        int32_t level = 0;
        auto iter = self.template ctr_select<IntList<0, 1>>(level, 1);

        ArenaBuffer<ALCMeta> arena;

        while (!iter->is_end())
        {
            CtrSizeT offset = iter->level0_pos();

            arena.clear();
            self.leaf_dispatcher().dispatch(
                        iter->path().leaf(),
                        ScanUnallocatedFn(),
                        arena,
                        offset
            ).get_or_throw();

            if (arena.size()) {
                if (!fn(arena.span())) {
                    return;
                }
            }

            if (iter->next_leaf()) {
                iter->template iter_select_fw<IntList<0, 1>>(level, 1);
            }
            else {
                break;
            }
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


    void ctr_layout_leaf_node(TreeNodePtr& node, const Position& sizes) const
    {
        return self().leaf_dispatcher().dispatch(node, LayoutLeafNodeFn()).get_or_throw();
    }


    CtrSizeT ctr_append_leaf(TreePathT& path, CtrSizeT size)
    {
        auto& self = this->self();

        if (MMA_LIKELY(size > 0))
        {
            if (path.size() == 1)
            {
                self.ctr_create_new_root_block(path);
            }

            auto new_leaf = self.ctr_create_node(0, false, true);

            auto leaf_inserted = self.ctr_enlarge_leaf(new_leaf, size);

            auto leaf_max = self.ctr_get_node_max_keys(new_leaf.as_immutable());

            auto branch_capacity = self.ctr_get_branch_node_capacity(path[1], -1ull);

            int32_t parent_idx;
            auto branch_size = self.ctr_get_branch_node_size(path[1]);
            if (MMA_UNLIKELY(branch_capacity == 0))
            {
                self.ctr_split_path(path, 1, branch_size);
                self.ctr_expect_next_node(path, 1);

                parent_idx = 0;
            }
            else {
                parent_idx = branch_size;
            }

            self.ctr_insert_to_branch_node(path, 1, parent_idx, leaf_max, new_leaf->id()).get_or_throw();

            path[0] = new_leaf.as_immutable();
            self.ctr_ref_block(new_leaf->id());
            return leaf_inserted;
        }
        else {
            return CtrSizeT{};
        }
    }


    void ctr_trim_tree(TreePathT& start_path)
    {
        auto& self = this->self();

        if (start_path.size() > 1)
        {
            self.ctr_cow_clone_path(start_path, 0);
            auto parent_idx = self.ctr_get_parent_idx(start_path, 0);
            self.ctr_remove_branch_nodes_at_end(start_path, 1, parent_idx + 1);
            return self.ctr_remove_redundant_root(start_path, 0);
        }
    }


    virtual CtrSizeT setup_bits(Span<const ALCMeta> allocations, bool set_bits)
    {
        auto& self = this->self();

        if (allocations.size() > 0)
        {
            CtrSizeT global_size{};

            auto alc_ii = allocations.begin();
            const auto& alloc0 = *alc_ii;

            CtrSizeT global_pos = alloc0.position();
            auto ii = self.ctr_seek(global_pos);

            if (ii->iter_is_end()) {
                return CtrSizeT{};
            }

            CtrSizeT size = alloc0.size();
            int32_t level = alloc0.level();

            auto processed = ii->iter_setup_bits(level, size, set_bits);

            global_size += processed << level;
            global_pos  += processed << level;

            alc_ii++;

            for (;alc_ii != allocations.end(); alc_ii++)
            {
                const auto& alc = *alc_ii;

                CtrSizeT next_pos = alc.position();
                CtrSizeT skip_size = next_pos - global_pos;

                ii->skip(skip_size);

                if (ii->iter_is_end()) {
                    break;
                }

                global_pos += skip_size;

                CtrSizeT size_ii = alc.size();
                int32_t level_ii = alc.level();

                auto processed_ii = ii->iter_setup_bits(level_ii, size_ii, set_bits);

                global_size += processed_ii << level_ii;
                global_pos  += (processed_ii << level_ii);
            }

            return global_size;
        }
        else {
            return CtrSizeT{};
        }
    }


    virtual CtrSizeT touch_bits(Span<const ALCMeta> allocations)
    {
        auto& self = this->self();

        if (allocations.size() > 0)
        {
            CtrSizeT global_size{};

            auto alc_ii = allocations.begin();
            const auto& alloc0 = *alc_ii;

            CtrSizeT global_pos = alloc0.position();
            auto ii = self.ctr_seek(global_pos);

            if (ii->iter_is_end()) {
                return CtrSizeT{};
            }

            CtrSizeT size = alloc0.size();
            int32_t level = alloc0.level();

            auto processed = ii->iter_touch_bits(level, size);

            global_size += processed << level;
            global_pos  += processed << level;

            alc_ii++;

            for (;alc_ii != allocations.end(); alc_ii++)
            {
                const auto& alc = *alc_ii;

                CtrSizeT next_pos = alc.position();
                CtrSizeT skip_size = next_pos - global_pos;

                ii->skip(skip_size);

                if (ii->iter_is_end()) {
                    break;
                }

                global_pos += skip_size;

                CtrSizeT size_ii = alc.size();
                int32_t level_ii = alc.level();

                auto processed_ii = ii->iter_touch_bits(level_ii, size_ii);

                global_size += processed_ii << level_ii;
                global_pos  += (processed_ii << level_ii);
            }

            return global_size;
        }
        else {
            return CtrSizeT{};
        }
    }


    virtual CtrSizeT mark_allocated(CtrSizeT pos, int32_t level, CtrSizeT size)
    {
        auto& self = this->self();

        auto ii = self.ctr_seek(pos << level);

        return ii->iter_setup_bits(level, size, true);
    }

    virtual CtrSizeT mark_unallocated(CtrSizeT pos, int32_t level, CtrSizeT size)
    {
        auto& self = this->self();

        auto ii = self.ctr_seek(pos << level);

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

    virtual CtrSizeT unallocated_at(int32_t level)
    {
        auto& self = this->self();
        auto root_node = self.ctr_get_root_node();
        return self.node_dispatcher().dispatch(root_node, UnallocatedFn(), level).get_or_throw();
    }

    virtual void unallocated(Span<CtrSizeT> ranks)
    {
        auto& self = this->self();
        auto root_node = self.ctr_get_root_node();
        return self.node_dispatcher().dispatch(root_node, UnallocatedFn(), ranks).get_or_throw();
    }

    virtual Optional<AllocationMapEntryStatus> get_allocation_status(int32_t level, CtrSizeT position)
    {
        auto& self = this->self();

        auto ii = self.ctr_seek(position << level);

        if (!ii->is_end())
        {
            auto bit_status = ii->iter_get_bit(level);
            AllocationMapEntryStatus status = static_cast<AllocationMapEntryStatus>(bit_status);
            return status;
        }

        return Optional<AllocationMapEntryStatus>{};
    }



MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(alcmap::CtrApiName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS

#undef M_PARAMS
#undef M_TYPE

}
