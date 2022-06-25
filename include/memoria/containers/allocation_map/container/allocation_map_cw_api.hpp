
// Copyright 2020-2022 Victor Smirnov
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


#include <memoria/containers/allocation_map/allocation_map_chunk_impl.hpp>


#include <memoria/api/allocation_map/allocation_map_api.hpp>

#include <memoria/prototypes/bt/nodes/branch_node_so.hpp>
#include <memoria/prototypes/bt/nodes/leaf_node_so.hpp>

#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>
#include <memoria/core/memory/memory.hpp>

#include <memoria/core/tools/optional.hpp>


#include <memoria/store/swmr/common/allocation_pool.hpp>

#include <vector>

namespace memoria {

MEMORIA_V1_CONTAINER_PART_BEGIN(alcmap::CtrWApiName)

    using typename Base::ContainerTypeName;
    using typename Base::BranchNodeExtData;
    using typename Base::LeafNodeExtData;
    using typename Base::Profile;
    using typename Base::ApiProfileT;
    using typename Base::CtrSizeT;
    using typename Base::TreeNodePtr;
    using typename Base::TreePathT;
    using typename Base::Position;
    using typename Base::OnLeafListener;
    using typename Base::ShuttleTypes;

    using Base::LEVELS;

    using ALCMeta           = AllocationMetadata<ApiProfileT>;
    using AllocationPoolT   = AllocationPool<ApiProfileT, LEVELS>;

    using typename Base::ChunkT;
    using typename Base::ChunkPtr;
    using typename Base::LeafPath;

    using typename Base::AlcMapChunkTypes;

    using typename Base::ChunkImplT;
    using typename Base::ChunkImplPtr;


    bool ctr_can_merge_nodes(const TreeNodePtr& tgt, const TreeNodePtr& src)  {
        return false;
    }


    struct ExpandBitmapFn {
        template <typename T>
        CtrSizeT treeNode(T&& node_so, CtrSizeT size) const
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

            bitmap_stream.data()->enlarge(static_cast<int32_t>(inserted));

            sizes_stream.data()->size() = bitmap_stream.data()->size();

            return inserted;
        }
    };

    CtrSizeT ctr_enlarge_leaf(const TreeNodePtr& node, CtrSizeT l0_size, bool update_path = true)
    {
        return self().leaf_dispatcher().dispatch(node, ExpandBitmapFn(), l0_size);
    }

    virtual CtrSizeT expand(CtrSizeT l0_size)
    {
        auto& self = this->self();
        CtrSizeT accum{};

        if (MMA_LIKELY(l0_size > 0))
        {
            auto ii = self.ctr_alcmap_seek_end();

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

        auto ii_start = self.ctr_alcmap_seek(up_to);
        return self.ctr_trim_tree(ii_start->path());
    }

//    virtual CtrSizeT rank(size_t level, CtrSizeT pos) const
//    {
//        return self().ctr_alcmap_seek(pos)->rank(level);
//    }

//    virtual CtrSizeT find_unallocated(
//        int32_t level,
//        CtrSizeT required,
//        ArenaBuffer<ALCMeta>& buffer
//    )
//    {
//        auto& self = this->self();
//        auto ii = self.ctr_alcmap_select0(level, 0);


//        CtrSizeT sum{};
//        while (is_valid_chunk(ii))
//        {
//            auto level0_pos = ii->level0_pos();
//            CtrSizeT available;
//            std::tie(available, ii) = ii->iter_count_fw();

//            auto len = available / (1 << level);

//            if (sum + len < required)
//            {
//                buffer.append_value(AllocationMetadata<ApiProfileT>{level0_pos, len, level});
//                ii = ii->iter_select_fw(level, 0);
//                sum += len;
//            }
//            else {
//                auto remainder = required - sum;
//                buffer.append_value(AllocationMetadata<ApiProfileT>{level0_pos, remainder, level});
//                sum += remainder;
//                break;
//            }
//        }

//        return sum;
//    }

    virtual CtrSizeT allocate(
            int32_t level,
            CtrSizeT required,
            ArenaBuffer<ALCMeta>& buffer
    ) {
        auto size0 = buffer.size();
        auto total = self().find_unallocated(level, required, buffer);

        if (total >= required) {
            setup_bits(buffer.span(size0), true, [](){}); // mark all regions as allocated
        }

        return total;
    }

//    struct ScanUnallocatedFn {
//        template <typename T>
//        void treeNode(T&& node_so, ArenaBuffer<ALCMeta>& arena, CtrSizeT offset) const
//        {
//            auto bitmap_stream_so = node_so.template substream_by_idx<1>();
//            const auto bitmaps = bitmap_stream_so.data();

//            for (int32_t lvl = LEVELS - 1; lvl >= 0; lvl--)
//            {
//                ALCMeta meta;
//                int32_t size = bitmaps->size(lvl);
//                int32_t rank = bitmaps->sum(lvl);

//                if (rank > 0) {
//                    // There are unallocated blocks, scan the bitmap for them.
//                    for (int32_t c = 0; c < size; c++)
//                    {
//                        bool upper_size_is_occupied = true;
//                        if (lvl < LEVELS - 1) {
//                            upper_size_is_occupied = bitmaps->get_bit(lvl + 1, c / 2);
//                        }

//                        if (upper_size_is_occupied && !bitmaps->get_bit(lvl, c))
//                        {
//                            if (!meta.size_at_level()) {
//                                meta = ALCMeta((c << lvl) + offset, 1 << lvl, lvl);
//                            }
//                            else {
//                                meta.enlarge1(1);
//                            }
//                        }
//                        else if (meta.size_at_level()) {
//                            arena.append_value(meta);
//                            meta = ALCMeta{0, 0, 0};
//                        }
//                    }

//                    if (meta.size_at_level()) {
//                        arena.append_value(meta);
//                    }
//                }
//            }
//        }
//    };


//    virtual void scan(const std::function<bool (Span<ALCMeta>)>& fn)
//    {
//        auto& self = this->self();

//        int32_t level = 0;
//        auto iter = self.ctr_alcmap_select0(level, 0);

//        ArenaBuffer<ALCMeta> arena;

//        while (is_valid_chunk(iter))
//        {
//            CtrSizeT offset = iter->level0_pos();

//            arena.clear();
//            self.leaf_dispatcher().dispatch(
//                        iter->path().leaf(),
//                        ScanUnallocatedFn(),
//                        arena,
//                        offset
//            );

//            if (arena.size()) {
//                if (!fn(arena.span())) {
//                    return;
//                }
//            }


//            iter = iter->iter_next_chunk();

//            if (is_valid_chunk(iter)) {
//                iter = iter->iter_select_fw(level, 0);
//            }
//            else {
//                break;
//            }
//        }

//    }


    struct LayoutLeafNodeFn {
        template <typename T>
        void treeNode(T&& node_so) const
        {
            node_so.layout();

            auto sizes_stream  = node_so.template substream_by_idx<0>();
            auto bitmap_stream = node_so.template substream_by_idx<1>();

            sizes_stream.data()->size() = bitmap_stream.data()->size();
        }
    };


    void ctr_layout_leaf_node(TreeNodePtr& node) const
    {
        return self().leaf_dispatcher().dispatch(node, LayoutLeafNodeFn());
    }


    CtrSizeT ctr_append_leaf(TreePathT& path, CtrSizeT size)
    {
        auto& self = this->self();

        if (MMA_LIKELY(size > 0))
        {
            if (path.size() == 1) {
                self.ctr_create_new_root_block(path);
            }

            auto new_leaf = self.ctr_create_node(0, false, true);

            auto leaf_inserted = self.ctr_enlarge_leaf(new_leaf, size);

            auto leaf_max = self.ctr_get_node_max_keys(new_leaf.as_immutable());

            auto branch_capacity = self.ctr_get_branch_node_capacity(path[1]);

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

            assert_success(self.ctr_insert_to_branch_node(path, 1, parent_idx, leaf_max, new_leaf->id()));

            path[0] = new_leaf.as_immutable();
            self.ctr_ref_block(new_leaf);
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

    void ctr_for_allocations(
            Span<ALCMeta> allocations,
            const std::function<void (ChunkImplPtr, Span<const ALCMeta>, CtrSizeT)>& leaf_updater
    )
    {
        auto& self = the_self();

        // FIXME: sort the allocations span here!
        size_t start = 0;

        if (allocations.size() > 0)
        {
            auto ii = self.ctr_alcmap_seek(allocations[start].position());

            ArenaBuffer<ALCMeta> buf;

            while (is_valid_chunk(ii))
            {                
                CtrSizeT local_pos = ii->iter_leaf_position();
                CtrSizeT leaf_base = ii->level0_pos() - local_pos;

                CtrSizeT leaf_size = ii->chunk_size();
                CtrSizeT leaf_limit = leaf_base + leaf_size;

                size_t end;
                for (end = start; end < allocations.size(); end++)
                {
                    ALCMeta& alc = allocations[end];

                    if (alc.position() >= leaf_limit) {
                        break;
                    }
                    else if (alc.position() + alc.size1() <= leaf_limit) {
                        buf.append_value(alc);
                    }
                    else {
                        auto alc_limit = alc.position() + alc.size1();
                        buf.append_value(alc.take(leaf_limit - alc_limit));
                        break;
                    }
                }

                if (end > start) {
                    leaf_updater(ii, allocations.subspan(start, end - start), leaf_base);
                }

                if (end < allocations.size())
                {
                    CtrSizeT skip_size = allocations[end].position() - allocations[start].position();
                    ii = ii->iter_next(skip_size);
                    start = end;
                    buf.clear();
                }
                else {
                    break;
                }
            }
        }
    }

    virtual void setup_bits(
            Span<ALCMeta> allocations,
            bool set_bits,
            const OnLeafListener& on_leaf
    )
    {
        auto& self = the_self();

        std::sort(allocations.begin(), allocations.end());

        if (allocations.size() > 0)
        {
            ctr_for_allocations(allocations, [&](ChunkImplPtr ii, Span<const ALCMeta> leaf_alc, CtrSizeT leaf_base){
                self.store().with_no_reentry(self.name(), [&](){
                    ii->iter_clone_path();
                    ii->iter_populate_leaf(leaf_alc, set_bits, leaf_base);
                });

                on_leaf();
            });
        }
    }




    virtual void touch_bits(Span<ALCMeta> allocations, const OnLeafListener& on_leaf)
    {
        auto& self = the_self();
        ctr_for_allocations(allocations, [&](ChunkImplPtr ii, Span<const ALCMeta>, CtrSizeT){
            self.store().with_no_reentry(self.name(), [&](){
                ii->iter_clone_path();
            });
            on_leaf();
        });
    }

    virtual CtrSizeT mark_allocated(const ALCMeta& alc) {
        return mark_allocated(alc.position(), alc.level(), alc.size_at_level());
    }


    virtual CtrSizeT mark_allocated(CtrSizeT pos, int32_t level, CtrSizeT size)
    {
        auto& self = this->self();
        auto ii = self.ctr_alcmap_seek(pos);
        return self.ctr_setup_bits(std::move(ii), level, size, true);
    }

    virtual CtrSizeT mark_unallocated(CtrSizeT pos, int32_t level, CtrSizeT size)
    {
        auto& self = this->self();
        auto ii = self.ctr_alcmap_seek(pos);
        return self.ctr_setup_bits(std::move(ii), level, size, false);
    }

    struct PopulateAllocationPoolFn {

        template <typename CtrT, typename NodeT>
        BoolResult treeNode(LeafNodeSO<CtrT, NodeT>& node_so, CtrSizeT base, AllocationPoolT& pool)
        {
            auto ss = node_so.template substream<IntList<0, 1>>();
            BoolResult res = ss.populate_allocation_pool(base, pool);
            return res;
        }
    };


    bool populate_allocation_pool(AllocationPoolT& pool, int32_t level)
    {
        auto& self = this->self();
        auto ii = self.ctr_alcmap_select0(level, 0);

        uint64_t cnt = 0;
        while (is_valid_chunk(ii))
        {
            CtrSizeT base = ii->level0_pos() - ii->iter_leaf_position();
            self.ctr_cow_clone_path(ii->path(), 0);

            bool updated = self.leaf_dispatcher().dispatch(
                        ii->path().leaf().as_mutable(),
                        PopulateAllocationPoolFn(),
                        base,
                        pool
            ).get_or_throw();

            if (updated)
            {
                cnt++;
                self.ctr_update_path(ii->path(), 0);

                if (pool.has_room(level))
                {
                    ii = ii->iter_next_chunk();
                    if (is_valid_chunk(ii)) {
                        ii = ii->iter_select_fw(level, 0);
                    }
                    else {
                        break;
                    }
                }
                else {
                    break;
                }
            }
            else {
                break;
            }
        }

        return cnt > 0;
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

    CtrSizeT ctr_setup_bits(ChunkImplPtr&& iter, size_t level, CtrSizeT size, bool set_bits)
    {
        auto& self = this->self();

        size_t local_pos = iter->iter_leaf_position() >> level;

        CtrSizeT accum{};

        while (accum < size)
        {
            self.ctr_cow_clone_path(iter->path(), 0);

            CtrSizeT remainder = size - accum;
            auto processed = self.leaf_dispatcher().dispatch(iter->path().leaf(), SetClearBitsFn(), local_pos, level, remainder, set_bits);

            accum += processed;

            self.ctr_update_path(iter->path(), 0);

            if (accum < size)
            {
                iter = iter->iter_next_chunk();
                if (!is_valid_chunk(iter)) {
                    break;
                }

                iter->leaf_position_ = local_pos = 0;
            }
            else {
                iter->leaf_position_ += processed << level;
            }
        }

        auto leaf_size = iter->chunk_size();

        if (leaf_size == iter->iter_leaf_position()) {
            iter = iter->iter_next_chunk();
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


    CtrSizeT ctr_touch_bits(ChunkImplPtr&& iter, size_t level, CtrSizeT size)
    {
        auto& self = this->self();

        size_t local_pos = iter->iter_leaf_position() >> level;

        CtrSizeT accum{};
        CtrSizeT remainder = size;
        while (accum < size)
        {
            self.ctr_cow_clone_path(iter->path(), 0);

            auto processed = self.leaf_dispatcher().dispatch(this->path().leaf(), TouchBitsFn(), local_pos, level, remainder);

            accum += processed;

            if (accum < size)
            {
                iter = iter->next_chunk();
                if (!is_valid_chunk(iter)) {
                    break;
                }

                iter->leaf_position_ = local_pos = 0;
            }
            else {
                iter->leaf_position_ += processed << level;
            }
        }

        auto leaf_size = iter->chunk_size();
        if (leaf_size == iter->iter_leaf_position()) {
            iter = iter->iter_next_leaf();
        }

        return accum;
    }



MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(alcmap::CtrWApiName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS

#undef M_PARAMS
#undef M_TYPE

}
