
// Copyright Victor Smirnov 2011-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_MODEL_BASE_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_MODEL_BASE_HPP

#include <memoria/core/container/container.hpp>
#include <memoria/core/container/names.hpp>
#include <memoria/core/types/algo.hpp>
#include <memoria/core/tools/static_array.hpp>
#include <memoria/core/container/macros.hpp>


#include <memoria/prototypes/bt/bt_macros.hpp>

#include <iostream>

namespace memoria           {
namespace bt     {

MEMORIA_BT_MODEL_BASE_CLASS_BEGIN(BTreeCtrBase)

    typedef typename Base::Types                                                Types;

    typedef typename Base::Allocator                                            Allocator;
    typedef typename Allocator::Page                                            Page;
    typedef typename Allocator::PageG                                           PageG;
    typedef typename Allocator::ID                                              ID;
    typedef typename Base::CtrShared                                            CtrShared;

    typedef typename Types::BranchNodeEntry                                     BranchNodeEntry;

    typedef typename Types::NodeBase                                            NodeBase;
    typedef typename Types::NodeBaseG                                           NodeBaseG;

    using NodeDispatcher = typename Types::Pages::NodeDispatcher;
    using LeafDispatcher = typename Types::Pages::LeafDispatcher;
    using BranchDispatcher = typename Types::Pages::BranchDispatcher;
    using DefaultDispatcher = typename Types::Pages::DefaultDispatcher;

    typedef typename Types::Metadata                                            Metadata;


    class BTreeCtrShared: public CtrShared {

        Metadata metadata_;
        Metadata metadata_log_;

        bool metadata_updated;

    public:

        BTreeCtrShared(const UUID& name): CtrShared(name), metadata_updated(false)                            {}
        BTreeCtrShared(const UUID& name, CtrShared* parent): CtrShared(name, parent), metadata_updated(false) {}

        const Metadata& root_metadata() const { return metadata_updated ? metadata_log_ : metadata_ ;}

        void update_metadata(const Metadata& metadata)
        {
            metadata_log_       = metadata;
            metadata_updated    = true;
        }

        void configure_metadata(const Metadata& metadata)
        {
            metadata_           = metadata;
            metadata_updated    = false;
        }

        bool is_metadata_updated() const
        {
            return metadata_updated;
        }

        virtual void commit()
        {
            CtrShared::commit();

            if (is_metadata_updated())
            {
                metadata_           = metadata_log_;
                metadata_updated    = false;
            }
        }

        virtual void rollback()
        {
            CtrShared::rollback();

            if (is_metadata_updated())
            {
                metadata_updated    = false;
            }
        }
    };





    void operator=(ThisType&& other) {
        Base::operator=(std::move(other));
    }

    void operator=(const ThisType& other) {
        Base::operator=(other);
    }

    PageG createRoot() const {
        return me()->createNode1(0, true, true);
    }



    template <typename Node>
    UUID getModelNameFn(const Node* node) const
    {
        return node->root_metadata().model_name();
    }

    MEMORIA_CONST_FN_WRAPPER_RTN(GetModelNameFn, getModelNameFn, Int);


    template <typename Node>
    void setModelNameFn(Node* node, const UUID& name)
    {
        node->root_metadata().model_name() = name;
    }

    MEMORIA_CONST_FN_WRAPPER_RTN(SetModelNameFn, setModelNameFn, Int);



    /**
     * \brief Get model name from the root node
     * \param root_id must be a root node ID
     */
    UUID getModelName(ID root_id) const
    {
        MEMORIA_ASSERT_NOT_EMPTY(root_id);

        auto& self = this->self();

        NodeBaseG root = self.allocator().getPage(root_id, self.master_name());

        return NodeDispatcher::dispatch(root, GetModelNameFn(me()));
    }

    void setModelName(const UUID& name)
    {
        NodeBaseG root = self().getRoot();

        NodeDispatcher::dispatch(root, SetModelNameFn(me()), name);
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
                        SBuf()<<"Container with name "<<self.name()<<" already exists"
                );
            }
        }
        else {
            findCtrByName();
        }
    }

    void initCtr(const ID& root_id)
    {
        // FIXME: Why root_id is not in use here?
        CtrShared* shared = self().getOrCreateCtrShared(self().name());
        Base::setCtrShared(shared);
    }

    void initCtr(const ID& root_id, BigInt name)
    {
    	// FIXME: Why root_id is not in use here?
    	CtrShared* shared = self().getOrCreateCtrShared(name);
    	Base::setCtrShared(shared);
    }

    void configureNewCtrShared(CtrShared* shared, PageG root) const
    {
        T2T<BTreeCtrShared*>(shared)->configure_metadata(self().getCtrRootMetadata(root));
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

        BTreeCtrShared* shared = T2T<BTreeCtrShared*>(self.shared());
        shared->update_metadata(metadata);
    }

    BTreeCtrShared* createCtrShared(const UUID& name)
    {
        return new (&self().allocator()) BTreeCtrShared(name);
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

    const Metadata& getRootMetadata() const
    {
        return T2T<const BTreeCtrShared*>(self().shared())->root_metadata();
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

        BTreeCtrShared* shared = T2T<BTreeCtrShared*>(self().shared());
        shared->update_metadata(metadata);
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

        BigInt txn_id = self().allocator().currentTxnId();
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

    NodeBaseG createNode1(Short level, bool root, bool leaf, Int size = -1) const
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
                        CreateNodeFn(me()), size
                    );



        node->ctr_type_hash()           = self.hash();
        node->master_ctr_type_hash()    = self.init_data().master_ctr_type_hash();
        node->owner_ctr_type_hash()     = self.init_data().owner_ctr_type_hash();

        node->parent_id()               = ID(0);
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

    NodeBaseG createRootNode1(Short level, bool leaf, const Metadata& metadata) const
    {
        MEMORIA_ASSERT(level, >=, 0);

        auto& self = this->self();

        NodeBaseG node = NodeDispatcher::dispatch2(
                    leaf,
                    CreateNodeFn(me()), metadata.page_size()
        );


        node->ctr_type_hash()           = self.hash();
        node->master_ctr_type_hash()    = self.init_data().master_ctr_type_hash();
        node->owner_ctr_type_hash()     = self.init_data().owner_ctr_type_hash();

        node->parent_id()               = ID(0);
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

        NodeDispatcher::dispatch(node, PrepareNodeFn(me()));
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
            throw vapi::Exception(MA_SRC, SBuf()<<"Invalid txn_id "<<txn_id<<" < "<<metadata.txn_id());
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

        return root->root_metadata().roots(name).isSet();
    }

 private:

    void findCtrByName()
    {
        CtrShared* shared = self().getOrCreateCtrShared(self().name());
        Base::setCtrShared(shared);
    }

    void createCtrByName()
    {
        auto& self = this->self();

        BTreeCtrShared* shared = self.createCtrShared(self.name());
        self.allocator().registerCtrShared(shared);

        NodeBaseG node          = self.createRoot();

        self.allocator().setRoot(self.name(), node->id());

        shared->root_log()      = node->id();
        shared->updated()       = true;

        self.configureNewCtrShared(shared, node);

        Base::setCtrShared(shared);
    }

MEMORIA_BT_MODEL_BASE_CLASS_END


}}

#endif
