
// Copyright Victor Smirnov 2011-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_MODEL_BASE_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_MODEL_BASE_HPP

#include <memoria/core/container/container.hpp>
#include <memoria/core/container/names.hpp>
#include <memoria/core/types/algo.hpp>
#include <memoria/core/tools/fixed_vector.hpp>
#include <memoria/core/container/macros.hpp>


#include <memoria/prototypes/balanced_tree/baltree_macros.hpp>

#include <iostream>

namespace memoria       	{
namespace balanced_tree     {

MEMORIA_BALTREE_MODEL_BASE_CLASS_BEGIN(BTreeContainerBase)


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
    typedef typename Types::NodeBase::BasePageType                              TreeNodePage;

    typedef typename Types::Pages::NodeDispatcher                               NodeDispatcher;
    typedef typename Types::Pages::RootDispatcher                               RootDispatcher;
    typedef typename Types::Pages::LeafDispatcher                               LeafDispatcher;
    typedef typename Types::Pages::NonLeafDispatcher                            NonLeafDispatcher;
    typedef typename Types::Pages::NonRootDispatcher                            NonRootDispatcher;

    typedef typename Types::Pages::Node2RootMap                                 Node2RootMap;
    typedef typename Types::Pages::Root2NodeMap                                 Root2NodeMap;

    typedef typename Types::Pages::NodeFactory                                  NodeFactory;

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


    static const Int  Indexes                                                   = Types::Indexes;


    void operator=(ThisType&& other) {
        Base::operator=(std::move(other));
    }

    void operator=(const ThisType& other) {
        Base::operator=(other);
    }

    PageG createRoot() const {
        return me()->createNode(0, true, true);
    }

    struct GetModelNameFn
    {
        BigInt name_;

        template <typename Node>
        void operator()(const Node* node)
        {
            name_ = node->root_metadata().model_name();
        }
    };

    struct SetModelNameFn
    {
        BigInt name_;

        SetModelNameFn(BigInt name): name_(name) {}

        template <typename Node>
        void operator()(Node* node)
        {
            node->root_metadata().model_name() = name_;
        }
    };

    /**
     * \brief Get model name from the root node
     * \param root_id must be a root node ID
     */
    BigInt getModelName(ID root_id) const
    {
        MEMORIA_ASSERT_NOT_EMPTY(root_id);

        NodeBaseG root      = me()->allocator().getPage(root_id, Allocator::READ);

        GetModelNameFn fn;
        RootDispatcher::DispatchConst(root.page(), fn);

        return fn.name_;
    }

    void setModelName(BigInt name)
    {
        MEMORIA_ASSERT_EXPR(name >= 0, "Container name must not be positive")

        NodeBaseG root  = me()->getRoot(Allocator::READ);

        SetModelNameFn fn(name);
        RootDispatcher::Dispatch(root.page(), fn);
    }

    void initCtr(Int command)
    {
        Base::initCtr(command);

        if ((command & CTR_CREATE) && (command & CTR_FIND))
        {
        	if (me()->allocator().hasRoot(me()->name()))
        	{
        		findCtrByName();
        	}
        	else {
        		createCtrByName();
        	}
        }
        else if (command & CTR_CREATE)
        {
        	if (!me()->allocator().hasRoot(me()->name()))
        	{
        		createCtrByName();
        	}
        	else {
        		throw CtrAlreadyExistsException (
        				MEMORIA_SOURCE,
        				SBuf()<<"Container with name "<<me()->name()<<" already exists"
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
        CtrShared* shared = me()->getOrCreateCtrShared(me()->name());
        Base::setCtrShared(shared);
    }

    void configureNewCtrShared(CtrShared* shared, PageG root) const
    {
        T2T<BTreeCtrShared*>(shared)->configure_metadata(MyType::getCtrRootMetadata(root));
    }

    struct GetRootIDFn {
        BigInt  name_;
        ID      root_;

        GetRootIDFn(BigInt name): name_(name) {}

        template <typename Node>
        void operator()(const Node* node)
        {
            root_ = node->root_metadata().roots(name_);
        }
    };

    virtual ID getRootID(BigInt name)
    {
        MEMORIA_ASSERT(name, >=, 0);

        NodeBaseG root = me()->allocator().getPage(me()->root(), Allocator::READ);

        GetRootIDFn fn(name);
        RootDispatcher::DispatchConst(root.page(), fn);

        return fn.root_;
    }

    struct SetRootIDFn {
        BigInt      name_;
        ID          root_;

        Metadata    metadata_;

        SetRootIDFn(BigInt name, const ID& root): name_(name), root_(root) {}

        template <typename Node>
        void operator()(Node* node)
        {
            node->root_metadata().roots(name_) = root_;

            metadata_ = node->root_metadata();
        }
    };

    virtual void  setRoot(BigInt name, const ID& root_id)
    {
        NodeBaseG root  = me()->allocator().getPage(me()->root(), Allocator::UPDATE);

        SetRootIDFn fn(name, root_id);
        RootDispatcher::Dispatch(root.page(), fn);

        BTreeCtrShared* shared = T2T<BTreeCtrShared*>(me()->shared());
        shared->update_metadata(fn.metadata_);
    }

    BTreeCtrShared* createCtrShared(BigInt name)
    {
        return new (&me()->allocator()) BTreeCtrShared(name);
    }


    struct GetMetadataFn {
        Metadata metadata_;

        GetMetadataFn() {}

        template <typename T>
        void operator()(T *node) {
            metadata_ = node->root_metadata();
        }
    };


    struct SetMetadataFn {
        const Metadata& metadata_;

        SetMetadataFn(const Metadata& metadata): metadata_(metadata) {}

        template <typename T>
        void operator()(T *node)
        {
            node->root_metadata() = metadata_;
        }
    };

    static Metadata getCtrRootMetadata(NodeBaseG node)
    {
        MEMORIA_ASSERT_NOT_NULL(node);

        GetMetadataFn fn;
        RootDispatcher::DispatchConst(node, fn);
        return fn.metadata_;
    }

    /**
     * \brief Set metadata into root node.
     *
     * \param node Must be a root node
     * \param metadata metadata to set
     */

    static void setCtrRootMetadata(NodeBaseG node, const Metadata& metadata)
    {
        MEMORIA_ASSERT_NOT_NULL(node);

        node.update();
        SetMetadataFn fn(metadata);
        RootDispatcher::Dispatch(node, fn);
    }

    const Metadata& getRootMetadata() const
    {
        return T2T<const BTreeCtrShared*>(me()->shared())->root_metadata();
    }

    void setRootMetadata(const Metadata& metadata) const
    {
        NodeBaseG root = me()->getRoot(Allocator::UPDATE);
        me()->setRootMetadata(root, metadata);
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

        BTreeCtrShared* shared = T2T<BTreeCtrShared*>(me()->shared());
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

        metadata.model_name()       = me()->name();
        metadata.page_size()        = DEFAULT_BLOCK_SIZE;
        metadata.branching_factor() = 0;

        return metadata;
    }

    Int getNewPageSize() const
    {
        return me()->getRootMetadata().page_size();
    }

    void setNewPageSize(Int page_size) const
    {
        Metadata metadata       = me()->getRootMetadata();
        metadata.page_size()    = page_size;

        me()->setRootMetadata(metadata);
    }

    NodeBaseG createNode(Short level, bool root, bool leaf, Int size = -1) const
    {
        MEMORIA_ASSERT(level, >=, 0);

        Metadata meta;

        if (!me()->isNew())
        {
            meta = me()->getRootMetadata();
        }
        else {
            meta = me()->createNewRootMetadata();
        }

        if (size == -1) {
            size = meta.page_size();
        }

        NodeBaseG node = NodeFactory::create(me()->allocator(), level, root, leaf, size);

        if (root)
        {
            MyType::setCtrRootMetadata(node, meta);
        }

        node->ctr_type_hash() = me()->hash();

        initNodeSize(node, size);

        return node;
    }

    NodeBaseG createRootNode(Short level, bool leaf, const Metadata& metadata) const
    {
        MEMORIA_ASSERT(level, >=, 0);

        NodeBaseG node = NodeFactory::create(me()->allocator(), level, true, leaf, metadata.page_size());

        MyType::setCtrRootMetadata(node, metadata);

        node->ctr_type_hash() = me()->hash();

        initNodeSize(node, metadata.page_size());

        return node;
    }

    template <typename Node>
    void initNode(Node* node, Int block_size) const
    {
        MEMORIA_ASSERT(block_size, >=, 512);

        node->map().init(block_size - sizeof(Node) + sizeof (typename Node::Map));
    }

 private:

    struct InitNodeFn {
        const MyType& me_;
        Int block_size_;
        InitNodeFn(const MyType& me, Int block_size): me_(me), block_size_(block_size) {}

        template <typename NodeT>
        void operator()(NodeT* node) const
        {
            me_.initNode(node, block_size_);
        }
    };

    void initNodeSize(NodeBaseG& node, Int block_size) const
    {
        MEMORIA_ASSERT_NOT_NULL(node);

        InitNodeFn fn(*me(), block_size);
        NodeDispatcher::Dispatch(node.page(), fn);
    }

    void findCtrByName()
    {
    	CtrShared* shared = me()->getOrCreateCtrShared(me()->name());
    	Base::setCtrShared(shared);
    }

    void createCtrByName()
    {
    	BTreeCtrShared* shared = me()->createCtrShared(me()->name());
    	me()->allocator().registerCtrShared(shared);

    	NodeBaseG node          = me()->createRoot();

    	me()->allocator().setRoot(me()->name(), node->id());

    	shared->root_log()      = node->id();
    	shared->updated()       = true;

    	me()->configureNewCtrShared(shared, node);

    	Base::setCtrShared(shared);
    }

MEMORIA_BALTREE_MODEL_BASE_CLASS_END


}}

#endif
