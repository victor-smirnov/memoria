
// Copyright Victor Smirnov 2011-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/core/container/container.hpp>
#include <memoria/v1/core/container/names.hpp>
#include <memoria/v1/core/types/algo.hpp>
#include <memoria/v1/core/tools/static_array.hpp>
#include <memoria/v1/core/container/macros.hpp>


#include <memoria/v1/prototypes/bt/bt_macros.hpp>

#include <iostream>

namespace memoria           {
namespace bt     {

MEMORIA_BT_MODEL_BASE_CLASS_BEGIN(BTreeCtrBase)

    using Types = typename Base::Types;

    using typename Base::Allocator;

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

    static const Int Streams = Types::Streams;

    PageG createRoot() const {
        return self().createNode(0, true, true);
    }



    template <typename Node>
    UUID getModelNameFn(const Node* node) const
    {
        return node->root_metadata().model_name();
    }

    MEMORIA_CONST_FN_WRAPPER_RTN(GetModelNameFn, getModelNameFn, UUID);


    template <typename Node>
    void setModelNameFn(Node* node, const UUID& name)
    {
        node->root_metadata().model_name() = name;
    }

    MEMORIA_CONST_FN_WRAPPER(SetModelNameFn, setModelNameFn);



    /**
     * \brief Get model name from the root node
     * \param root_id must be a root node ID
     */
    UUID getModelName(ID root_id) const
    {
        MEMORIA_ASSERT_NOT_EMPTY(root_id);

        auto& self = this->self();

        NodeBaseG root = self.allocator().getPage(root_id, self.master_name());

        return NodeDispatcher::dispatch(root, GetModelNameFn(self));
    }

    void setModelName(const UUID& name)
    {
        NodeBaseG root = self().getRoot();

        NodeDispatcher::dispatch(root, SetModelNameFn(self()), name);
    }

    void initCtr(Int command)
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
                throw CtrAlreadyExistsException (
                        MEMORIA_SOURCE,
                        SBuf()<<"Container with name "<<self.master_name()<<" already exists"
                );
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


    MEMORIA_FN_WRAPPER_RTN(SetRootIdFn, setRootIdFn, Metadata);

    virtual void setRoot(const UUID& name, const ID& root_id)
    {
        auto& self = this->self();

        NodeBaseG root  = self.allocator().getPageForUpdate(self.root(), self.master_name());

        Metadata& metadata = root->root_metadata();
        metadata.roots(name) = root_id;
    }


    static Metadata getCtrRootMetadata(NodeBaseG node)
    {
        MEMORIA_ASSERT_TRUE(node.isSet());
        MEMORIA_ASSERT_TRUE(node->has_root_metadata());

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
        MEMORIA_ASSERT_TRUE(node.isSet());

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

    BigInt getContainerName() const
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

    Int getNewPageSize() const
    {
        return self().getRootMetadata().page_size();
    }

    void setNewPageSize(Int page_size) const
    {
        Metadata metadata       = self().getRootMetadata();
        metadata.page_size()    = page_size;

        self().setRootMetadata(metadata);
    }

    template <typename Node>
    NodeBaseG createNodeFn(Int size) const
    {
        auto& self = this->self();

        NodeBaseG node = self.allocator().createPage(size, self.master_name());
        node->init();

        node->page_type_hash() = Node::hash();

        return node;
    }

    MEMORIA_CONST_STATIC_FN_WRAPPER_RTN(CreateNodeFn, createNodeFn, NodeBaseG);

    NodeBaseG createNode(Short level, bool root, bool leaf, Int size = -1) const
    {
        MEMORIA_ASSERT(level, >=, 0);

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
                        CreateNodeFn(self), size
                    );



        node->ctr_type_hash()           = self.hash();
        node->master_ctr_type_hash()    = self.init_data().master_ctr_type_hash();
        node->owner_ctr_type_hash()     = self.init_data().owner_ctr_type_hash();

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

        return node;
    }

    NodeBaseG createRootNode(Short level, bool leaf, const Metadata& metadata) const
    {
        MEMORIA_ASSERT(level, >=, 0);

        auto& self = this->self();

        NodeBaseG node = NodeDispatcher::dispatch2(
                    leaf,
                    CreateNodeFn(self), metadata.page_size()
        );


        node->ctr_type_hash()           = self.hash();
        node->master_ctr_type_hash()    = self.init_data().master_ctr_type_hash();
        node->owner_ctr_type_hash()     = self.init_data().owner_ctr_type_hash();

        node->parent_id()               = ID();
        node->parent_idx()              = 0;

        node->set_root(true);
        node->set_leaf(leaf);

        node->level() = level;

        prepareNode(node);

        self.setCtrRootMetadata(node, metadata);

        return node;
    }

    template <typename Node>
    void prepareNode(Node* node) const
    {
        node->prepare();
    }

    MEMORIA_CONST_FN_WRAPPER(PrepareNodeFn, prepareNode);

    void prepareNode(NodeBaseG& node) const
    {
        MEMORIA_ASSERT_TRUE(node.isSet());

        NodeDispatcher::dispatch(node, PrepareNodeFn(self()));
    }

    void markCtrUpdated()
    {
        auto& self = this->self();

        BigInt txn_id = self.allocator().currentTxnId();
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
            throw Exception(MA_SRC, SBuf()<<"Invalid txn_id "<<txn_id<<" < "<<metadata.txn_id());
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
                throw CtrTypeException(MEMORIA_SOURCE, SBuf()<<"Invalid container type: "<<node->ctr_type_hash());
            }
        }
        else {
            throw NoCtrException(MEMORIA_SOURCE, SBuf()<<"Container with name "<<name<<" does not exists");
        }
    }

    void createCtrByName()
    {
        auto& self = this->self();

        NodeBaseG node = self.createRoot();

        self.set_root(node->id());
    }

MEMORIA_BT_MODEL_BASE_CLASS_END


}}
