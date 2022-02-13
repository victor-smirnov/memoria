
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


#include <memoria/store/swmr/common/allocation_pool.hpp>

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
    using typename Base::IteratorPtr;
    using typename Base::OnLeafListener;

    using Base::LEVELS;

    using ALCMeta           = AllocationMetadata<ApiProfileT>;
    using ApiIteratorT      = CtrSharedPtr<AllocationMapIterator<ApiProfileT>>;
    using CtrSizeTResult    = Result<CtrSizeT>;
    using AllocationPoolT   = AllocationPool<ApiProfileT, LEVELS>;


    void configure_types(
        const ContainerTypeName& type_name,
        BranchNodeExtData& branch_node_ext_data,
        LeafNodeExtData& leaf_node_ext_data
    )  {

    }

    bool ctr_can_merge_nodes(const TreeNodePtr& tgt, const TreeNodePtr& src)  {
        return false;
    }

    ApiIteratorT iterator()
    {
        auto iter = self().ctr_begin();
        return memoria_static_pointer_cast<AllocationMapIterator<ApiProfileT>>(std::move(iter));
    }


    virtual ApiIteratorT seek(CtrSizeT position)  {
        auto iter = self().ctr_seek(position);
        return memoria_static_pointer_cast<AllocationMapIterator<ApiProfileT>>(std::move(iter));
    }


    struct ExpandBitmapFn {
        template <typename T>
        CtrSizeTResult treeNode(T&& node_so, CtrSizeT size) const
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
        auto ii = self.template ctr_select<IntList<0, 1>>(level, 0, SeqOpType::EQ);

        CtrSizeT sum{};
        while (!ii->is_end())
        {
            auto level0_pos = ii.get()->level0_pos();
            auto available = ii.get()->count_fw();

            auto len = available / (1 << level);            


            if (sum + len < required)
            {
                buffer.append_value(AllocationMetadata<ApiProfileT>{level0_pos, len, level});
                ii.get()->template iter_select_fw<IntList<0, 1>>(level, 0, SeqOpType::EQ);
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
            setup_bits(buffer.span(size0), true, [](){}); // mark all regions as allocated
        }

        return total;
    }

    struct ScanUnallocatedFn {
        template <typename T>
        void treeNode(T&& node_so, ArenaBuffer<ALCMeta>& arena, CtrSizeT offset) const
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
                            if (!meta.size_at_level()) {
                                meta = ALCMeta((c << lvl) + offset, 1 << lvl, lvl);
                            }
                            else {
                                meta.enlarge1(1);
                            }
                        }
                        else if (meta.size_at_level()) {
                            arena.append_value(meta);
                            meta = ALCMeta{0, 0, 0};
                        }
                    }

                    if (meta.size_at_level()) {
                        arena.append_value(meta);
                    }
                }
            }
        }
    };


    virtual void scan(const std::function<bool (Span<ALCMeta>)>& fn)
    {
        auto& self = this->self();

        int32_t level = 0;
        auto iter = self.template ctr_select<IntList<0, 1>>(level, 0, SeqOpType::EQ);

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
            );

            if (arena.size()) {
                if (!fn(arena.span())) {
                    return;
                }
            }

            if (iter->next_leaf()) {
                iter->template iter_select_fw<IntList<0, 1>>(level, 0, SeqOpType::EQ);
            }
            else {
                break;
            }
        }

    }


    struct LayoutLeafNodeFn {
        template <typename T>
        VoidResult treeNode(T&& node_so) const
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

    void for_allocations(
            Span<ALCMeta> allocations,
            const std::function<void (IteratorPtr, Span<const ALCMeta>, CtrSizeT)>& leaf_updater
    )
    {
        auto& self = the_self();

        // FIXME: sort the allocations span here!
        size_t start = 0;

        if (allocations.size() > 0)
        {            
            IteratorPtr ii = self.ctr_seek(allocations[start].position());

            ArenaBuffer<ALCMeta> buf;

            while (!ii->is_end())
            {                
                int32_t local_pos = ii->iter_local_pos();
                CtrSizeT leaf_base = ii->level0_pos() - local_pos;

                int32_t leaf_size = ii->iter_leaf_size();
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
                    ii->skip(skip_size);
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
            for_allocations(allocations, [&](IteratorPtr ii, Span<const ALCMeta> leaf_alc, CtrSizeT leaf_base){
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
        for_allocations(allocations, [&](IteratorPtr ii, Span<const ALCMeta>, CtrSizeT){
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
        auto ii = self.ctr_seek(pos);
        return ii->iter_setup_bits(level, size, true);
    }

    virtual CtrSizeT mark_unallocated(CtrSizeT pos, int32_t level, CtrSizeT size)
    {
        auto& self = this->self();
        auto ii = self.ctr_seek(pos);
        return ii->iter_setup_bits(level, size, false);
    }



    struct UnallocatedFn {
        template <typename CtrT, typename BranchNode>
        CtrSizeT treeNode(const BranchNodeSO<CtrT, BranchNode>& node_so, int32_t level)
        {
            auto ss = node_so.template substream<IntList<0, 1>>();
            return ss.sum(level);
        }

        template <typename CtrT, typename BranchNode>
        void treeNode(const BranchNodeSO<CtrT, BranchNode>& node_so, Span<CtrSizeT>& ranks)
        {
            auto ss = node_so.template substream<IntList<0, 1>>();

            for (int32_t ll = 0; ll < LEVELS; ll++) {
                ranks[ll] = ss.sum(ll);
            }
        }

        template <typename CtrT, typename BranchNode>
        CtrSizeT treeNode(const LeafNodeSO<CtrT, BranchNode>& node_so, int32_t level)
        {
            auto ss = node_so.template substream<IntList<0, 1>>();
            return ss.sum(level);
        }

        template <typename CtrT, typename BranchNode>
        void treeNode(const LeafNodeSO<CtrT, BranchNode>& node_so, Span<CtrSizeT>& ranks)
        {
            auto ss = node_so.template substream<IntList<0, 1>>();

            for (int32_t ll = 0; ll < LEVELS; ll++) {
                ranks[ll] = ss.sum(ll);
            }
        }
    };

    virtual CtrSizeT unallocated_at(int32_t level)
    {
        auto& self = this->self();
        auto root_node = self.ctr_get_root_node();
        return self.node_dispatcher().dispatch(root_node, UnallocatedFn(), level);
    }

    virtual void unallocated(Span<CtrSizeT> ranks)
    {
        auto& self = this->self();
        auto root_node = self.ctr_get_root_node();
        return self.node_dispatcher().dispatch(root_node, UnallocatedFn(), ranks);
    }

    virtual Optional<AllocationMapEntryStatus> get_allocation_status(int32_t level, CtrSizeT position)
    {
        auto& self = this->self();

        auto ii = self.ctr_seek(position);

        if (!ii->is_end())
        {
            auto bit_status = ii->iter_get_bit(level);
            AllocationMapEntryStatus status = static_cast<AllocationMapEntryStatus>(bit_status);
            return status;
        }

        return Optional<AllocationMapEntryStatus>{};
    }


    virtual bool check_allocated(const ALCMeta& meta)
    {
        for (CtrSizeT c = 0; c < meta.size_at_level(); c++) {
            auto status = get_allocation_status(meta.level(), meta.position() + (c << meta.level()));
            if ((!status) || status.get() == AllocationMapEntryStatus::FREE) {
                return false;
            }
        }

        return true;
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
        auto ii = self.template ctr_select<IntList<0, 1>>(level, 0, SeqOpType::EQ);

        uint64_t cnt = 0;
        while (!ii->is_end())
        {
            CtrSizeT base = ii->level0_pos() - ii->iter_local_pos();
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

                if (pool.has_room(level) && ii->next_leaf()) {
                    ii->template iter_select_fw<IntList<0, 1>>(level, 0, SeqOpType::EQ);
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


    class CompareHelper: public AllocationMapCompareHelper<ApiProfileT> {
        using MapIteratorPtr = CtrSharedPtr<AllocationMapIterator<ApiProfileT>>;

        MapIteratorPtr my_ii_;
        MapIteratorPtr other_ii_;

        int32_t my_idx_{};
        int32_t other_idx_{};
        int32_t level_{};

        int32_t my_bit_{};
        int32_t other_bit_{};

        CtrSizeT my_base_{};
        CtrSizeT other_base_{};

    public:
        CompareHelper(
                MapIteratorPtr my_ii,
                MapIteratorPtr other_ii
        ): my_ii_(my_ii), other_ii_(other_ii)
        {}


        AnyID my_id() const {return my_ii_->leaf_id();}
        AnyID other_id() const {return other_ii_->leaf_id();}

        int32_t my_idx() const {return my_idx_;}
        int32_t other_idx() const {return other_idx_;}
        int32_t level() const {return level_;}

        int32_t my_bit() const {return my_bit_;}
        int32_t other_bit() const {return other_bit_;}

        CtrSizeT my_base() const {
            return my_base_;
        }

        CtrSizeT other_base() const {
            return other_base_;
        }

        void dump_my_leaf() {my_ii_->dump();}
        void dump_other_leaf() {other_ii_->dump();}

        void set_my_idx(int32_t v) {
            my_idx_ = v;
        }

        void set_other_idx(int32_t v) {
            other_idx_ = v;
        }

        void set_level(int32_t v) {
            level_ = v;
        }

        void set_my_bit(int32_t v) {
            my_bit_ = v;
        }

        void set_other_bit(int32_t v) {
            other_bit_ = v;
        }

        void add_to_my_base(CtrSizeT v) {
            my_base_ += v;
        }

        void add_to_other_base(CtrSizeT v) {
            other_base_ += v;
        }
    };


    CtrSizeT compare_with(
        CtrSharedPtr<ICtrApi<AllocationMap, ApiProfileT>> other,
        const std::function<bool (AllocationMapCompareHelper<ApiProfileT>&)>& consumer
    ) {
        auto my_ii = self().iterator();
        auto other_ii = other->iterator();

        CompareHelper helper(my_ii, other_ii);

        CtrSizeT mismatches = 0;

        int32_t my_pos = 0;
        int32_t other_pos = 0;

        while (!my_ii->is_end())
        {
            auto my_bitmap = my_ii->bitmap();
            auto other_bitmap = other_ii->bitmap();

            int32_t my_limit = my_bitmap->size();
            int32_t other_limit = other_bitmap->size();

            auto fn = [&](int32_t my_idx, int32_t other_idx, int32_t level, int32_t my_value, int32_t other_value) {
                return wrap_throwing([&]() -> bool {
                    mismatches++;

                    helper.set_my_idx(my_idx);
                    helper.set_other_idx(other_idx);
                    helper.set_level(level);

                    helper.set_my_bit(my_value);
                    helper.set_other_bit(other_value);

                    return consumer(helper);
                });
            };

            if (my_limit - my_pos <= other_limit - other_pos)
            {
                int32_t size = my_limit - my_pos;
                bool do_continue = my_bitmap->compare_with(other_bitmap, my_pos, other_pos, size, fn).get_or_throw();

                if (!do_continue) {
                    break;
                }

                other_pos += size;

                if (!my_ii->next_leaf()) {
                    break;
                }
                my_pos = 0;
                helper.add_to_my_base(my_limit);
            }
            else {
                int32_t size = other_limit - other_pos;
                bool do_continue = my_bitmap->compare_with(other_bitmap, my_pos, other_pos, size, fn).get_or_throw();

                if (!do_continue) {
                    break;
                }

                my_pos += size;

                if (!other_ii->next_leaf()) {
                    break;
                }

                other_pos = 0;
                helper.add_to_other_base(other_limit);
            }
        }

        return mismatches;
    }


MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(alcmap::CtrApiName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS

#undef M_PARAMS
#undef M_TYPE

}
