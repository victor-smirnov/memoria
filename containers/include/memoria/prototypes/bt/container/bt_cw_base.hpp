
// Copyright 2011-2021 Victor Smirnov
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

#include <memoria/core/container/container.hpp>
#include <memoria/core/container/names.hpp>
#include <memoria/core/types/algo.hpp>
#include <memoria/core/tools/static_array.hpp>
#include <memoria/core/container/macros.hpp>
#include <memoria/core/memory/object_pool.hpp>
#include <memoria/prototypes/bt/bt_macros.hpp>
#include <memoria/prototypes/bt/tools/bt_tools_tree_path.hpp>

#include <memoria/prototypes/bt/shuttles/bt_shuttle_base.hpp>

#include <memoria/core/types/mp11.hpp>
#include <memoria/core/types/list/tuple.hpp>

#include <memoria/core/packed/misc/packed_tuple.hpp>
#include <memoria/core/packed/misc/packed_map.hpp>
#include <memoria/core/packed/misc/packed_map_so.hpp>

#include <memoria/prototypes/bt/bt_names.hpp>

#include <iostream>
#include <type_traits>

namespace memoria {


MEMORIA_V1_CONTAINER_PART_BEGIN(bt::BaseWName)

    using typename Base::Types;
    using typename Base::ROAllocator;

    using typename Base::SharedBlockPtr;
    using typename Base::SharedBlockConstPtr;
    using typename Base::BlockID;
    using typename Base::CtrID;
    using typename Base::ContainerTypeName;

    using typename Base::Profile;
    using typename Base::ApiProfileT;
    using typename Base::SnapshotID;


    using typename Base::BranchNodeEntry;

    using typename Base::TreeNodePtr;
    using typename Base::TreeNodeConstPtr;

    using typename Base::TreePathT;

    using typename Base::Position;
    using typename Base::CtrSizeT;
    using typename Base::CtrSizesT;

    using typename Base::NodeDispatcher;
    using typename Base::LeafDispatcher;
    using typename Base::BranchDispatcher;
    using typename Base::DefaultDispatcher;
    using typename Base::TreeDispatcher;

    using typename Base::Metadata;


    using typename Base::LeafNode;
    using typename Base::BranchNode;

    using typename Base::LeafNodeSO;
    using typename Base::BranchNodeSO;

    using typename Base::LeafNodeExtData;
    using typename Base::BranchNodeExtData;

    using typename Base::LeafNodeExtDataPkdTuple;
    using typename Base::BranchNodeExtDataPkdTuple;
    using typename Base::CtrPropertiesMap;
    using typename Base::CtrReferencesMap;

    using typename Base::BranchUpdateState;

    template <typename LeafPath>
    using LeafUpdateState = typename LeafNodeSO::template UpdateState<LeafPath>;

    using typename Base::ShuttleTypes;

    using Base::CONTAINER_HASH;

    using Base::Streams;

    using Base::METADATA_IDX;
    using Base::BRANCH_EXT_DATA_IDX;
    using Base::LEAF_EXT_DATA_IDX;
    using Base::CTR_PROPERTIES_IDX;
    using Base::CTR_REFERENCES_IDX;

    TreeNodePtr createRootLeaf() const {
        return self().ctr_create_root_node(0, true, -1);
    }

    struct MakeBranchUpdateStateFn {
        BranchUpdateState treeNode(BranchNodeSO so) {
            return so.make_update_state();
        }
    };

    BranchUpdateState ctr_make_branch_update_state(const TreeNodeConstPtr& node) const {
        return self().branch_dispatcher().dispatch(node, MakeBranchUpdateStateFn());
    }

    template <typename LeafPath>
    struct MakeLeafUpdateStateFn {
        LeafUpdateState<LeafPath> treeNode(LeafNodeSO so) {
            return so.template make_update_state<LeafPath>();
        }
    };


    template <typename LeafPath>
    LeafUpdateState<LeafPath> ctr_make_leaf_update_state(const TreeNodeConstPtr& node) const {
        return self().leaf_dispatcher().dispatch(node, MakeLeafUpdateStateFn<LeafPath>());
    }

    void ctr_upsize_node(TreePathT& path, size_t level, size_t upsize)
    {
        size_t free_space = path[level]->allocator()->free_space();

        if (free_space < upsize)
        {
            size_t memory_block_size = path[level]->header().memory_block_size();

            size_t total_free_space = free_space;

            while (total_free_space < upsize)
            {
                size_t next_memory_block_size = memory_block_size * 2;
                size_t additional_free_space  = next_memory_block_size - memory_block_size;

                memory_block_size = next_memory_block_size;
                total_free_space += additional_free_space;
            }

            self().ctr_cow_clone_path(path, level);
            return self().ctr_resize_block(path, level, memory_block_size);
        }
    }

    void ctr_upsize_node_2x(TreePathT& path, size_t level)
    {
        size_t memory_block_size = path[level]->header().memory_block_size();

        self().ctr_cow_clone_path(path, level);
        return self().ctr_resize_block(path, level, memory_block_size * 2);
    }

    void ctr_downsize_node(TreePathT& path, size_t level)
    {
        size_t memory_block_size = path[level]->header().memory_block_size();

        size_t used_memory_block_size = path[level]->used_memory_block_size();

        size_t min_block_size = 8192;

        if (memory_block_size > min_block_size)
        {
            size_t target_memory_block_size = min_block_size;
            while (target_memory_block_size < used_memory_block_size)
            {
                target_memory_block_size *= 2;
            }

            self().ctr_cow_clone_path(path, level);
            return self().ctr_resize_block(path, level, target_memory_block_size);
        }
    }


    virtual void set_ctr_property(U8StringView key, U8StringView value)
    {
        auto& self = this->self();

        auto root = self.ctr_get_root_node();

        TreePathT path = TreePathT::build(root, 1);

        CtrPropertiesMap* map = get<CtrPropertiesMap>(path.root().as_mutable()->allocator(), CTR_PROPERTIES_IDX);

        PackedMapSO<CtrPropertiesMap> map_so(map);

        self.ctr_cow_clone_path(path, 0);

        while (true)
        {
            auto us = map_so.make_update_state();
            PkdUpdateStatus status = map_so.prepare_set(key, value, us.first);
            if (is_success(status))
            {
                map_so.commit_set(key, value, us.first);
                break;
            }
            else {
                self.ctr_upsize_node_2x(path, 0);
                map_so.setup(get<CtrPropertiesMap>(path.root().as_mutable()->allocator(), CTR_PROPERTIES_IDX));
            }
        }
    }

    virtual void remove_ctr_property(U8StringView key)
    {
        auto& self = this->self();
        auto root = self.ctr_get_root_node();
        TreePathT path = TreePathT::build(root, 1);

        CtrPropertiesMap* map = get<CtrPropertiesMap>(path.root().as_mutable()->allocator(), CTR_PROPERTIES_IDX);

        PackedMapSO<CtrPropertiesMap> map_so(map);

        self.ctr_cow_clone_path(path, 0);
        map_so.remove(key);

        self.ctr_downsize_node(path, 0);
    }


    virtual void set_ctr_properties(const std::vector<std::pair<U8String, U8String>>& entries)
    {
        for (const auto& entry: entries) {
            set_ctr_property(entry.first, entry.second);
        }
    }

    virtual void set_ctr_reference(U8StringView key, const CtrID& value)
    {
        auto& self = this->self();
        auto root = self.ctr_get_root_node();
        TreePathT path = TreePathT::build(root, 1);

        CtrReferencesMap* map = get<CtrReferencesMap>(path.root().as_mutable()->allocator(), CTR_REFERENCES_IDX);

        PackedMapSO<CtrReferencesMap> map_so(map);

        self.ctr_cow_clone_path(path, 0);

        while (true)
        {
            auto us = map_so.make_update_state();
            PkdUpdateStatus status = map_so.prepare_set(key, value, us.first);
            if (is_success(status))
            {
                map_so.commit_set(key, value, us.first);
                break;
            }
            else {
                self.ctr_upsize_node_2x(path, 0);
                map_so.setup(get<CtrReferencesMap>(path.root().as_mutable()->allocator(), CTR_REFERENCES_IDX));
            }
        }
    }

    virtual void remove_ctr_reference(U8StringView key)
    {
        auto& self = this->self();
        auto root = self.ctr_get_root_node();
        TreePathT path = TreePathT::build(root, 1);

        CtrReferencesMap* map = get<CtrReferencesMap>(path.root().as_mutable()->allocator(), CTR_REFERENCES_IDX);

        PackedMapSO<CtrReferencesMap> map_so(map);

        self.ctr_cow_clone_path(path, 0);
        map_so.remove(key);

        self.ctr_downsize_node(path, 0);
    }



    virtual void set_ctr_references(const std::vector<std::pair<U8String, CtrID>>& entries)
    {
        for (const auto& entry: entries) {
            set_ctr_reference(entry.first, entry.second);
        }
    }


    SnapshotID snapshot_id() const noexcept {
        return self().store().snaphsot_Id();
    }


    void ctr_root_to_node(const TreeNodePtr& node)
    {
        self().ctr_update_block_guard(node);
        node->set_root(false);
        node->clear_metadata();
    }

    void ctr_node_to_root(const TreeNodePtr& node)
    {
        auto& self = this->self();
        auto root = self.ctr_get_root_node();

        self.ctr_update_block_guard(node);

        node->set_root(true);
        return self.ctr_copy_root_metadata(root, node);
    }

    void ctr_copy_root_metadata(const TreeNodeConstPtr& src, const TreeNodePtr& tgt)
    {
        self().ctr_update_block_guard(tgt);
        return tgt->copy_metadata_from(src.block());
    }

    bool ctr_can_convert_to_root(const TreeNodeConstPtr& node, psize_t metadata_size) const {
        return node->can_convert_to_root(metadata_size);
    }


    template <typename Node>
    void ctr_set_model_name_fn(const Node& node, const CtrID& name)
    {
        node->root_metadata().model_name() = name;
    }

    MEMORIA_V1_CONST_FN_WRAPPER(SetModelNameFn, ctr_set_model_name_fn);

//    static CtrID ctr_get_model_name(const TreeNodeConstPtr& root)
//    {
//        return ctr_get_root_metadata(root).model_name();
//    }

//    static const Metadata& ctr_get_root_metadata(const TreeNodeConstPtr& node)
//    {
//        return node->root_metadata();
//    }

//    static Metadata ctr_get_ctr_root_metadata(const TreeNodeConstPtr& node)
//    {
//        return node->root_metadata();
//    }


    void ctr_set_ctr_root_metadata(const TreeNodePtr& node, const Metadata& metadata) const
    {
        MEMORIA_V1_ASSERT_TRUE(node.isSet());

        self().ctr_update_block_guard(node);
        node->setMetadata(metadata);
    }

//    Metadata ctr_get_root_metadata() const
//    {
//        auto& self          = this->self();
//        const auto& root_id = self.root();

//        auto root = self.ctr_get_block(root_id);
//        return root->root_metadata();
//    }

    void ctr_copy_all_root_metadata_from_to(const TreeNodePtr& from, TreeNodePtr& to) const
    {
        return to->copy_metadata_from(from.block());
    }

    /**
     * \brief Set metadata into root node.
     *
     * \param node Must be a root node
     * \param metadata to set
     */
    void ctr_set_root_metadata(const TreeNodePtr& node, const Metadata& metadata) const
    {
        return ctr_set_ctr_root_metadata(node, metadata);
    }

//    CtrID ctr_get_container_name() const
//    {
//        return ctr_get_root_metadata().model_name();
//    }

    Metadata ctr_create_new_root_metadata() const
    {
        auto& self = this->self();
        Metadata metadata;

        memset(&metadata, 0, sizeof(Metadata));

        metadata.model_name()        = self.name();
        metadata.memory_block_size() = -1;

        return metadata;
    }

//    int32_t get_new_block_size() const
//    {
//        auto& self = this->self();
//        auto root_block = self.ctr_get_root_node();
//        const Metadata* meta = get<const Metadata>(root_block->allocator(), METADATA_IDX);
//        return meta->memory_block_size();
//    }

    void set_new_block_size(int32_t block_size)
    {
        auto& self = this->self();

        auto root_block = self.ctr_get_root_node();

        TreePathT path = TreePathT::build(root_block, 1);
        self.ctr_cow_clone_path(path, 0);

        Metadata* meta = get<Metadata>(path.root().as_mutable()->allocator(), METADATA_IDX);
        meta->memory_block_size() = block_size;
    }



    template <typename Node>
    TreeNodePtr ctr_create_node_fn(int32_t size) const
    {
        auto& self = this->self();
        auto node = static_cast_block<TreeNodePtr>(self.store().createBlock(size, self.name()));
        node->header().block_type_hash() = Node::NodeType::hash();
        return node;
    }


    MEMORIA_V1_CONST_STATIC_FN_WRAPPER_RTN(CreateNodeFn, ctr_create_node_fn, TreeNodePtr);
    TreeNodePtr createNonRootNode(int16_t level, bool leaf, int32_t size = -1) const
    {
        auto& self = this->self();

        if (size == -1)
        {
            auto root_block = self.ctr_get_block(self.root());
            const Metadata* meta = get<const Metadata>(root_block->allocator(), METADATA_IDX);
            size = meta->memory_block_size();
        }

        auto node = self.default_dispatcher().dispatch2(
            leaf,
            CreateNodeFn(self),
            size
        );

        node->header().ctr_type_hash() = self.hash();
        
        node->set_root(false);
        node->set_leaf(leaf);

        node->level() = level;

        ctr_prepare_node(node);

        if (leaf) {
            self.ctr_layout_leaf_node(node);
        }
        else {
            self.ctr_layout_branch_node(node);
        }

        return node;
    }

    MEMORIA_V1_DECLARE_NODE_FN(InitRootMetadataFn, init_root_metadata);
    TreeNodePtr ctr_create_root_node(int16_t level, bool leaf, int32_t size = -1) const
    {
        auto& self = this->self();

        if (size == -1 && self.root().is_set())
        {
            self.root().is_set();

            auto root_block = self.ctr_get_block(self.root());
            const Metadata* meta = get<const Metadata>(root_block->allocator(), METADATA_IDX);
            size = meta->memory_block_size();
        }

        auto node = self.node_dispatcher().dispatch2(
            leaf,
            CreateNodeFn(self), size
        );

        node->header().ctr_type_hash() = self.hash();
        
        node->set_root(true);
        node->set_leaf(leaf);

        node->level() = level;

        ctr_prepare_node(node);

        if (self.root().is_set())
        {
            auto root_block = self.ctr_get_block(self.root());
            node->copy_metadata_from(root_block.block());
        }
        else {
            self.node_dispatcher().dispatch(node, InitRootMetadataFn());

            Metadata& meta = *get<Metadata>(node->allocator(), METADATA_IDX);
            meta = self.ctr_create_new_root_metadata();

            BranchNodeExtDataPkdTuple* branch_tuple = get<BranchNodeExtDataPkdTuple>(
                        node->allocator(), BRANCH_EXT_DATA_IDX
            );

            branch_tuple->set_value(self.branch_node_ext_data());

            LeafNodeExtDataPkdTuple* leaf_tuple = get<LeafNodeExtDataPkdTuple>(
                        node->allocator(), LEAF_EXT_DATA_IDX
            );

            leaf_tuple->set_value(self.leaf_node_ext_data());
        }

        if (leaf) {
            self.ctr_layout_leaf_node(node);
        }
        else {
            self.ctr_layout_branch_node(node);
        }

        return node;
    }

    TreeNodePtr ctr_create_node(int16_t level, bool root, bool leaf, int32_t size = -1) const
    {
        auto& self = this->self();
        if (root) {
            return self.ctr_create_root_node(level, leaf, size);
        }
        else {
            return self.createNonRootNode(level, leaf, size);
        }
    }

    template <typename Node>
    void ctr_prepare_node(Node&& node) const
    {
        return node.prepare();
    }

    MEMORIA_V1_CONST_FN_WRAPPER(PrepareNodeFn, ctr_prepare_node);

    void ctr_prepare_node(TreeNodePtr& node) const
    {
        return self().node_dispatcher().dispatch(node, PrepareNodeFn(self()));
    }







//    static CtrBlockDescription<ApiProfileT> describe_block(const BlockID& node_id, ROAllocator* alloc)
//    {
//        auto tmp = alloc->getBlock(node_id);
//        TreeNodeConstPtr node = tmp;

//        int32_t size = node->header().memory_block_size();
//        bool leaf = node->is_leaf();
//        bool root = node->is_root();

//        uint64_t offset{};
//        return CtrBlockDescription<ApiProfileT>(size, CtrID{}, root, leaf, offset);
//    }

//    void configure_types(
//            const ContainerTypeName& type_name,
//            BranchNodeExtData& branch_node_ext_data,
//            LeafNodeExtData& leaf_node_ext_data
//    ) {
//        MMA_THROW(UnimplementedOperationException());
//    }




 protected:

//    CtrID do_init_ctr(const SharedBlockConstPtr& node)
//    {
//        init_dispatchers();
//        auto& self = this->self();

//        if (node->ctr_type_hash() == CONTAINER_HASH)
//        {
//            TreeNodeConstPtr root_node = node;

//            self.set_root_id(root_node->id());

//            const auto* branch_tuple = get<BranchNodeExtDataPkdTuple>(root_node->allocator(), BRANCH_EXT_DATA_IDX);
//            const auto* leaf_tuple   = get<LeafNodeExtDataPkdTuple>(root_node->allocator(), LEAF_EXT_DATA_IDX);
//            const auto* meta         = get<Metadata>(root_node->allocator(), METADATA_IDX);

//            branch_tuple->get_value(this->branch_node_ext_data_);
//            leaf_tuple->get_value(this->leaf_node_ext_data_);

//            // TODO: Is CtrID correct here?
//            return meta->model_name();
//        }
//        else {
//            MEMORIA_MAKE_GENERIC_ERROR("Invalid container type: {}", node->ctr_type_hash()).do_throw();
//        }
//    }

    void do_create_ctr(const CtrID& ctr_id, const ContainerTypeName& ctr_type_name)
    {
        auto& self = this->self();

        self.init_dispatchers();

        auto has_root = self.store().hasRoot(ctr_id);

        if (has_root)
        {
            MEMORIA_MAKE_GENERIC_ERROR("Container with name {} already exists", ctr_id).do_throw();
        }

        self.configure_types(ctr_type_name, self.branch_node_ext_data(), self.leaf_node_ext_data());

        auto node = self.createRootLeaf();

        return self.set_root(node->id());
    }

private:
//    void init_dispatchers()
//    {
//        node_dispatcher_.set_ctr(&self());
//        leaf_dispatcher_.set_ctr(&self());
//        branch_dispatcher_.set_ctr(&self());
//        default_dispatcher_.set_ctr(&self());
//        tree_dispatcher_.set_ctr(&self());
//    }

MEMORIA_V1_CONTAINER_PART_END


}
