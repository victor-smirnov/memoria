
// Copyright 2011-2025 Victor Smirnov
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

#include <iostream>
#include <type_traits>

namespace memoria {
namespace bt {

MEMORIA_V1_BT_MODEL_BASE_CLASS_BEGIN(BTreeCtrBase)

    using Types = typename Base::Types;

    using ROAllocator = typename Base::ROAllocator;

    using typename Base::SharedBlockPtr;
    using typename Base::SharedBlockConstPtr;
    using typename Base::BlockID;
    using typename Base::CtrID;
    using typename Base::ContainerTypeName;


    using Profile  = typename Types::Profile;
    using ApiProfileT = ApiProfile<Profile>;
    using SnapshotID = ProfileSnapshotID<Profile>;


    using BranchNodeEntry = typename Types::BranchNodeEntry;

    using TreeNodePtr = typename Types::TreeNodePtr;
    using TreeNodeConstPtr = typename Types::TreeNodeConstPtr;

    using TreePathT = TreePath<TreeNodeConstPtr>;

    using Position  = typename Types::Position;
    using CtrSizeT  = typename Types::CtrSizeT;
    using CtrSizesT = typename Types::CtrSizesT;

    using NodeDispatcher    = typename Types::Blocks::template NodeDispatcher<MyType>;
    using LeafDispatcher    = typename Types::Blocks::template LeafDispatcher<MyType>;
    using BranchDispatcher  = typename Types::Blocks::template BranchDispatcher<MyType>;
    using DefaultDispatcher = typename Types::Blocks::template DefaultDispatcher<MyType>;
    using TreeDispatcher    = typename Types::Blocks::template TreeDispatcher<MyType>;

    using Metadata = typename Types::Metadata;

    using BlockUpdateMgr = typename Types::BlockUpdateMgr;

    using LeafNode      = typename Types::LeafNode;
    using BranchNode    = typename Types::BranchNode;

    using LeafNodeSO    = typename LeafNode::template SparseObject<MyType>;
    using BranchNodeSO  = typename BranchNode::template SparseObject<MyType>;

    using LeafNodeExtData   = MakeTuple<typename LeafNodeSO::LeafSubstreamExtensionsList>;
    using BranchNodeExtData = MakeTuple<typename BranchNodeSO::BranchSubstreamExtensionsList>;

    using LeafNodeExtDataPkdTuple   = PackedTuple<LeafNodeExtData>;
    using BranchNodeExtDataPkdTuple = PackedTuple<BranchNodeExtData>;
    using CtrPropertiesMap          = PackedMap<Varchar, Varchar>;
    using CtrReferencesMap          = PackedMap<Varchar, ProfileCtrID<typename Types::Profile>>;

    using BranchUpdateState = typename BranchNodeSO::UpdateState;

    template <typename LeafPath>
    using LeafUpdateState = typename LeafNodeSO::template UpdateState<LeafPath>;

    struct ShuttleTypes: Types {
        using BranchNodeSOType  = BranchNodeSO;
        using LeafNodeSOType    = LeafNodeSO;
        using CtrType           = MyType;
        using IteratorState     = typename Base::BlockIteratorState;
    };

    using Base::CONTAINER_HASH;

    static const int32_t Streams = Types::Streams;

    static const int32_t METADATA_IDX        = 0;
    static const int32_t BRANCH_EXT_DATA_IDX = 1;
    static const int32_t LEAF_EXT_DATA_IDX   = 2;
    static const int32_t CTR_PROPERTIES_IDX  = 3;
    static const int32_t CTR_REFERENCES_IDX  = 4;

    mutable LeafNodeExtData leaf_node_ext_data_;
    mutable BranchNodeExtData branch_node_ext_data_;

    NodeDispatcher node_dispatcher_;
    LeafDispatcher leaf_dispatcher_;
    BranchDispatcher branch_dispatcher_;
    DefaultDispatcher default_dispatcher_;
    TreeDispatcher tree_dispatcher_;

    ObjectPools& pools() const noexcept {return self().allocator().object_pools();}

    TreeNodePtr createRootLeaf() const {
        return self().ctr_create_root_node(0, true, -1);
    }

    const NodeDispatcher& node_dispatcher() const noexcept {
        return node_dispatcher_;
    }

    const LeafDispatcher& leaf_dispatcher() const noexcept {
        return leaf_dispatcher_;
    }

    const BranchDispatcher& branch_dispatcher() const noexcept {
        return branch_dispatcher_;
    }

    const DefaultDispatcher& default_dispatcher() const noexcept {
        return default_dispatcher_;
    }

    const TreeDispatcher& tree_dispatcher() const noexcept {
        return tree_dispatcher_;
    }

//    struct MakeBranchUpdateStateFn {
//        BranchUpdateState treeNode(BranchNodeSO so) {
//            return so.make_update_state();
//        }
//    };

//    BranchUpdateState ctr_make_branch_update_state(const TreeNodeConstPtr& node) const {
//        return branch_dispatcher().dispatch(node, MakeBranchUpdateStateFn());
//    }

//    template <typename LeafPath>
//    struct MakeLeafUpdateStateFn {
//        LeafUpdateState<LeafPath> treeNode(LeafNodeSO so) {
//            return so.template make_update_state<LeafPath>();
//        }
//    };


//    template <typename LeafPath>
//    LeafUpdateState<LeafPath> ctr_make_leaf_update_state(const TreeNodeConstPtr& node) const {
//        return leaf_dispatcher().dispatch(node, MakeLeafUpdateStateFn<LeafPath>());
//    }

    LeafNodeExtData& leaf_node_ext_data() const noexcept {return leaf_node_ext_data_;}
    BranchNodeExtData& branch_node_ext_data() const noexcept {return branch_node_ext_data_;}

//    void ctr_upsize_node(TreePathT& path, size_t level, size_t upsize)
//    {
//        size_t free_space = path[level]->allocator()->free_space();

//        if (free_space < upsize)
//        {
//            size_t memory_block_size = path[level]->header().memory_block_size();

//            size_t total_free_space = free_space;

//            while (total_free_space < upsize)
//            {
//                size_t next_memory_block_size = memory_block_size * 2;
//                size_t additional_free_space  = next_memory_block_size - memory_block_size;

//                memory_block_size = next_memory_block_size;
//                total_free_space += additional_free_space;
//            }

//            self().ctr_cow_clone_path(path, level);
//            return self().ctr_resize_block(path, level, memory_block_size);
//        }
//    }

//    void ctr_upsize_node_2x(TreePathT& path, size_t level)
//    {
//        size_t memory_block_size = path[level]->header().memory_block_size();

//        self().ctr_cow_clone_path(path, level);
//        return self().ctr_resize_block(path, level, memory_block_size * 2);
//    }

//    void ctr_downsize_node(TreePathT& path, size_t level)
//    {
//        size_t memory_block_size = path[level]->header().memory_block_size();

//        size_t used_memory_block_size = path[level]->used_memory_block_size();

//        size_t min_block_size = 8192;

//        if (memory_block_size > min_block_size)
//        {
//            size_t target_memory_block_size = min_block_size;
//            while (target_memory_block_size < used_memory_block_size)
//            {
//                target_memory_block_size *= 2;
//            }

//            self().ctr_cow_clone_path(path, level);
//            return self().ctr_resize_block(path, level, target_memory_block_size);
//        }
//    }

    virtual Optional<U8String> get_ctr_property(U8StringView key) const
    {
        auto& self     = this->self();
        auto root = self.ctr_get_root_node();

        const CtrPropertiesMap* map = get<const CtrPropertiesMap>(root->allocator(), CTR_PROPERTIES_IDX);

        PackedMapSO<CtrPropertiesMap> map_so(const_cast<CtrPropertiesMap*>(map));

        auto res = map_so.find(key);

        if (res) {
            return Optional<U8String>(res.value());
        }
        else {
            return Optional<U8String>{};
        }
    }

//    virtual void set_ctr_property(U8StringView key, U8StringView value)
//    {
//        auto& self = this->self();

//        auto root = self.ctr_get_root_node();

//        TreePathT path = TreePathT::build(root, 1);

//        CtrPropertiesMap* map = get<CtrPropertiesMap>(path.root().as_mutable()->allocator(), CTR_PROPERTIES_IDX);

//        PackedMapSO<CtrPropertiesMap> map_so(map);

//        self.ctr_cow_clone_path(path, 0);

//        while (true)
//        {
//            auto us = map_so.make_update_state();
//            PkdUpdateStatus status = map_so.prepare_set(key, value, us.first);
//            if (is_success(status))
//            {
//                map_so.commit_set(key, value, us.first);
//                break;
//            }
//            else {
//                self.ctr_upsize_node_2x(path, 0);
//                map_so.setup(get<CtrPropertiesMap>(path.root().as_mutable()->allocator(), CTR_PROPERTIES_IDX));
//            }
//        }
//    }

    virtual size_t ctr_properties() const
    {
        auto& self = this->self();
        auto root = self.ctr_get_root_node();

        const CtrPropertiesMap* map = get<const CtrPropertiesMap>(root->allocator(), CTR_PROPERTIES_IDX);

        return map->size();
    }

//    virtual void remove_ctr_property(U8StringView key)
//    {
//        auto& self = this->self();
//        auto root = self.ctr_get_root_node();
//        TreePathT path = TreePathT::build(root, 1);

//        CtrPropertiesMap* map = get<CtrPropertiesMap>(path.root().as_mutable()->allocator(), CTR_PROPERTIES_IDX);

//        PackedMapSO<CtrPropertiesMap> map_so(map);

//        self.ctr_cow_clone_path(path, 0);
//        map_so.remove(key);

//        self.ctr_downsize_node(path, 0);
//    }

    virtual void for_each_ctr_property(std::function<void (U8StringView, U8StringView)> consumer) const
    {
        auto& self = this->self();
        auto root = self.ctr_get_root_node();

        const CtrPropertiesMap* map = get<CtrPropertiesMap>(root->allocator(), CTR_PROPERTIES_IDX);

        PackedMapSO<CtrPropertiesMap> map_so(map);

        map_so.for_each(consumer);
    }

//    virtual void set_ctr_properties(const std::vector<std::pair<U8String, U8String>>& entries)
//    {
//        for (const auto& entry: entries) {
//            set_ctr_property(entry.first, entry.second);
//        }
//    }


    virtual Optional<CtrID> get_ctr_reference(U8StringView key) const
    {
        auto& self = this->self();
        auto root = self.ctr_get_root_node();

        const CtrReferencesMap* map = get<CtrReferencesMap>(root->allocator(), CTR_REFERENCES_IDX);

        PackedMapSO<CtrReferencesMap> map_so(map);

        return map_so.find(key);
    }

//    virtual void set_ctr_reference(U8StringView key, const CtrID& value)
//    {
//        auto& self = this->self();
//        auto root = self.ctr_get_root_node();
//        TreePathT path = TreePathT::build(root, 1);

//        CtrReferencesMap* map = get<CtrReferencesMap>(path.root().as_mutable()->allocator(), CTR_REFERENCES_IDX);

//        PackedMapSO<CtrReferencesMap> map_so(map);

//        self.ctr_cow_clone_path(path, 0);

//        while (true)
//        {
//            auto us = map_so.make_update_state();
//            PkdUpdateStatus status = map_so.prepare_set(key, value, us.first);
//            if (is_success(status))
//            {
//                map_so.commit_set(key, value, us.first);
//                break;
//            }
//            else {
//                self.ctr_upsize_node_2x(path, 0);
//                map_so.setup(get<CtrReferencesMap>(path.root().as_mutable()->allocator(), CTR_REFERENCES_IDX));
//            }
//        }
//    }

//    virtual void remove_ctr_reference(U8StringView key)
//    {
//        auto& self = this->self();
//        auto root = self.ctr_get_root_node();
//        TreePathT path = TreePathT::build(root, 1);

//        CtrReferencesMap* map = get<CtrReferencesMap>(path.root().as_mutable()->allocator(), CTR_REFERENCES_IDX);

//        PackedMapSO<CtrReferencesMap> map_so(map);

//        self.ctr_cow_clone_path(path, 0);
//        map_so.remove(key);

//        self.ctr_downsize_node(path, 0);
//    }

    virtual size_t ctr_references() const
    {
        auto& self = this->self();
        auto root = self.ctr_get_root_node();

        const CtrReferencesMap* map = get<const CtrReferencesMap>(root->allocator(), CTR_REFERENCES_IDX);

        return map->size();
    }

    virtual void for_each_ctr_reference(std::function<void (U8StringView, const CtrID&)> consumer) const
    {
        auto& self = this->self();
        auto root = self.ctr_get_root_node();

        const CtrReferencesMap* map = get<CtrReferencesMap>(root->allocator(), CTR_REFERENCES_IDX);

        PackedMapSO<CtrReferencesMap> map_so(map);

        map_so.for_each(consumer);
    }

//    virtual void set_ctr_references(const std::vector<std::pair<U8String, CtrID>>& entries)
//    {
//        for (const auto& entry: entries) {
//            set_ctr_reference(entry.first, entry.second);
//        }
//    }


    SnapshotID snapshot_id() const noexcept {
        return self().store().snaphsot_Id();
    }


//    void ctr_root_to_node(const TreeNodePtr& node)
//    {
//        self().ctr_update_block_guard(node);
//        node->set_root(false);
//        node->clear_metadata();
//    }

//    void ctr_node_to_root(const TreeNodePtr& node)
//    {
//        auto& self = this->self();
//        auto root = self.ctr_get_root_node();

//        self.ctr_update_block_guard(node);

//        node->set_root(true);
//        return self.ctr_copy_root_metadata(root, node);
//    }

//    void ctr_copy_root_metadata(const TreeNodeConstPtr& src, const TreeNodePtr& tgt)
//    {
//        self().ctr_update_block_guard(tgt);
//        return tgt->copy_metadata_from(src.block());
//    }

//    bool ctr_can_convert_to_root(const TreeNodeConstPtr& node, psize_t metadata_size) const {
//        return node->can_convert_to_root(metadata_size);
//    }



    template <typename Node>
    CtrID ctr_get_model_name_fn(const Node& node) const {
        return node.node()->root_metadata().model_name();
    }

    MEMORIA_V1_CONST_FN_WRAPPER_RTN(GetModelNameFn, ctr_get_model_name_fn, CtrID);


//    template <typename Node>
//    void ctr_set_model_name_fn(const Node& node, const CtrID& name)
//    {
//        node->root_metadata().model_name() = name;
//    }

//    MEMORIA_V1_CONST_FN_WRAPPER(SetModelNameFn, ctr_set_model_name_fn);



    /**
     * \brief Get model name from the root node
     * \param root_id must be a root node ID
     */
    CtrID ctr_get_model_name(const BlockID& root_id) const
    {
        auto& self = this->self();

        auto root = self.ctr_get_block(root_id);

        return self.node_dispatcher().dispatch(root.get(), GetModelNameFn(self));
    }


    static CtrID ctr_get_model_name(const TreeNodeConstPtr& root)
    {
        return ctr_get_root_metadata(root).model_name();
    }

    static const Metadata& ctr_get_root_metadata(const TreeNodeConstPtr& node)
    {
        return node->root_metadata();
    }

    static Metadata ctr_get_ctr_root_metadata(const TreeNodeConstPtr& node)
    {
        return node->root_metadata();
    }


//    void ctr_set_ctr_root_metadata(const TreeNodePtr& node, const Metadata& metadata) const
//    {
//        MEMORIA_V1_ASSERT_TRUE(node.isSet());

//        self().ctr_update_block_guard(node);
//        node->setMetadata(metadata);
//    }

    Metadata ctr_get_root_metadata() const
    {
        auto& self          = this->self();
        const auto& root_id = self.root();

        auto root = self.ctr_get_block(root_id);
        return root->root_metadata();
    }

//    void ctr_copy_all_root_metadata_from_to(const TreeNodePtr& from, TreeNodePtr& to) const
//    {
//        return to->copy_metadata_from(from.block());
//    }

//    /**
//     * \brief Set metadata into root node.
//     *
//     * \param node Must be a root node
//     * \param metadata to set
//     */
//    void ctr_set_root_metadata(const TreeNodePtr& node, const Metadata& metadata) const
//    {
//        return ctr_set_ctr_root_metadata(node, metadata);
//    }

    CtrID ctr_get_container_name() const
    {
        return ctr_get_root_metadata().model_name();
    }

//    Metadata ctr_create_new_root_metadata() const
//    {
//        auto& self = this->self();
//        Metadata metadata;

//        memset(&metadata, 0, sizeof(Metadata));

//        metadata.model_name()        = self.name();
//        metadata.memory_block_size() = -1;

//        return metadata;
//    }

    int32_t get_new_block_size() const
    {
        auto& self = this->self();
        auto root_block = self.ctr_get_root_node();
        const Metadata* meta = get<const Metadata>(root_block->allocator(), METADATA_IDX);
        return meta->memory_block_size();
    }

//    void set_new_block_size(int32_t block_size)
//    {
//        auto& self = this->self();

//        auto root_block = self.ctr_get_root_node();

//        TreePathT path = TreePathT::build(root_block, 1);
//        self.ctr_cow_clone_path(path, 0);

//        Metadata* meta = get<Metadata>(path.root().as_mutable()->allocator(), METADATA_IDX);
//        meta->memory_block_size() = block_size;
//    }



//    template <typename Node>
//    TreeNodePtr ctr_create_node_fn(int32_t size) const
//    {
//        auto& self = this->self();
//        auto node = static_cast_block<TreeNodePtr>(self.store().createBlock(size, self.name()));
//        node->header().block_type_hash() = Node::NodeType::hash();
//        return node;
//    }


//    MEMORIA_V1_CONST_STATIC_FN_WRAPPER_RTN(CreateNodeFn, ctr_create_node_fn, TreeNodePtr);
//    TreeNodePtr createNonRootNode(int16_t level, bool leaf, int32_t size = -1) const
//    {
//        auto& self = this->self();

//        if (size == -1)
//        {
//            auto root_block = self.ctr_get_block(self.root());
//            const Metadata* meta = get<const Metadata>(root_block->allocator(), METADATA_IDX);
//            size = meta->memory_block_size();
//        }

//        auto node = self.default_dispatcher().dispatch2(
//            leaf,
//            CreateNodeFn(self),
//            size
//        );

//        node->header().ctr_type_hash() = self.hash();
        
//        node->set_root(false);
//        node->set_leaf(leaf);

//        node->level() = level;

//        ctr_prepare_node(node);

//        if (leaf) {
//            self.ctr_layout_leaf_node(node);
//        }
//        else {
//            self.ctr_layout_branch_node(node);
//        }

//        return node;
//    }

//    MEMORIA_V1_DECLARE_NODE_FN(InitRootMetadataFn, init_root_metadata);
//    TreeNodePtr ctr_create_root_node(int16_t level, bool leaf, int32_t size = -1) const
//    {
//        auto& self = this->self();

//        if (size == -1 && self.root().is_set())
//        {
//            self.root().is_set();

//            auto root_block = self.ctr_get_block(self.root());
//            const Metadata* meta = get<const Metadata>(root_block->allocator(), METADATA_IDX);
//            size = meta->memory_block_size();
//        }

//        auto node = self.node_dispatcher().dispatch2(
//            leaf,
//            CreateNodeFn(self), size
//        );

//        node->header().ctr_type_hash() = self.hash();
        
//        node->set_root(true);
//        node->set_leaf(leaf);

//        node->level() = level;

//        ctr_prepare_node(node);

//        if (self.root().is_set())
//        {
//            auto root_block = self.ctr_get_block(self.root());
//            node->copy_metadata_from(root_block.block());
//        }
//        else {
//            self.node_dispatcher().dispatch(node, InitRootMetadataFn());

//            Metadata& meta = *get<Metadata>(node->allocator(), METADATA_IDX);
//            meta = self.ctr_create_new_root_metadata();

//            BranchNodeExtDataPkdTuple* branch_tuple = get<BranchNodeExtDataPkdTuple>(
//                        node->allocator(), BRANCH_EXT_DATA_IDX
//            );

//            branch_tuple->set_value(branch_node_ext_data_);

//            LeafNodeExtDataPkdTuple* leaf_tuple = get<LeafNodeExtDataPkdTuple>(
//                        node->allocator(), LEAF_EXT_DATA_IDX
//            );

//            leaf_tuple->set_value(leaf_node_ext_data_);
//        }

//        if (leaf) {
//            self.ctr_layout_leaf_node(node);
//        }
//        else {
//            self.ctr_layout_branch_node(node);
//        }

//        return node;
//    }

//    TreeNodePtr ctr_create_node(int16_t level, bool root, bool leaf, int32_t size = -1) const
//    {
//        auto& self = this->self();
//        if (root) {
//            return self.ctr_create_root_node(level, leaf, size);
//        }
//        else {
//            return self.createNonRootNode(level, leaf, size);
//        }
//    }

//    template <typename Node>
//    void ctr_prepare_node(Node&& node) const
//    {
//        return node.prepare();
//    }

//    MEMORIA_V1_CONST_FN_WRAPPER(PrepareNodeFn, ctr_prepare_node);

//    void ctr_prepare_node(TreeNodePtr& node) const
//    {
//        return self().node_dispatcher().dispatch(node, PrepareNodeFn(self()));
//    }







    static CtrBlockDescription<ApiProfileT> describe_block(const BlockID& node_id, ROAllocator* alloc)
    {
        auto tmp = alloc->getBlock(node_id);
        TreeNodeConstPtr node = tmp;

        int32_t size = node->header().memory_block_size();
        bool leaf = node->is_leaf();
        bool root = node->is_root();

        uint64_t offset{};
        return CtrBlockDescription<ApiProfileT>(size, CtrID{}, root, leaf, offset);
    }

//    void configure_types(
//            const ContainerTypeName& type_name,
//            BranchNodeExtData& branch_node_ext_data,
//            LeafNodeExtData& leaf_node_ext_data
//    ) {
//        MMA_THROW(UnimplementedOperationException());
//    }




 protected:

    CtrID do_init_ctr(const SharedBlockConstPtr& node)
    {
        init_dispatchers();
        auto& self = this->self();

        if (node->ctr_type_hash() == CONTAINER_HASH)
        {
            TreeNodeConstPtr root_node = node;

            self.set_root_id(root_node->id());

            const auto* branch_tuple = get<BranchNodeExtDataPkdTuple>(root_node->allocator(), BRANCH_EXT_DATA_IDX);
            const auto* leaf_tuple   = get<LeafNodeExtDataPkdTuple>(root_node->allocator(), LEAF_EXT_DATA_IDX);
            const auto* meta         = get<Metadata>(root_node->allocator(), METADATA_IDX);

            branch_tuple->get_value(this->branch_node_ext_data_);
            leaf_tuple->get_value(this->leaf_node_ext_data_);

            // TODO: Is CtrID correct here?
            return meta->model_name();
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Invalid container type: {}", node->ctr_type_hash()).do_throw();
        }
    }

//    void do_create_ctr(const CtrID& ctr_id, const ContainerTypeName& ctr_type_name)
//    {
//        init_dispatchers();

//        auto& self = this->self();

//        auto has_root = self.store().hasRoot(ctr_id);

//        if (has_root)
//        {
//            MEMORIA_MAKE_GENERIC_ERROR("Container with name {} already exists", ctr_id).do_throw();
//        }

//        self.configure_types(ctr_type_name, branch_node_ext_data_, leaf_node_ext_data_);

//        auto node = self.createRootLeaf();

//        return self.set_root(node->id());
//    }

public:
    void init_dispatchers()
    {
        node_dispatcher_.set_ctr(&self());
        leaf_dispatcher_.set_ctr(&self());
        branch_dispatcher_.set_ctr(&self());
        default_dispatcher_.set_ctr(&self());
        tree_dispatcher_.set_ctr(&self());
    }

MEMORIA_V1_BT_MODEL_BASE_CLASS_END


}}
