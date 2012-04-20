
// Copyright Victor Smirnov 2011-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BTREE_MODEL_BASE_HPP
#define	_MEMORIA_PROTOTYPES_BTREE_MODEL_BASE_HPP

#include <memoria/core/container/container.hpp>
#include <memoria/core/container/names.hpp>
#include <memoria/prototypes/btree/macros.hpp>
#include <memoria/core/types/algo.hpp>
#include <memoria/core/tools/fixed_vector.hpp>


#include <iostream>

namespace memoria    	{
namespace btree			{

MEMORIA_BTREE_MODEL_BASE_CLASS_BEGIN(BTreeContainerBase)


    typedef typename Base::Types                                                Types;

	typedef typename Base::Allocator                                           	Allocator;
	typedef typename Allocator::PageG                                           PageG;
	typedef typename Allocator::ID                                           	ID;
	typedef typename Base::CtrShared                                       		CtrShared;

    typedef typename Types::Key                                               	Key;
    typedef typename Types::Value                                               Value;
    typedef typename Types::Element                                             Element;
    typedef typename Types::Accumulator											Accumulator;

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

    typedef typename Types::Metadata 											Metadata;

    typedef typename Types::TreePathItem										TreePathItem;
    typedef typename Types::TreePath											TreePath;


    class BTreeCtrShared: public CtrShared {

    	Metadata metadata_;
    	Metadata metadata_log_;

    	bool metadata_updated;

    public:

    	BTreeCtrShared(BigInt name): CtrShared(name), metadata_updated(false) 							 {}
    	BTreeCtrShared(BigInt name, CtrShared* parent): CtrShared(name, parent), metadata_updated(false) {}

    	const Metadata& metadata() const { return metadata_updated ? metadata_log_ : metadata_ ;}

    	void update_metadata(const Metadata& metadata)
    	{
    		metadata_log_ 		= metadata;
    		metadata_updated 	= true;
    	}

    	void configure_metadata(const Metadata& metadata)
    	{
    		metadata_ 			= metadata;
    		metadata_updated 	= false;
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
    			metadata_ 			= metadata_log_;
    			metadata_updated	= false;
    		}
    	}

    	virtual void rollback()
    	{
    		CtrShared::rollback();

    		if (is_metadata_updated())
    		{
    			metadata_updated	= false;
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

    PageG CreateRoot() const {
    	return me()->CreateNode(0, true, true);
    }

    struct GetModelNameFn
    {
    	BigInt name_;

    	template <typename Node>
    	void operator()(const Node* node)
    	{
    		name_ = node->metadata().model_name();
    	}
    };

    struct SetModelNameFn
    {
    	BigInt name_;

    	SetModelNameFn(BigInt name): name_(name) {}

    	template <typename Node>
    	void operator()(Node* node)
    	{
    		node->metadata().model_name() = name_;
    	}
    };

    BigInt GetModelName(ID root_id) const
    {
    	NodeBaseG root 		= me()->allocator().GetPage(root_id, Allocator::READ);

    	GetModelNameFn fn;
    	RootDispatcher::DispatchConst(root.page(), fn);

    	return fn.name_;
    }

    void SetModelName(BigInt name)
    {
    	NodeBaseG root 	= me()->GetRoot(Allocator::READ);

    	SetModelNameFn fn(name);
    	RootDispatcher::Dispatch(root.page(), fn);
    }

    void InitCtr(bool create)
    {
    	Base::InitCtr(create);

    	if (create)
    	{
    		BTreeCtrShared* shared = me()->CreateCtrShared(me()->name());
    		me()->allocator().RegisterCtrShared(shared);

    		NodeBaseG node 		    = me()->CreateRoot();

    		me()->allocator().SetRoot(me()->name(), node->id());

    		shared->root_log() 		= node->id();
    		shared->updated() 		= true;

    		me()->ConfigureNewCtrShared(shared, node);

    		Base::SetCtrShared(shared);
    	}
    	else {
    		CtrShared* shared = me()->GetOrCreateCtrShared(me()->name());

    		Base::SetCtrShared(shared);
    	}
    }

    void InitCtr(const ID& root_id)
    {
    	CtrShared* shared = me()->GetOrCreateCtrShared(me()->name());
    	Base::SetCtrShared(shared);
    }

    void ConfigureNewCtrShared(CtrShared* shared, PageG root) const
    {
    	T2T<BTreeCtrShared*>(shared)->configure_metadata(MyType::GetCtrRootMetadata(root));
    }

    struct GetRootIDFn {
    	BigInt 	name_;
    	ID		root_;

    	GetRootIDFn(BigInt name): name_(name) {}

    	template <typename Node>
    	void operator()(const Node* node)
    	{
    		root_ = node->metadata().roots(name_);
    	}
    };

    virtual ID GetRootID(BigInt name)
    {
    	NodeBaseG root = me()->allocator().GetPage(me()->root(), Allocator::READ);

    	GetRootIDFn fn(name);
    	RootDispatcher::DispatchConst(root.page(), fn);

    	return fn.root_;
    }

    struct SetRootIDFn {
    	BigInt 		name_;
    	ID			root_;

    	Metadata 	metadata_;

    	SetRootIDFn(BigInt name, const ID& root): name_(name), root_(root) {}

    	template <typename Node>
    	void operator()(Node* node)
    	{
    		node->metadata().roots(name_) = root_;

    		metadata_ = node->metadata();
    	}
    };

    virtual void  SetRoot(BigInt name, const ID& root_id)
    {
    	NodeBaseG root 	= me()->allocator().GetPage(me()->root(), Allocator::UPDATE);

    	SetRootIDFn fn(name, root_id);
    	RootDispatcher::Dispatch(root.page(), fn);

    	BTreeCtrShared* shared = T2T<BTreeCtrShared*>(me()->shared());
    	shared->update_metadata(fn.metadata_);
    }

    BTreeCtrShared* CreateCtrShared(BigInt name)
    {
    	return new (&me()->allocator()) BTreeCtrShared(name);
    }


    struct GetMetadataFn {
        Metadata metadata_;

        GetMetadataFn() {}

        template <typename T>
        void operator()(T *node) {
            metadata_ = node->metadata();
        }
    };


    struct SetMetadataFn {
    	const Metadata& metadata_;

    	SetMetadataFn(const Metadata& metadata): metadata_(metadata) {}

    	template <typename T>
    	void operator()(T *node)
    	{
    		node->metadata() = metadata_;
    	}
    };

    static Metadata GetCtrRootMetadata(NodeBaseG node)
    {
    	GetMetadataFn fn;
    	RootDispatcher::DispatchConst(node, fn);
    	return fn.metadata_;
    }

    static void SetCtrRootMetadata(NodeBaseG node, const Metadata& metadata)
    {
    	node.update();
    	SetMetadataFn fn(metadata);
    	RootDispatcher::Dispatch(node, fn);
    }

    const Metadata& GetRootMetadata() const
    {
    	return T2T<const BTreeCtrShared*>(me()->shared())->metadata();
    }

    void SetRootMetadata(const Metadata& metadata) const
    {
    	NodeBaseG root = me()->GetRoot(Allocator::UPDATE);
    	me()->SetRootMetadata(root, metadata);
    }

    void SetRootMetadata(NodeBaseG& node, const Metadata& metadata) const
    {
    	SetCtrRootMetadata(node, metadata);

    	BTreeCtrShared* shared = T2T<BTreeCtrShared*>(me()->shared());
    	shared->update_metadata(metadata);
    }

    BigInt GetContainerName() const
    {
        return GetRootMetadata().model_name();
    }


    NodeBaseG CreateNode(Short level, bool root, bool leaf) const
    {
    	NodeBaseG node = NodeFactory::Create(me()->allocator(), level, root, leaf);

    	if (root)
    	{
    		Metadata meta = MyType::GetCtrRootMetadata(node);

    		me()->ConfigureRootMetadata(meta);

    		MyType::SetCtrRootMetadata(node, meta);
    	}

    	node->model_hash() = me()->hash();

    	InitNodeSize(node, Allocator::PAGE_SIZE);

    	return node;
    }

    NodeBaseG CreateRootNode(Short level, bool leaf, const Metadata& metadata) const
    {
    	NodeBaseG node = NodeFactory::Create(me()->allocator(), level, true, leaf);

    	MyType::SetCtrRootMetadata(node, metadata);

    	node->model_hash() = me()->hash();

    	InitNodeSize(node, Allocator::PAGE_SIZE);

    	return node;
    }


    void ConfigureRootMetadata(Metadata& metadata) const
    {
    	metadata.model_name() = me()->name();
    }

    template <typename Node>
    void InitNode(Node* node, Int block_size) const
    {
    	node->map().InitByBlock(block_size - sizeof(Node));
    }

 private:

    struct InitNodeFn {
    	const MyType& me_;
    	Int block_size_;
    	InitNodeFn(const MyType& me, Int block_size): me_(me), block_size_(block_size) {}

    	template <typename NodeT>
    	void operator()(NodeT* node) const
    	{
    		me_.InitNode(node, block_size_);
    	}
    };

    void InitNodeSize(NodeBaseG& node, Int block_size) const
    {
    	InitNodeFn fn(*me(), block_size);
    	NodeDispatcher::Dispatch(node.page(), fn);
    }

MEMORIA_BTREE_MODEL_BASE_CLASS_END


}}

#endif
