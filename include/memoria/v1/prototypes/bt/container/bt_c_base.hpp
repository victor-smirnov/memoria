
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




#include <iostream>
#include "../../../core/tools/pair.hpp"

namespace memoria {
namespace v1 {
namespace bt     {

MEMORIA_V1_BT_MODEL_BASE_CLASS_BEGIN(BTreeCtrBase)

    using Types = typename Base::Types;

    using Allocator = typename Base::Allocator;

    using typename Base::Page;
    using typename Base::PageG;
    using typename Base::ID;

    using BranchNodeEntry = typename Types::BranchNodeEntry;

    using NodeBase  = typename Types::NodeBase;
    using NodeBaseG = typename Types::NodeBaseG;

    using Position  = typename Types::Position;
    using CtrSizeT  = typename Types::CtrSizeT;
    using CtrSizesT = typename Types::CtrSizesT;

    using NodeDispatcher    = typename Types::Pages::NodeDispatcher;
    using LeafDispatcher    = typename Types::Pages::LeafDispatcher;
    using BranchDispatcher  = typename Types::Pages::BranchDispatcher;
    using DefaultDispatcher = typename Types::Pages::DefaultDispatcher;

    using Metadata = typename Types::Metadata;

    using PageUpdateMgr = typename Types::PageUpdateMgr;

    using Base::CONTAINER_HASH;

    static const int32_t Streams = Types::Streams;

    ObjectPools pools_;


    ObjectPools& pools() {return pools_;}

    PageG createRoot() const {
        return self().createNode(0, true, true);
    }



    template <typename Node>
    UUID getModelNameFn(const Node* node) const
    {
        return node->root_metadata().model_name();
    }

    MEMORIA_V1_CONST_FN_WRAPPER_RTN(GetModelNameFn, getModelNameFn, UUID);


    template <typename Node>
    void setModelNameFn(Node* node, const UUID& name)
    {
        node->root_metadata().model_name() = name;
    }

    MEMORIA_V1_CONST_FN_WRAPPER(SetModelNameFn, setModelNameFn);



    /**
     * \brief Get model name from the root node
     * \param root_id must be a root node ID
     */
    UUID getModelName(ID root_id) const
    {
        MEMORIA_V1_ASSERT_NOT_EMPTY(root_id);

        auto& self = this->self();

        NodeBaseG root = self.allocator().getPage(root_id, self.master_name());

        return NodeDispatcher::dispatch(root, GetModelNameFn(self));
    }

    static UUID getModelNameS(NodeBaseG root)
    {
        return getRootMetadataS(root).model_name();
    }

    static const Metadata& getRootMetadataS(NodeBaseG node)
    {
        MEMORIA_V1_ASSERT_TRUE(node.isSet());
        MEMORIA_V1_ASSERT_TRUE(node->is_root());

        return node->root_metadata();
    }



    void setModelName(const UUID& name)
    {
        NodeBaseG root = self().getRoot();

        NodeDispatcher::dispatch(root, SetModelNameFn(self()), name);
    }

    void initCtr(int32_t command)
    {
        Base::initCtr(command);

        auto& self = this->self();

        if ((command & CTR_CREATE) && (command & CTR_FIND))
        {
            if (self.allocator().hasRoot(self.master_name()))
            {
                findCtrByName();
            }
            else {
                createCtrByName();
            }
        }
        else if (command & CTR_CREATE)
        {
            if (!self.allocator().hasRoot(self.master_name()))
            {
                createCtrByName();
            }
            else {
                MMA1_THROW(CtrAlreadyExistsException()) << WhatInfo(fmt::format8(u"Container with name {} already exists", self.master_name()));
            }
        }
        else {
            findCtrByName();
        }
    }

    void initCtr(const ID& root_id)
    {
        self().set_root_id(root_id);
    }

    void initCtr(const ID& root_id, const UUID& name)
    {
        auto& self = this->self();
        self.set_root_id(root_id);
    }


    virtual ID getRootID(const UUID& name)
    {
        auto& self = this->self();

        NodeBaseG root = self.allocator().getPage(self.root(), self.master_name());

        return root->root_metadata().roots(name);
    }


    MEMORIA_V1_FN_WRAPPER_RTN(SetRootIdFn, setRootIdFn, Metadata);

    virtual void setRoot(const UUID& name, const ID& root_id)
    {
        auto& self = this->self();

        NodeBaseG root  = self.allocator().getPageForUpdate(self.root(), self.master_name());

        Metadata& metadata = root->root_metadata();
        metadata.roots(name) = root_id;
    }


    static Metadata getCtrRootMetadata(NodeBaseG node)
    {
        MEMORIA_V1_ASSERT_TRUE(node.isSet());
        MEMORIA_V1_ASSERT_TRUE(node->has_root_metadata());

        return node->root_metadata();
    }

    /**
     * \brief Set metadata into root node.
     *
     * \param node Must be a root node
     * \param metadata metadata to set
     */

    void setCtrRootMetadata(NodeBaseG& node, const Metadata& metadata) const
    {
        MEMORIA_V1_ASSERT_TRUE(node.isSet());

        self().updatePageG(node);
        node->setMetadata(metadata);
    }

    Metadata getRootMetadata() const
    {
        auto& self          = this->self();
        const auto& root_id = self.root();
        NodeBaseG root      = self.allocator().getPage(root_id, self.master_name());

        return root->root_metadata();
    }

    void setRootMetadata(const Metadata& metadata) const
    {
        NodeBaseG root = self().getRootForUpdate();
        self().setRootMetadata(root, metadata);
    }

    /**
     * \brief Set metadata into root node.
     *
     * \param node Must be a root node
     * \param metadata to set
     */
    void setRootMetadata(NodeBaseG& node, const Metadata& metadata) const
    {
        setCtrRootMetadata(node, metadata);
    }

    int64_t getContainerName() const
    {
        return getRootMetadata().model_name();
    }

    Metadata createNewRootMetadata() const
    {
        Metadata metadata;

        memset(&metadata, 0, sizeof(Metadata));

        metadata.model_name()       = self().name();
        metadata.page_size()        = DEFAULT_BLOCK_SIZE;
        metadata.branching_factor() = 0;

        auto txn_id = self().allocator().currentTxnId();
        metadata.txn_id() = txn_id;

        return metadata;
    }

    int32_t getNewPageSize() const
    {
        return self().getRootMetadata().page_size();
    }

    void setNewPageSize(int32_t page_size) const
    {
        Metadata metadata       = self().getRootMetadata();
        metadata.page_size()    = page_size;

        self().setRootMetadata(metadata);
    }

    template <typename Node>
    NodeBaseG createNodeFn(int32_t size) const
    {
        auto& self = this->self();

        NodeBaseG node = self.allocator().createPage(size, self.master_name());
        node->init();

        node->page_type_hash() = Node::hash();

        return node;
    }

    MEMORIA_V1_CONST_STATIC_FN_WRAPPER_RTN(CreateNodeFn, createNodeFn, NodeBaseG);

    NodeBaseG createNode(int16_t level, bool root, bool leaf, int32_t size = -1) const
    {
        MEMORIA_V1_ASSERT(level, >=, 0);

        auto& self = this->self();

        Metadata meta;

        if (!self.isNew())
        {
            meta = self.getRootMetadata();
        }
        else {
            meta = self.createNewRootMetadata();
        }

        if (size == -1)
        {
            size = meta.page_size();
        }

        NodeBaseG node = DefaultDispatcher::dispatch2(
                        leaf,
                        CreateNodeFn(self),
						size
        );



        node->ctr_type_hash()           = self.hash();
        
        node->parent_id()               = ID();
        node->parent_idx()              = 0;

        node->set_root(root);
        node->set_leaf(leaf);

        node->level() = level;

        prepareNode(node);

        if (root)
        {
            self.setCtrRootMetadata(node, meta);
        }

        if (leaf) {
        	self.layoutLeafNode(node, Position());
        }
        else {
        	self.layoutBranchNode(node, -1ull);
        }

        return node;
    }

    NodeBaseG createRootNode(int16_t level, bool leaf, const Metadata& metadata) const
    {
        MEMORIA_V1_ASSERT(level, >=, 0);

        auto& self = this->self();

        NodeBaseG node = NodeDispatcher::dispatch2(
                    leaf,
                    CreateNodeFn(self), metadata.page_size()
        );


        node->ctr_type_hash()           = self.hash();
        
        node->parent_id()               = ID();
        node->parent_idx()              = 0;

        node->set_root(true);
        node->set_leaf(leaf);

        node->level() = level;

        prepareNode(node);

        self.setCtrRootMetadata(node, metadata);

        if (leaf) {
        	self.layoutLeafNode(node, Position());
        }
        else {
        	self.layoutBranchNode(node, -1ull);
        }

        return node;
    }

    template <typename Node>
    void prepareNode(Node* node) const
    {
        node->prepare();
    }

    MEMORIA_V1_CONST_FN_WRAPPER(PrepareNodeFn, prepareNode);

    void prepareNode(NodeBaseG& node) const
    {
        MEMORIA_V1_ASSERT_TRUE(node.isSet());
        NodeDispatcher::dispatch(node, PrepareNodeFn(self()));
    }

    void markCtrUpdated()
    {
        auto& self = this->self();

        int64_t txn_id = self.allocator().currentTxnId();
        const Metadata& metadata = self.getRootMetadata();

        if (txn_id == metadata.txn_id())
        {
            // do nothing
        }
        else if (txn_id > metadata.txn_id())
        {
            Metadata copy = metadata;
            copy.txn_id() = txn_id;

            self.setRootMetadata(copy);

            self.allocator().markUpdated(self.master_name());
        }
        else {
            MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Invalid txn_id {} < {}", txn_id, metadata.txn_id()));
        }
    }

    void updatePageG(NodeBaseG& node) const
    {
        node.update(self().master_name());
    }


    virtual bool hasRoot(const UUID& name)
    {
        auto& self = this->self();

        NodeBaseG root = self.allocator().getPage(self.root(), self.master_name());

        return !root->root_metadata().roots(name).is_null();
    }


    MEMORIA_V1_DECLARE_NODE_FN_RTN(ValuesAsVectorFn, template values_as_vector<ID>, std::vector<ID>);
    Collection<Edge> describe_page_links(const UUID& page_id, const UUID& name, Direction direction)
    {
        std::vector<Edge> links;

        NodeBaseG page = this->allocator_holder_->getPage(page_id, name);

        Graph graph = this->graph();
        Vertex ctr_vx = this->as_vertex();
        Vertex page_vx = this->page_as_vertex(page_id);

        if (is_in(direction))
        {
            if (!page->is_root())
            {
                links.push_back(DefaultEdge::make(graph, "child", this->page_as_vertex(page->parent_id()), page_vx));
            }
            else {
                links.push_back(DefaultEdge::make(graph, "root", ctr_vx, page_vx));
            }
        }

        if (is_out(direction))
        {
            if (!page->is_leaf())
            {
                auto child_ids = BranchDispatcher::dispatch(page, ValuesAsVectorFn());
                for (auto& child_id: child_ids)
                {
                    links.push_back(DefaultEdge::make(graph, "child", page_vx, this->page_as_vertex(child_id)));
                }
            }
        }

        return STLCollection<Edge>::make(std::move(links));
    }

    Collection<VertexProperty> page_properties(const Vertex& vx, const ID& page_id, const UUID& name)
    {
        NodeBaseG page = this->allocator_holder_->getPage(page_id, name);

        std::vector<VertexProperty> props;

        CtrPageType page_type{};

        if (page->is_root() && page->is_leaf()) {
            page_type = CtrPageType::ROOT_LEAF;
        }
        else if (page->is_root() && !page->is_leaf()) {
            page_type = CtrPageType::ROOT;
        }
        else if ((!page->is_root()) && page->is_leaf()) {
            page_type = CtrPageType::LEAF;
        }
        else {
            page_type = CtrPageType::INTERNAL;
        }

        props.emplace_back(DefaultVertexProperty::make(vx, u"type", page_type));

        std::stringstream buf;
        self().dump(page, buf);

        props.emplace_back(DefaultVertexProperty::make(vx, u"content", U16String(buf.str())));

        return STLCollection<VertexProperty>::make(std::move(props));
    }

    int32_t metadata_links_num() const {
        return self().getRootMetadata().links_num();
    }

    UUID get_metadata_link(int num) const
    {
        return self().getRootMetadata().links(num);
    }

    void set_metadata_link(int num, const UUID& link_id)
    {
        auto& self = this->self();

        Metadata metadata   = self.getRootMetadata();
        metadata.links(num) = link_id;

        self.setRootMetadata(metadata);
    }

    std::string get_descriptor_str() const {
        return self().getRootMetadata().descriptor_str();
    }

    void set_descriptor_str(const std::string& str)
    {
        auto& self = this->self();

        Metadata metadata = self.getRootMetadata();
        metadata.set_descriptor(str);

        self.setRootMetadata(metadata);
    }



 private:

    void findCtrByName()
    {
        auto& self = this->self();

        auto name = self.master_name();

        ID root_id = self.allocator().getRootID(name);

        if (!root_id.is_null())
        {
            PageG node = self.allocator().getPage(root_id, name);

            if (node->ctr_type_hash() == CONTAINER_HASH)
            {
                self.set_root_id(root_id);
            }
            else {
                MMA1_THROW(CtrTypeException()) << WhatInfo(fmt::format8(u"Invalid container type: {}", node->ctr_type_hash()));
            }
        }
        else {
            MMA1_THROW(NoCtrException()) << WhatInfo(fmt::format8(u"Container with name {} does not exists", name));
        }
    }

    void createCtrByName()
    {
        auto& self = this->self();

        NodeBaseG node = self.createRoot();

        self.set_root(node->id());
    }

MEMORIA_V1_BT_MODEL_BASE_CLASS_END


}}}
