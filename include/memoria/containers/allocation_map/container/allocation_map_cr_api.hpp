
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

MEMORIA_V1_CONTAINER_PART_BEGIN(alcmap::CtrRApiName)

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
    using CtrSizeTResult    = Result<CtrSizeT>;
    using AllocationPoolT   = AllocationPool<ApiProfileT, LEVELS>;

    using ChunkT = AllocationMapChunk<ApiProfile<Profile>>;
    using ChunkPtr = IterSharedPtr<ChunkT>;
    using LeafPath = IntList<0, 1>;

    struct AlcMapChunkTypes: TypesType {
        using ShuttleTypes = typename Base::ShuttleTypes;
    };

    using ChunkImplT = AllocationMapChunkImpl<AlcMapChunkTypes>;
    using ChunkImplPtr = IterSharedPtr<ChunkImplT>;



    void configure_types(
        const ContainerTypeName& type_name,
        BranchNodeExtData& branch_node_ext_data,
        LeafNodeExtData& leaf_node_ext_data
    )  {

    }

    ChunkImplPtr ctr_alcmap_seek(CtrSizeT pos) const
    {
        auto& self = this->self();
        return self.ctr_descend(
            TypeTag<ChunkImplT>{},
            TypeTag<bt::SkipForwardShuttle<ShuttleTypes, 0, ChunkImplT>>{},
            pos
        );
    }

    ChunkPtr seek(CtrSizeT pos) const {
        return ctr_alcmap_seek(pos);
    }

    ChunkImplPtr ctr_alcmap_select0(size_t level, CtrSizeT rank) const
    {
        auto& self = this->self();
        return self.ctr_descend(
            TypeTag<ChunkImplT>{},
            TypeTag<bt::SelectForwardShuttle<ShuttleTypes, LeafPath, ChunkImplT>>{},
            level, rank, SeqOpType::EQ
        );
    }

    ChunkImplPtr ctr_alcmap_seek_end() const
    {
        auto& self = this->self();
        return self.ctr_alcmap_seek(self.size());
    }



    virtual CtrSizeT rank(size_t level, CtrSizeT pos) const
    {
        return self().ctr_alcmap_seek(pos)->rank(level);
    }

    virtual CtrSizeT find_unallocated(
        int32_t level,
        CtrSizeT required,
        ArenaBuffer<ALCMeta>& buffer
    )
    {
        auto& self = this->self();
        auto ii = self.ctr_alcmap_select0(level, 0);


        CtrSizeT sum{};
        while (is_valid_chunk(ii))
        {
            auto level0_pos = ii->level0_pos();
            CtrSizeT available;
            std::tie(available, ii) = ii->iter_count_fw();

            auto len = available / (1 << level);

            if (sum + len < required)
            {
                buffer.push_back(AllocationMetadata<ApiProfileT>{level0_pos, len, level});
                ii = ii->iter_select_fw(level, 0);
                sum += len;
            }
            else {
                auto remainder = required - sum;
                buffer.push_back(AllocationMetadata<ApiProfileT>{level0_pos, remainder, level});
                sum += remainder;
                break;
            }
        }

        return sum;
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
                            arena.push_back(meta);
                            meta = ALCMeta{0, 0, 0};
                        }
                    }

                    if (meta.size_at_level()) {
                        arena.push_back(meta);
                    }
                }
            }
        }
    };


    virtual void scan(const std::function<bool (Span<ALCMeta>)>& fn) const
    {
        auto& self = this->self();

        int32_t level = 0;
        auto iter = self.ctr_alcmap_select0(level, 0);

        ArenaBuffer<ALCMeta> arena;

        while (is_valid_chunk(iter))
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


            iter = iter->iter_next_chunk();

            if (is_valid_chunk(iter)) {
                iter = iter->iter_select_fw(level, 0);
            }
            else {
                break;
            }
        }

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

    virtual CtrSizeT unallocated_at(int32_t level) const
    {
        auto& self = this->self();
        auto root_node = self.ctr_get_root_node();
        return self.node_dispatcher().dispatch(root_node, UnallocatedFn(), level);
    }

    virtual void unallocated(Span<CtrSizeT> ranks) const
    {
        auto& self = this->self();
        auto root_node = self.ctr_get_root_node();
        return self.node_dispatcher().dispatch(root_node, UnallocatedFn(), ranks);
    }

    virtual Optional<AllocationMapEntryStatus> get_allocation_status(int32_t level, CtrSizeT position) const
    {
        auto& self = this->self();

        auto ii = self.ctr_alcmap_seek(position);

        if (is_valid_chunk(ii))
        {
            auto bit_status = ii->iter_get_bit(level);
            AllocationMapEntryStatus status = static_cast<AllocationMapEntryStatus>(bit_status);
            return status;
        }

        return Optional<AllocationMapEntryStatus>{};
    }


    virtual bool check_allocated(const ALCMeta& meta) const
    {
        for (CtrSizeT c = 0; c < meta.size_at_level(); c++) {
            auto status = get_allocation_status(meta.level(), meta.position() + (c << meta.level()));
            if ((!status) || status.get() == AllocationMapEntryStatus::FREE) {
                return false;
            }
        }

        return true;
    }




    class CompareHelper: public AllocationMapCompareHelper<ApiProfileT> {
        ChunkPtr my_ii_;
        ChunkPtr other_ii_;

        int32_t my_idx_{};
        int32_t other_idx_{};
        int32_t level_{};

        int32_t my_bit_{};
        int32_t other_bit_{};

        CtrSizeT my_base_{};
        CtrSizeT other_base_{};

    public:
        CompareHelper(
                ChunkPtr my_ii,
                ChunkPtr other_ii
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
    ) const {
        auto my_ii = self().seek(0);
        auto other_ii = other->seek(0);

        CompareHelper helper(my_ii, other_ii);

        CtrSizeT mismatches = 0;

        int32_t my_pos = 0;
        int32_t other_pos = 0;

        while (is_valid_chunk(my_ii))
        {
            auto my_bitmap = my_ii->bitmap();
            auto other_bitmap = other_ii->bitmap();

            int32_t my_limit = my_bitmap->size();
            int32_t other_limit = other_bitmap->size();

            auto fn = [&](int32_t my_idx, int32_t other_idx, int32_t level, int32_t my_value, int32_t other_value) {                
                mismatches++;

                helper.set_my_idx(my_idx);
                helper.set_other_idx(other_idx);
                helper.set_level(level);

                helper.set_my_bit(my_value);
                helper.set_other_bit(other_value);

                return consumer(helper);
            };

            if (my_limit - my_pos <= other_limit - other_pos)
            {
                int32_t size = my_limit - my_pos;
                bool do_continue = my_bitmap->compare_with(other_bitmap, my_pos, other_pos, size, fn);

                if (!do_continue) {
                    break;
                }

                other_pos += size;

                my_ii = my_ii->next_chunk();

                if (!is_valid_chunk(my_ii)) {
                    break;
                }

                my_pos = 0;
                helper.add_to_my_base(my_limit);
            }
            else {
                int32_t size = other_limit - other_pos;
                bool do_continue = my_bitmap->compare_with(other_bitmap, my_pos, other_pos, size, fn);

                if (!do_continue) {
                    break;
                }

                my_pos += size;

                other_ii = other_ii->next_chunk();

                if (!is_valid_chunk(other_ii)) {
                    break;
                }

                other_pos = 0;
                helper.add_to_other_base(other_limit);
            }
        }

        return mismatches;
    }

    void dump(ChunkDumpMode mode = ChunkDumpMode::LEAF, std::ostream& out = std::cout) const {
        auto& self = this->self();

        auto ii = self.iterator();
        while (is_valid_chunk(ii)) {
            ii->dump(mode, out);
            ii = ii->next_chunk();
        }
    }

MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(alcmap::CtrRApiName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS

#undef M_PARAMS
#undef M_TYPE

}
