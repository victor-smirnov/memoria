
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

    typedef typename Types::Key                                                 Key;
    typedef typename Types::Value                                               Value;
    typedef typename Types::Element                                             Element;
    typedef typename Types::Accumulator                                         Accumulator;

    typedef typename Types::NodeBase                                            NodeBase;
    typedef typename Types::NodeBaseG                                           NodeBaseG;

    typedef typename Types::Pages::NodeDispatcher                               NodeDispatcher;
    typedef typename Types::Pages::NodeDispatcher                               RootDispatcher;
    typedef typename Types::Pages::LeafDispatcher                               LeafDispatcher;
    typedef typename Types::Pages::NonLeafDispatcher                            NonLeafDispatcher;
    typedef typename Types::Pages::NodeDispatcher                               NonRootDispatcher;
    typedef typename Types::Pages::DefaultDispatcher                            DefaultDispatcher;

    typedef typename Types::Metadata                                            Metadata;

    typedef typename Types::TreePathItem                                        TreePathItem;
    typedef typename Types::TreePath                                            TreePath;


    class BTreeCtrShared: public CtrShared {

        Metadata metadata_;
        Metadata metadata_log_;

        bool metadata_updated;

    public:

        BTreeCtrShared(BigInt name): CtrShared(name), metadata_updated(false)                            {}
        BTreeCtrShared(BigInt name, CtrShared* parent): CtrShared(name, parent), metadata_updated(false) {}

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
    Int getModelNameFn(const Node* node) const
    {
        return node->root_metadata().model_name();
    }

    MEMORIA_CONST_FN_WRAPPER_RTN(GetModelNameFn, getModelNameFn, Int);


    template <typename Node>
    void setModelNameFn(Node* node, Int name)
    {
        node->root_metadata().model_name() = name;
    }

    MEMORIA_CONST_FN_WRAPPER_RTN(SetModelNameFn, setModelNameFn, Int);



    /**
     * \brief Get model name from the root node
     * \param root_id must be a root node ID
     */
    BigInt getModelName(ID root_id) const
    {
        MEMORIA_ASSERT_NOT_EMPTY(root_id);

        NodeBaseG root      = self().allocator().getPage(root_id, Allocator::READ);

        return RootDispatcher::dispatchConstRtn(root.page(), GetModelNameFn(me()));
    }

    void setModelName(BigInt name)
    {
        MEMORIA_ASSERT_EXPR(name >= 0, "Container name must not be positive")

        NodeBaseG root  = self().getRoot(Allocator::READ);

        RootDispatcher::dispatch(root.page(), SetModelNameFn(me()), name);
    }

    void initCtr(Int command)
    {
        Base::initCtr(command);

        auto& self = this->self();

        if ((command & CTR_CREATE) && (command & CTR_FIND))
        {
            if (self.allocator().hasRoot(self.name()))
            {
                findCtrByName();
            }
            else {
                createCtrByName();
            }
        }
        else if (command & CTR_CREATE)
        {
            if (!self.allocator().hasRoot(self.name()))
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

    void configureNewCtrShared(CtrShared* shared, PageG root) const
    {
        T2T<BTreeCtrShared*>(shared)->configure_metadata(MyType::getCtrRootMetadata(root));
    }


    virtual ID getRootID(BigInt name)
    {
        MEMORIA_ASSERT(name, >=, 0);

        auto& self = this->self();

        NodeBaseG root = self.allocator().getPage(self.root(), Allocator::READ);

        return root->root_metadata().roots(name);
    }


    MEMORIA_FN_WRAPPER_RTN(SetRootIdFn, setRootIdFn, Metadata);

    virtual void  setRoot(BigInt name, const ID& root_id)
    {
        auto& self = this->self();

        NodeBaseG root  = self.allocator().getPage(self.root(), Allocator::UPDATE);

        Metadata& metadata = root->root_metadata();
        metadata.roots(name) = root_id;

        BTreeCtrShared* shared = T2T<BTreeCtrShared*>(self.shared());
        shared->update_metadata(metadata);
    }

    BTreeCtrShared* createCtrShared(BigInt name)
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

    static void setCtrRootMetadata(NodeBaseG node, const Metadata& metadata)
    {
        MEMORIA_ASSERT_TRUE(node.isSet());

        node.update();
//        RootDispatcher::dispatch(node, SetMetadataFn(), metadata);

        node->setMetadata(metadata);
    }

    const Metadata& getRootMetadata() const
    {
        return T2T<const BTreeCtrShared*>(self().shared())->root_metadata();
    }

    void setRootMetadata(const Metadata& metadata) const
    {
        NodeBaseG root = self().getRoot(Allocator::UPDATE);
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
        NodeBaseG node = self().allocator().createPage(size);
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

        NodeBaseG node = DefaultDispatcher::dispatchStatic2Rtn(
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
            MyType::setCtrRootMetadata(node, meta);
        }

        return node;
    }

    NodeBaseG createRootNode1(Short level, bool leaf, const Metadata& metadata) const
    {
        MEMORIA_ASSERT(level, >=, 0);

        auto& self = this->self();

        NodeBaseG node = NodeDispatcher::dispatchStatic2Rtn(
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

        MyType::setCtrRootMetadata(node, metadata);

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

        NodeDispatcher::dispatch(node.page(), PrepareNodeFn(me()));
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

    		self.allocator().markUpdated(self.name());
    	}
    	else {
    		throw vapi::Exception(MA_SRC, SBuf()<<"Invalid txn_id "<<txn_id<<" < "<<metadata.txn_id());
    	}
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
