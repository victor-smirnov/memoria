
// Copyright 2011 Victor Smirnov
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
#include <memoria/core/tools/object_pool.hpp>
#include <memoria/prototypes/bt/bt_macros.hpp>
#include <memoria/prototypes/bt/tools/bt_tools_tree_path.hpp>

#include <memoria/core/types/mp11.hpp>
#include <memoria/core/types/list/tuple.hpp>

#include <memoria/core/packed/misc/packed_tuple.hpp>
#include <memoria/core/packed/misc/packed_map.hpp>
#include <memoria/core/packed/misc/packed_map_so.hpp>

#include <iostream>

namespace memoria {
namespace bt {

MEMORIA_V1_BT_MODEL_BASE_CLASS_BEGIN(BTreeCtrBase)

    using Types = typename Base::Types;

    using Allocator = typename Base::Allocator;


    using typename Base::BlockG;
    using typename Base::BlockID;
    using typename Base::CtrID;
    using typename Base::ContainerTypeName;


    using Profile  = typename Types::Profile;
    using ApiProfileT = ApiProfile<Profile>;
    using SnapshotID = ProfileSnapshotID<Profile>;


    using BranchNodeEntry = typename Types::BranchNodeEntry;

    using NodeBaseG = typename Types::NodeBaseG;
    using TreePathT = TreePath<NodeBaseG>;

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

    using Base::CONTAINER_HASH;

    static const int32_t Streams = Types::Streams;

    static const int32_t METADATA_IDX        = 0;
    static const int32_t BRANCH_EXT_DATA_IDX = 1;
    static const int32_t LEAF_EXT_DATA_IDX   = 2;
    static const int32_t CTR_PROPERTIES_IDX  = 3;
    static const int32_t CTR_REFERENCES_IDX  = 4;

    ObjectPools pools_;

    mutable LeafNodeExtData leaf_node_ext_data_;
    mutable BranchNodeExtData branch_node_ext_data_;

    NodeDispatcher node_dispatcher_{self()};
    LeafDispatcher leaf_dispatcher_{self()};
    BranchDispatcher branch_dispatcher_{self()};
    DefaultDispatcher default_dispatcher_{self()};
    TreeDispatcher tree_dispatcher_{self()};


    ObjectPools& pools() {return pools_;}

    Result<NodeBaseG> createRootLeaf() const noexcept {
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



    LeafNodeExtData& leaf_node_ext_data() const noexcept {return leaf_node_ext_data_;}
    BranchNodeExtData& branch_node_ext_data() const noexcept {return branch_node_ext_data_;}

    VoidResult ctr_upsize_node(TreePathT& path, size_t level, size_t upsize) noexcept
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

            MEMORIA_TRY_VOID(self().ctr_cow_clone_path(path, level));
            return self().ctr_resize_block(path, level, memory_block_size);
        }

        return VoidResult::of();
    }

    VoidResult ctr_downsize_node(TreePathT& path, size_t level) noexcept
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

            MEMORIA_TRY_VOID(self().ctr_cow_clone_path(path, level));
            return self().ctr_resize_block(path, level, target_memory_block_size);
        }

        return VoidResult::of();
    }

    virtual Result<Optional<U8String>> get_ctr_property(U8StringView key) const noexcept
    {
        using ResultT = Result<Optional<U8String>>;

        auto& self     = this->self();
        MEMORIA_TRY(root, self.ctr_get_root_node());

        const CtrPropertiesMap* map = get<const CtrPropertiesMap>(root->allocator(), CTR_PROPERTIES_IDX);

        PackedMapSO<CtrPropertiesMap> map_so(const_cast<CtrPropertiesMap*>(map));

        auto res = map_so.find(key);

        if (res) {
            return ResultT::of(Optional<U8String>(res.get()));
        }
        else {
            return ResultT::of(Optional<U8String>());
        }
    }

    virtual VoidResult set_ctr_property(U8StringView key, U8StringView value) noexcept
    {
        return wrap_throwing([&]() -> VoidResult {
            auto& self = this->self();

            MEMORIA_TRY(root, self.ctr_get_root_node());

            TreePathT path = TreePathT::build(root, 1);

            CtrPropertiesMap* map = get<CtrPropertiesMap>(path.root()->allocator(), CTR_PROPERTIES_IDX);

            PackedMapSO<CtrPropertiesMap> map_so(map);

            MEMORIA_TRY_VOID(self.ctr_cow_clone_path(path, 0));

            psize_t upsize = map_so.estimate_required_upsize(key, value);
            if (upsize > map->compute_free_space_up())
            {
                MEMORIA_TRY_VOID(self.ctr_upsize_node(path, 0, upsize));
                map_so.setup(get<CtrPropertiesMap>(path.root()->allocator(), CTR_PROPERTIES_IDX));
            }

            return map_so.set(key, value);
        });
    }

    virtual Result<size_t> ctr_properties() const noexcept
    {
        auto& self = this->self();
        MEMORIA_TRY(root, self.ctr_get_root_node());

        const CtrPropertiesMap* map = get<const CtrPropertiesMap>(root->allocator(), CTR_PROPERTIES_IDX);

        return Result<size_t>::of((size_t)map->size());
    }

    virtual VoidResult remove_ctr_property(U8StringView key) noexcept
    {
        return wrap_throwing([&]() -> VoidResult {
            auto& self = this->self();
            MEMORIA_TRY(root, self.ctr_get_root_node());
            TreePathT path = TreePathT::build(root, 1);

            CtrPropertiesMap* map = get<CtrPropertiesMap>(path.root()->allocator(), CTR_PROPERTIES_IDX);

            PackedMapSO<CtrPropertiesMap> map_so(map);

            MEMORIA_TRY_VOID(self.ctr_cow_clone_path(path, 0));
            MEMORIA_TRY_VOID(map_so.remove(key));

            MEMORIA_TRY_VOID(self.ctr_downsize_node(path, 0));
            return VoidResult::of();
        });
    }

    virtual VoidResult for_each_ctr_property(std::function<void (U8StringView, U8StringView)> consumer) const noexcept
    {
        auto& self = this->self();
        MEMORIA_TRY(root, self.ctr_get_root_node());

        CtrPropertiesMap* map = get<CtrPropertiesMap>(root->allocator(), CTR_PROPERTIES_IDX);

        PackedMapSO<CtrPropertiesMap> map_so(map);

        map_so.for_each(consumer);

        return VoidResult::of();
    }

    virtual VoidResult set_ctr_properties(const std::vector<std::pair<U8String, U8String>>& entries) noexcept
    {
        return wrap_throwing([&]() -> VoidResult {
            auto& self = this->self();
            MEMORIA_TRY(root, self.ctr_get_root_node());
            TreePathT path = TreePathT::build(root, 1);

            CtrPropertiesMap* map = get<CtrPropertiesMap>(path.root()->allocator(), CTR_PROPERTIES_IDX);

            PackedMapSO<CtrPropertiesMap> map_so(map);

            std::vector<std::pair<U8StringView, U8StringView>> entries_view;

            for (auto& entry: entries) {
                entries_view.emplace_back(entry.first, entry.second);
            }

            MEMORIA_TRY_VOID(self.ctr_cow_clone_path(path, 0));

            psize_t upsize = map_so.estimate_required_upsize(entries_view);
            if (upsize > map->compute_free_space_up())
            {
                MEMORIA_TRY_VOID(self.ctr_upsize_node(path, 0, upsize));
                map_so.setup(get<CtrPropertiesMap>(path.root()->allocator(), CTR_PROPERTIES_IDX));
            }

            return map_so.set_all(entries_view);
        });
    }


    virtual Result<Optional<CtrID>> get_ctr_reference(U8StringView key) const noexcept
    {
        using ResultT = Result<Optional<CtrID>>;
        auto& self = this->self();
        MEMORIA_TRY(root, self.ctr_get_root_node());

        CtrReferencesMap* map = get<CtrReferencesMap>(root->allocator(), CTR_REFERENCES_IDX);

        PackedMapSO<CtrReferencesMap> map_so(map);

        return ResultT::of(map_so.find(key));
    }

    virtual VoidResult set_ctr_reference(U8StringView key, const CtrID& value) noexcept
    {
        return wrap_throwing([&]() -> VoidResult {
            auto& self = this->self();
            MEMORIA_TRY(root, self.ctr_get_root_node());
            TreePathT path = TreePathT::build(root, 1);

            CtrReferencesMap* map = get<CtrReferencesMap>(path.root()->allocator(), CTR_REFERENCES_IDX);

            PackedMapSO<CtrReferencesMap> map_so(map);

            MEMORIA_TRY_VOID(self.ctr_cow_clone_path(path, 0));

            psize_t upsize = map_so.estimate_required_upsize(key, value);
            if (upsize > map->compute_free_space_up())
            {
                MEMORIA_TRY_VOID(self.ctr_upsize_node(path, 0, upsize));
                map_so.setup(get<CtrReferencesMap>(path.root()->allocator(), CTR_REFERENCES_IDX));
            }

            return map_so.set(key, value);
        });
    }

    virtual VoidResult remove_ctr_reference(U8StringView key) noexcept
    {
        return wrap_throwing([&]() -> VoidResult {
            auto& self     = this->self();
            MEMORIA_TRY(root, self.ctr_get_root_node());
            TreePathT path = TreePathT::build(root, 1);

            CtrReferencesMap* map = get<CtrReferencesMap>(path.root()->allocator(), CTR_REFERENCES_IDX);

            PackedMapSO<CtrReferencesMap> map_so(map);

            MEMORIA_TRY_VOID(self.ctr_cow_clone_path(path, 0));
            MEMORIA_TRY_VOID(map_so.remove(key));

            return self.ctr_downsize_node(path, 0);
        });
    }

    virtual Result<size_t> ctr_references() const noexcept
    {
        auto& self = this->self();
        MEMORIA_TRY(root, self.ctr_get_root_node());

        const CtrReferencesMap* map = get<const CtrReferencesMap>(root->allocator(), CTR_REFERENCES_IDX);

        return Result<size_t>::of((size_t)map->size());
    }

    virtual VoidResult for_each_ctr_reference(std::function<VoidResult (U8StringView, const CtrID&)> consumer) const noexcept
    {
        auto& self = this->self();
        MEMORIA_TRY(root, self.ctr_get_root_node());

        CtrReferencesMap* map = get<CtrReferencesMap>(root->allocator(), CTR_REFERENCES_IDX);

        PackedMapSO<CtrReferencesMap> map_so(map);

        return map_so.for_each_noexcept(consumer);
    }

    virtual VoidResult set_ctr_references(const std::vector<std::pair<U8String, CtrID>>& entries) noexcept
    {
        return wrap_throwing([&]() -> VoidResult {
            auto& self = this->self();
            MEMORIA_TRY(root, self.ctr_get_root_node());
            TreePathT path = TreePathT::build(root, 1);

            CtrReferencesMap* map = get<CtrReferencesMap>(path.root()->allocator(), CTR_REFERENCES_IDX);

            PackedMapSO<CtrReferencesMap> map_so(map);

            std::vector<std::pair<U8StringView, CtrID>> entries_view;

            for (auto& entry: entries) {
                entries_view.emplace_back(entry.first, entry.second);
            }

            MEMORIA_TRY_VOID(self.ctr_cow_clone_path(path, 0));

            psize_t upsize = map_so.estimate_required_upsize(entries_view);
            if (upsize > map->compute_free_space_up())
            {
                MEMORIA_TRY_VOID(self.ctr_upsize_node(path, 0, upsize));
                map_so.setup(get<CtrReferencesMap>(path.root()->allocator(), CTR_REFERENCES_IDX));
            }

            return map_so.set_all(entries_view);
        });
    }


    SnapshotID snapshot_id() const noexcept {
        return self().store().currentTxnId();
    }


    VoidResult ctr_root_to_node(NodeBaseG& node) noexcept
    {
        MEMORIA_TRY_VOID(self().ctr_update_block_guard(node));

        node->set_root(false);

        MEMORIA_TRY_VOID(node->clear_metadata());

        return VoidResult::of();
    }

    VoidResult ctr_node_to_root(NodeBaseG& node) noexcept
    {
        auto& self = this->self();
        MEMORIA_TRY(root, self.ctr_get_root_node());

        MEMORIA_TRY_VOID(self.ctr_update_block_guard(node));

        node->set_root(true);
        return self.ctr_copy_root_metadata(root, node);
    }

    VoidResult ctr_copy_root_metadata(const NodeBaseG& src, NodeBaseG& tgt) noexcept
    {
        MEMORIA_TRY_VOID(self().ctr_update_block_guard(tgt));
        return tgt->copy_metadata_from(src.block());
    }

    bool ctr_can_convert_to_root(const NodeBaseG& node, psize_t metadata_size) const noexcept
    {
        return node->can_convert_to_root(metadata_size);
    }



    template <typename Node>
    CtrID ctr_get_model_name_fn(Node& node) const noexcept
    {
        return node.node()->root_metadata().model_name();
    }

    MEMORIA_V1_CONST_FN_WRAPPER_RTN(GetModelNameFn, ctr_get_model_name_fn, CtrID);


    template <typename Node>
    void ctr_set_model_name_fn(Node& node, const CtrID& name) noexcept
    {
        node->root_metadata().model_name() = name;
    }

    MEMORIA_V1_CONST_FN_WRAPPER(SetModelNameFn, ctr_set_model_name_fn);



    /**
     * \brief Get model name from the root node
     * \param root_id must be a root node ID
     */
    Result<CtrID> ctr_get_model_name(const BlockID& root_id) const noexcept
    {
        auto& self = this->self();

        MEMORIA_TRY(root, self.ctr_get_block(root_id));

        return self.node_dispatcher().dispatch(root.get(), GetModelNameFn(self));
    }


    static CtrID ctr_get_model_name(NodeBaseG root) noexcept
    {
        return ctr_get_root_metadata(root).model_name();
    }

    static const Metadata& ctr_get_root_metadata(NodeBaseG node) noexcept
    {
        return node->root_metadata();
    }

    static Metadata ctr_get_ctr_root_metadata(NodeBaseG node) noexcept
    {
        return node->root_metadata();
    }


    VoidResult ctr_set_ctr_root_metadata(NodeBaseG& node, const Metadata& metadata) const noexcept
    {
        MEMORIA_V1_ASSERT_TRUE_RTN(node.isSet());

        MEMORIA_TRY_VOID(self().ctr_update_block_guard(node));
        node->setMetadata(metadata);

        return VoidResult::of();
    }

    Result<Metadata> ctr_get_root_metadata() const noexcept
    {
        using ResultT = Result<Metadata>;

        auto& self          = this->self();
        const auto& root_id = self.root();

        MEMORIA_TRY(root, self.ctr_get_block(root_id));
        return ResultT::of(root->root_metadata());
    }

//    VoidResult ctr_set_root_metadata(const Metadata& metadata) const noexcept
//    {
//        NodeBaseG root = self().ctr_get_root_node_for_update();
//        self().setRootMetadata(root, metadata);
//    }

    VoidResult ctr_copy_all_root_metadata_from_to(const NodeBaseG& from, NodeBaseG& to) const noexcept
    {
        return to->copy_metadata_from(from.block());
    }

    /**
     * \brief Set metadata into root node.
     *
     * \param node Must be a root node
     * \param metadata to set
     */
    VoidResult ctr_set_root_metadata(NodeBaseG& node, const Metadata& metadata) const noexcept
    {
        return ctr_set_ctr_root_metadata(node, metadata);
    }

    Result<CtrID> ctr_get_container_name() const
    {
        return Result<CtrID>::of(ctr_get_root_metadata().model_name());
    }

    Metadata ctr_create_new_root_metadata() const noexcept
    {
        auto& self = this->self();
        Metadata metadata;

        memset(&metadata, 0, sizeof(Metadata));

        metadata.model_name()        = self.name();
        metadata.memory_block_size() = -1;

        return metadata;
    }

    Int32Result get_new_block_size() const noexcept
    {
        auto& self = this->self();
        MEMORIA_TRY(root_block, self.ctr_get_root_node());
        const Metadata* meta = get<const Metadata>(root_block->allocator(), METADATA_IDX);
        return Result<int32_t>::of(meta->memory_block_size());
    }

    VoidResult set_new_block_size(int32_t block_size) noexcept
    {
        auto& self = this->self();

        MEMORIA_TRY(root_block, self.ctr_get_root_node());

        TreePathT path = TreePathT::build(root_block, 1);
        MEMORIA_TRY_VOID(self.ctr_cow_clone_path(path, 0));

        Metadata* meta = get<Metadata>(path.root()->allocator(), METADATA_IDX);
        meta->memory_block_size() = block_size;

        return VoidResult::of();
    }



    template <typename Node>
    Result<NodeBaseG> ctr_create_node_fn(int32_t size) const noexcept
    {
        using ResultT = Result<NodeBaseG>;
        auto& self = this->self();

        MEMORIA_TRY(node, static_cast_block<NodeBaseG>(self.store().createBlock(size)));

        node->init();
        node->header().block_type_hash() = Node::NodeType::hash();

        return ResultT::of(node);
    }


    MEMORIA_V1_CONST_STATIC_FN_WRAPPER_RTN(CreateNodeFn, ctr_create_node_fn, Result<NodeBaseG>);
    Result<NodeBaseG> createNonRootNode(int16_t level, bool leaf, int32_t size = -1) const noexcept
    {
        using ResultT = Result<NodeBaseG>;
        auto& self = this->self();

        if (size == -1)
        {
            MEMORIA_TRY(root_block, self.ctr_get_block(self.root()));
            const Metadata* meta = get<const Metadata>(root_block->allocator(), METADATA_IDX);
            size = meta->memory_block_size();
        }

        MEMORIA_TRY(node, self.default_dispatcher().dispatch2(
                        leaf,
                        CreateNodeFn(self),
						size
        ));

        node->header().ctr_type_hash() = self.hash();
        
        node->set_root(false);
        node->set_leaf(leaf);

        node->level() = level;

        MEMORIA_TRY_VOID(ctr_prepare_node(node));

        if (leaf) {
            MEMORIA_TRY_VOID(self.ctr_layout_leaf_node(node, Position()));
        }
        else {
            MEMORIA_TRY_VOID(self.ctr_layout_branch_node(node, -1ull));
        }

        return ResultT::of(node);
    }

    MEMORIA_V1_DECLARE_NODE_FN(InitRootMetadataFn, init_root_metadata);
    Result<NodeBaseG> ctr_create_root_node(int16_t level, bool leaf, int32_t size = -1) const noexcept
    {
        using ResultT = Result<NodeBaseG>;
        auto& self = this->self();

        if (size == -1 && self.root().is_set())
        {
            MEMORIA_TRY(root_block, self.ctr_get_block(self.root()));
            const Metadata* meta = get<const Metadata>(root_block->allocator(), METADATA_IDX);
            size = meta->memory_block_size();
        }

        MEMORIA_TRY(node, self.node_dispatcher().dispatch2(
            leaf,
            CreateNodeFn(self), size
        ));

        node->header().ctr_type_hash() = self.hash();
        
        node->set_root(true);
        node->set_leaf(leaf);

        node->level() = level;

        MEMORIA_TRY_VOID(ctr_prepare_node(node));

        if (self.root().is_set())
        {
            MEMORIA_TRY(root_block, self.ctr_get_block(self.root()));
            MEMORIA_TRY_VOID(node->copy_metadata_from(root_block.block()));
        }
        else {
            MEMORIA_TRY_VOID(self.node_dispatcher().dispatch(node, InitRootMetadataFn()));

            Metadata& meta = *get<Metadata>(node->allocator(), METADATA_IDX);
            meta = self.ctr_create_new_root_metadata();

            BranchNodeExtDataPkdTuple* branch_tuple = get<BranchNodeExtDataPkdTuple>(
                        node->allocator(), BRANCH_EXT_DATA_IDX
            );

            branch_tuple->set_value(branch_node_ext_data_);

            LeafNodeExtDataPkdTuple* leaf_tuple = get<LeafNodeExtDataPkdTuple>(
                        node->allocator(), LEAF_EXT_DATA_IDX
            );

            leaf_tuple->set_value(leaf_node_ext_data_);
        }

        if (leaf) {
            MEMORIA_TRY_VOID(self.ctr_layout_leaf_node(node, Position()));
        }
        else {
            MEMORIA_TRY_VOID(self.ctr_layout_branch_node(node, -1ull));
        }

        return ResultT::of(node);
    }

    Result<NodeBaseG> ctr_create_node(int16_t level, bool root, bool leaf, int32_t size = -1) const noexcept
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
    VoidResult ctr_prepare_node(Node&& node) const noexcept
    {
        return node.prepare();
    }

    MEMORIA_V1_CONST_FN_WRAPPER(PrepareNodeFn, ctr_prepare_node);

    VoidResult ctr_prepare_node(NodeBaseG& node) const noexcept
    {
        return self().node_dispatcher().dispatch(node, PrepareNodeFn(self()));
    }







    static Result<CtrBlockDescription<ApiProfileT>> describe_block(const BlockID& node_id, Allocator* alloc) noexcept
    {
        MEMORIA_TRY(tmp, alloc->getBlock(node_id));
        NodeBaseG node = tmp;

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

    Result<CtrID> do_init_ctr(const BlockG& node) noexcept
    {
        using ResultT = Result<CtrID>;
        auto& self = this->self();

        if (node->ctr_type_hash() == CONTAINER_HASH)
        {
            NodeBaseG root_node = node;

            self.set_root_id(root_node->id());

            const auto* branch_tuple = get<BranchNodeExtDataPkdTuple>(root_node->allocator(), BRANCH_EXT_DATA_IDX);
            const auto* leaf_tuple   = get<LeafNodeExtDataPkdTuple>(root_node->allocator(), LEAF_EXT_DATA_IDX);
            const auto* meta         = get<Metadata>(root_node->allocator(), METADATA_IDX);

            branch_tuple->get_value(this->branch_node_ext_data_);
            leaf_tuple->get_value(this->leaf_node_ext_data_);

            // TODO: Is CtrID correct here?
            return ResultT::of(meta->model_name());
        }
        else {
            return MEMORIA_MAKE_GENERIC_ERROR("Invalid container type: {}", node->ctr_type_hash());
        }
    }

    VoidResult do_create_ctr(const CtrID& ctr_id, const ContainerTypeName& ctr_type_name) noexcept
    {
        auto& self = this->self();

        MEMORIA_TRY(has_root, self.store().hasRoot(ctr_id));

        if (has_root)
        {
            return MEMORIA_MAKE_GENERIC_ERROR("Container with name {} already exists", ctr_id);
        }

        self.configure_types(ctr_type_name, branch_node_ext_data_, leaf_node_ext_data_);

        MEMORIA_TRY(node, self.createRootLeaf());

        return self.set_root(node->id());
    }


MEMORIA_V1_BT_MODEL_BASE_CLASS_END


}}
