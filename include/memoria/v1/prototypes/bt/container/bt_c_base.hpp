
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

#include <memoria/v1/core/container/container.hpp>
#include <memoria/v1/core/container/names.hpp>
#include <memoria/v1/core/types/algo.hpp>
#include <memoria/v1/core/tools/static_array.hpp>
#include <memoria/v1/core/container/macros.hpp>
#include <memoria/v1/core/tools/object_pool.hpp>
#include <memoria/v1/prototypes/bt/bt_macros.hpp>

#include <memoria/v1/core/types/mp11.hpp>
#include <memoria/v1/core/types/list/tuple.hpp>

#include <memoria/v1/core/packed/misc/packed_tuple.hpp>
#include <memoria/v1/core/packed/misc/packed_map.hpp>
#include <memoria/v1/core/packed/misc/packed_map_so.hpp>

#include <iostream>

namespace memoria {
namespace v1 {
namespace bt     {

MEMORIA_V1_BT_MODEL_BASE_CLASS_BEGIN(BTreeCtrBase)

    using Types = typename Base::Types;

    using Allocator = typename Base::Allocator;


    using typename Base::BlockG;
    using typename Base::BlockID;
    using typename Base::CtrID;
    using typename Base::ContainerTypeName;

    using ProfileT = typename Types::Profile;

    using BranchNodeEntry = typename Types::BranchNodeEntry;

    using NodeBaseG = typename Types::NodeBaseG;

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

    LeafNodeExtData leaf_node_ext_data_;
    BranchNodeExtData branch_node_ext_data_;

    NodeDispatcher node_dispatcher_{self()};
    LeafDispatcher leaf_dispatcher_{self()};
    BranchDispatcher branch_dispatcher_{self()};
    DefaultDispatcher default_dispatcher_{self()};
    TreeDispatcher tree_dispatcher_{self()};


    ObjectPools& pools() {return pools_;}

    BlockG createRootLeaf() const {
        return self().ctr_create_root_node(0, true, -1);
    }

    const NodeDispatcher& node_dispatcher() const {
        return node_dispatcher_;
    }

    const LeafDispatcher& leaf_dispatcher() const {
        return leaf_dispatcher_;
    }

    const BranchDispatcher& branch_dispatcher() const {
        return branch_dispatcher_;
    }

    const DefaultDispatcher& default_dispatcher() const {
        return default_dispatcher_;
    }

    const TreeDispatcher& tree_dispatcher() const {
        return tree_dispatcher_;
    }



    const LeafNodeExtData& leaf_node_ext_data() const {return leaf_node_ext_data_;}
    const BranchNodeExtData& branch_node_ext_data() const {return branch_node_ext_data_;}

    void ctr_upsize_node(NodeBaseG node, size_t upsize)
    {
        size_t free_space = node->allocator()->free_space();

        if (free_space < upsize)
        {
            size_t memory_block_size = node->header().memory_block_size();

            size_t total_free_space = free_space;

            while (total_free_space < upsize)
            {
                size_t next_memory_block_size = memory_block_size * 2;
                size_t additional_free_space  = next_memory_block_size - memory_block_size;

                memory_block_size = next_memory_block_size;
                total_free_space += additional_free_space;
            }

            node.resize(memory_block_size);
        }
    }

    void ctr_downsize_node(NodeBaseG node)
    {
        size_t memory_block_size = node->header().memory_block_size();

        size_t used_memory_block_size = node->used_memory_block_size();

        size_t min_block_size = 8192;

        if (memory_block_size > min_block_size)
        {
            size_t target_memory_block_size = min_block_size;
            while (target_memory_block_size < used_memory_block_size)
            {
                target_memory_block_size *= 2;
            }

            node.resize(target_memory_block_size);
        }
    }

    virtual Optional<U8String> get_ctr_property(U8StringView key) const
    {
        auto& self     = this->self();
        NodeBaseG root = self.ctr_get_root_node();

        const CtrPropertiesMap* map = get<const CtrPropertiesMap>(root->allocator(), CTR_PROPERTIES_IDX);

        PackedMapSO<CtrPropertiesMap> map_so(const_cast<CtrPropertiesMap*>(map));

        auto res = map_so.find(key);

        return res ? Optional<U8String>(res.get()) : Optional<U8String>();
    }

    virtual void set_ctr_property(U8StringView key, U8StringView value)
    {
        auto& self     = this->self();
        NodeBaseG root = self.ctr_get_root_node();
        CtrPropertiesMap* map = get<CtrPropertiesMap>(root->allocator(), CTR_PROPERTIES_IDX);

        PackedMapSO<CtrPropertiesMap> map_so(map);

        psize_t upsize = map_so.estimate_required_upsize(key, value);
        if (upsize > map->compute_free_space_up())
        {
            self.ctr_upsize_node(root, upsize);

            map_so.setup(get<CtrPropertiesMap>(root->allocator(), CTR_PROPERTIES_IDX));
        }

        OOM_THROW_IF_FAILED(map_so.set(key, value), MMA1_SRC);
    }

    virtual size_t ctr_properties() const
    {
        auto& self     = this->self();
        NodeBaseG root = self.ctr_get_root_node();
        const CtrPropertiesMap* map = get<const CtrPropertiesMap>(root->allocator(), CTR_PROPERTIES_IDX);

        return map->size();
    }

    virtual void remove_ctr_property(U8StringView key)
    {
        auto& self     = this->self();
        NodeBaseG root = self.ctr_get_root_node();

        CtrPropertiesMap* map = get<CtrPropertiesMap>(root->allocator(), CTR_PROPERTIES_IDX);

        PackedMapSO<CtrPropertiesMap> map_so(map);

        OOM_THROW_IF_FAILED(map_so.remove(key), MMA1_SRC);

        self.ctr_downsize_node(root);
    }

    virtual void for_each_ctr_property(std::function<void (U8StringView, U8StringView)> consumer) const
    {
        auto& self     = this->self();
        NodeBaseG root = self.ctr_get_root_node();

        CtrPropertiesMap* map = get<CtrPropertiesMap>(root->allocator(), CTR_PROPERTIES_IDX);

        PackedMapSO<CtrPropertiesMap> map_so(map);

        map_so.for_each(consumer);
    }

    virtual void set_ctr_properties(const std::vector<std::pair<U8String, U8String>>& entries)
    {
        auto& self     = this->self();
        NodeBaseG root = self.ctr_get_root_node();

        CtrPropertiesMap* map = get<CtrPropertiesMap>(root->allocator(), CTR_PROPERTIES_IDX);

        PackedMapSO<CtrPropertiesMap> map_so(map);

        std::vector<std::pair<U8StringView, U8StringView>> entries_view;

        for (auto& entry: entries) {
            entries_view.emplace_back(entry.first, entry.second);
        }

        psize_t upsize = map_so.estimate_required_upsize(entries_view);
        if (upsize > map->compute_free_space_up())
        {
            self.ctr_upsize_node(root, upsize);

            map_so.setup(get<CtrPropertiesMap>(root->allocator(), CTR_PROPERTIES_IDX));
        }

        OOM_THROW_IF_FAILED(map_so.set_all(entries_view), MMA1_SRC);
    }


    virtual Optional<CtrID> get_ctr_reference(U8StringView key) const {
        auto& self     = this->self();
        NodeBaseG root = self.ctr_get_root_node();

        CtrReferencesMap* map = get<CtrReferencesMap>(root->allocator(), CTR_REFERENCES_IDX);

        PackedMapSO<CtrReferencesMap> map_so(map);

        return map_so.find(key);
    }

    virtual void set_ctr_reference(U8StringView key, const CtrID& value)
    {
        auto& self     = this->self();
        NodeBaseG root = self.ctr_get_root_node();
        CtrReferencesMap* map = get<CtrReferencesMap>(root->allocator(), CTR_REFERENCES_IDX);

        PackedMapSO<CtrReferencesMap> map_so(map);

        psize_t upsize = map_so.estimate_required_upsize(key, value);
        if (upsize > map->compute_free_space_up())
        {
            self.ctr_upsize_node(root, upsize);

            map_so.setup(get<CtrReferencesMap>(root->allocator(), CTR_REFERENCES_IDX));
        }

        OOM_THROW_IF_FAILED(map_so.set(key, value), MMA1_SRC);
    }

    virtual void remove_ctr_reference(U8StringView key) {
        auto& self     = this->self();
        NodeBaseG root = self.ctr_get_root_node();

        CtrReferencesMap* map = get<CtrReferencesMap>(root->allocator(), CTR_REFERENCES_IDX);

        PackedMapSO<CtrReferencesMap> map_so(map);

        OOM_THROW_IF_FAILED(map_so.remove(key), MMA1_SRC);

        self.ctr_downsize_node(root);
    }

    virtual size_t ctr_references() const
    {
        auto& self = this->self();
        NodeBaseG root = self.ctr_get_root_node();
        const CtrReferencesMap* map = get<const CtrReferencesMap>(root->allocator(), CTR_REFERENCES_IDX);

        return map->size();
    }

    virtual void for_each_ctr_reference(std::function<void (U8StringView, const CtrID&)> consumer) const {
        auto& self = this->self();
        NodeBaseG root = self.ctr_get_root_node();
        CtrReferencesMap* map = get<CtrReferencesMap>(root->allocator(), CTR_REFERENCES_IDX);

        PackedMapSO<CtrReferencesMap> map_so(map);

        map_so.for_each(consumer);
    }

    virtual void set_ctr_references(const std::vector<std::pair<U8String, CtrID>>& entries)
    {
        auto& self     = this->self();
        NodeBaseG root = self.ctr_get_root_node();

        CtrReferencesMap* map = get<CtrReferencesMap>(root->allocator(), CTR_REFERENCES_IDX);

        PackedMapSO<CtrReferencesMap> map_so(map);

        std::vector<std::pair<U8StringView, CtrID>> entries_view;

        for (auto& entry: entries) {
            entries_view.emplace_back(entry.first, entry.second);
        }

        psize_t upsize = map_so.estimate_required_upsize(entries_view);
        if (upsize > map->compute_free_space_up())
        {
            self.ctr_upsize_node(root, upsize);

            map_so.setup(get<CtrReferencesMap>(root->allocator(), CTR_REFERENCES_IDX));
        }

        OOM_THROW_IF_FAILED(map_so.set_all(entries_view), MMA1_SRC);
    }




    void ctr_root_to_node(NodeBaseG& node) const
    {
        self().ctr_update_block_guard(node);

        node->set_root(false);

        node->clear_metadata();
    }

    void ctr_node_to_root(NodeBaseG& node, const Metadata& meta) const
    {
        self().ctr_update_block_guard(node);

        node->set_root(true);

        node->parent_id().clear();
        node->parent_idx() = 0;

        node->setMetadata(meta);
    }

    void ctr_copy_root_metadata(NodeBaseG& src, NodeBaseG& tgt) const
    {
        self().ctr_update_block_guard(tgt);
        tgt->copy_metadata_from(src);
    }

    bool ctr_can_convert_to_root(const NodeBaseG& node, psize_t metadata_size) const
    {
        return node->can_convert_to_root(metadata_size);
    }



    template <typename Node>
    CtrID ctr_get_model_name_fn(Node& node) const
    {
        return node.node()->root_metadata().model_name();
    }

    MEMORIA_V1_CONST_FN_WRAPPER_RTN(GetModelNameFn, ctr_get_model_name_fn, CtrID);


    template <typename Node>
    void ctr_set_model_name_fn(Node& node, const CtrID& name)
    {
        node->root_metadata().model_name() = name;
    }

    MEMORIA_V1_CONST_FN_WRAPPER(SetModelNameFn, ctr_set_model_name_fn);



    /**
     * \brief Get model name from the root node
     * \param root_id must be a root node ID
     */
    CtrID ctr_get_model_name(const BlockID& root_id) const
    {
        MEMORIA_V1_ASSERT_NOT_EMPTY(root_id);

        auto& self = this->self();

        NodeBaseG root = self.store().getBlock(root_id);

        return self.node_dispatcher().dispatch(root, GetModelNameFn(self));
    }

    static CtrID ctr_get_model_name(NodeBaseG root)
    {
        return ctr_get_root_metadata(root).model_name();
    }

    static const Metadata& ctr_get_root_metadata(NodeBaseG node)
    {
        MEMORIA_V1_ASSERT_TRUE(node.isSet());
        MEMORIA_V1_ASSERT_TRUE(node->is_root());

        return node->root_metadata();
    }

    void ctr_set_model_name(const CtrID& name)
    {
        NodeBaseG root = self().ctr_get_root_node();

        self().node_dispatcher().dispatch(root, SetModelNameFn(self()), name);
    }



    MEMORIA_V1_FN_WRAPPER_RTN(SetRootIdFn, setRootIdFn, Metadata);


    static Metadata ctr_get_ctr_root_metadata(NodeBaseG node)
    {
        MEMORIA_V1_ASSERT_TRUE(node.isSet());
        MEMORIA_V1_ASSERT_TRUE(node->has_root_metadata());

        return node->root_metadata();
    }


    void ctr_set_ctr_root_metadata(NodeBaseG& node, const Metadata& metadata) const
    {
        MEMORIA_V1_ASSERT_TRUE(node.isSet());

        self().ctr_update_block_guard(node);
        node->setMetadata(metadata);
    }

    Metadata ctr_get_root_metadata() const
    {
        auto& self          = this->self();
        const auto& root_id = self.root();
        NodeBaseG root      = self.store().getBlock(root_id);

        return root->root_metadata();
    }

    void ctr_set_root_metadata(const Metadata& metadata) const
    {
        NodeBaseG root = self().ctr_get_root_node_for_update();
        self().setRootMetadata(root, metadata);
    }

    /**
     * \brief Set metadata into root node.
     *
     * \param node Must be a root node
     * \param metadata to set
     */
    void ctr_set_root_metadata(NodeBaseG& node, const Metadata& metadata) const
    {
        ctr_set_ctr_root_metadata(node, metadata);
    }

    CtrID ctr_get_container_name() const
    {
        return ctr_get_root_metadata().model_name();
    }

    Metadata ctr_create_new_root_metadata() const
    {
        Metadata metadata;

        memset(&metadata, 0, sizeof(Metadata));

        metadata.model_name()        = self().name();
        metadata.memory_block_size() = DEFAULT_BLOCK_SIZE;

        return metadata;
    }

    int32_t get_new_block_size() const
    {
        NodeBaseG root_block = self().store().getBlockForUpdate(self().root());
        const Metadata* meta = get<const Metadata>(root_block->allocator(), METADATA_IDX);
        return meta->memory_block_size();
    }

    void set_new_block_size(int32_t block_size)
    {
        NodeBaseG root_block = self().store().getBlockForUpdate(self().root());
        Metadata* meta = get<Metadata>(root_block->allocator(), METADATA_IDX);
        meta->memory_block_size() = block_size;
    }



    template <typename Node>
    NodeBaseG ctr_create_node_fn(int32_t size) const
    {
        auto& self = this->self();

        NodeBaseG node = self.store().createBlock(size);
        node->init();

        node->header().block_type_hash() = Node::NodeType::hash();

        return node;
    }


    MEMORIA_V1_CONST_STATIC_FN_WRAPPER_RTN(CreateNodeFn, ctr_create_node_fn, NodeBaseG);
    NodeBaseG createNonRootNode(int16_t level, bool leaf, int32_t size = -1) const
    {
        MEMORIA_V1_ASSERT(level, >=, 0);

        auto& self = this->self();

        if (size == -1)
        {
            NodeBaseG root_block = self.store().getBlock(self.root());
            const Metadata* meta = get<const Metadata>(root_block->allocator(), METADATA_IDX);
            size = meta->memory_block_size();
        }

        NodeBaseG node = self.default_dispatcher().dispatch2(
                        leaf,
                        CreateNodeFn(self),
						size
        );

        node->header().ctr_type_hash() = self.hash();
        
        node->parent_id()  = BlockID{};
        node->parent_idx() = 0;

        node->set_root(false);
        node->set_leaf(leaf);

        node->level() = level;

        ctr_prepare_node(node);

        if (leaf) {
            self.ctr_layout_leaf_node(node, Position());
        }
        else {
            self.ctr_layout_branch_node(node, -1ull);
        }

        return node;
    }

    MEMORIA_V1_DECLARE_NODE_FN(InitRootMetadataFn, init_root_metadata);
    NodeBaseG ctr_create_root_node(int16_t level, bool leaf, int32_t size = -1) const
    {
        MEMORIA_V1_ASSERT(level, >=, 0);

        auto& self = this->self();

        NodeBaseG root_block = self.store().getBlock(self.root());

        if (size == -1)
        {
            if (root_block) {
                const Metadata* meta = get<const Metadata>(root_block->allocator(), METADATA_IDX);
                size = meta->memory_block_size();
            }
            else {
                size = DEFAULT_BLOCK_SIZE;
            }
        }

        NodeBaseG node = self.node_dispatcher().dispatch2(
            leaf,
            CreateNodeFn(self), size
        );

        node->header().ctr_type_hash() = self.hash();
        
        node->parent_id()  = BlockID{};
        node->parent_idx() = 0;

        node->set_root(true);
        node->set_leaf(leaf);

        node->level() = level;

        ctr_prepare_node(node);

        if (root_block) {
            node->copy_metadata_from(root_block);
        }
        else {
            self.node_dispatcher().dispatch(node, InitRootMetadataFn());

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
            self.ctr_layout_leaf_node(node, Position());
        }
        else {
            self.ctr_layout_branch_node(node, -1ull);
        }

        return node;
    }

    NodeBaseG ctr_create_node(int16_t level, bool root, bool leaf, int32_t size = -1) const
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
        node.prepare();
    }

    MEMORIA_V1_CONST_FN_WRAPPER(PrepareNodeFn, ctr_prepare_node);

    void ctr_prepare_node(NodeBaseG& node) const
    {
        MEMORIA_V1_ASSERT_TRUE(node.isSet());
        self().node_dispatcher().dispatch(node, PrepareNodeFn(self()));
    }


    void ctr_update_block_guard(NodeBaseG& node) const
    {
        node.update();
    }

    MEMORIA_V1_DECLARE_NODE_FN_RTN(ValuesAsVectorFn, template values_as_vector<BlockID>, std::vector<BlockID>);
    Collection<Edge> describe_block_links(const BlockID& block_id, Direction direction)
    {
        std::vector<Edge> links;

        NodeBaseG block = this->allocator_holder_->getBlock(block_id);

        Graph graph     = this->graph();
        Vertex ctr_vx   = this->as_vertex();
        Vertex block_vx = this->block_as_vertex(block_id);

        if (is_in(direction))
        {
            if (!block->is_root())
            {
                links.push_back(DefaultEdge::make(graph, "child", this->block_as_vertex(block->parent_id()), block_vx));
            }
            else {
                links.push_back(DefaultEdge::make(graph, "root", ctr_vx, block_vx));
            }
        }

        if (is_out(direction))
        {
            if (!block->is_leaf())
            {
                auto child_ids = self().branch_dispatcher().dispatch(block, ValuesAsVectorFn());
                for (auto& child_id: child_ids)
                {
                    links.push_back(DefaultEdge::make(graph, "child", block_vx, this->block_as_vertex(child_id)));
                }
            }
        }

        return STLCollection<Edge>::make(std::move(links));
    }

    Collection<VertexProperty> block_properties(const Vertex& vx, const BlockID& block_id)
    {
        NodeBaseG block = this->allocator_holder_->getBlock(block_id);

        std::vector<VertexProperty> props;

        CtrBlockType block_type{};

        if (block->is_root() && block->is_leaf()) {
            block_type = CtrBlockType::ROOT_LEAF;
        }
        else if (block->is_root() && !block->is_leaf()) {
            block_type = CtrBlockType::ROOT;
        }
        else if ((!block->is_root()) && block->is_leaf()) {
            block_type = CtrBlockType::LEAF;
        }
        else {
            block_type = CtrBlockType::INTERNAL;
        }

        props.emplace_back(DefaultVertexProperty::make(vx, "type", block_type));

        std::stringstream buf;
        self().ctr_dump_node(block, buf);

        props.emplace_back(DefaultVertexProperty::make(vx, "content", U8String(buf.str())));

        return STLCollection<VertexProperty>::make(std::move(props));
    }




    static CtrBlockDescription<ProfileT> describe_block(const BlockID& node_id, Allocator* alloc)
    {
        NodeBaseG node = alloc->getBlock(node_id);

        int32_t size = node->header().memory_block_size();
        bool leaf = node->is_leaf();
        bool root = node->is_root();

        uint64_t offset{};

        while (!node->is_root())
        {
            offset += node->parent_idx();
            node = alloc->getBlock(node->parent_id());
        }

        return CtrBlockDescription<ProfileT>(size, ctr_get_model_name(node), root, leaf, offset);
    }

//    void configure_types(
//            const ContainerTypeName& type_name,
//            BranchNodeExtData& branch_node_ext_data,
//            LeafNodeExtData& leaf_node_ext_data
//    ) {
//        MMA1_THROW(UnimplementedOperationException());
//    }

 protected:

    CtrID do_init_ctr(const BlockG& node)
    {
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

            return meta->model_name();
        }
        else {
            MMA1_THROW(CtrTypeException()) << WhatInfo(format8("Invalid container type: {}", node->ctr_type_hash()));
        }
    }

    void do_create_ctr(const CtrID& ctr_id, const ContainerTypeName& ctr_type_name)
    {
        auto& self = this->self();

        if (self.store().hasRoot(ctr_id))
        {
            MMA1_THROW(NoCtrException()) << WhatInfo(format8("Container with name {} already exists", ctr_id));
        }

        self.configure_types(ctr_type_name, branch_node_ext_data_, leaf_node_ext_data_);

        NodeBaseG node = self.createRootLeaf();

        self.set_root(node->id());
    }


MEMORIA_V1_BT_MODEL_BASE_CLASS_END


}}}
