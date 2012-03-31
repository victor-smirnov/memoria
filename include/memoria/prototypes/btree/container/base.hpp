
// Copyright Victor Smirnov 2011.
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

    static const Int  Indexes                                                   = Types::Indexes;


    void operator=(ThisType&& other) {
    	Base::operator=(std::move(other));
    }

    void operator=(const ThisType& other) {
    	Base::operator=(other);
    }

    PageG CreateRoot() {
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
    		CtrShared* shared = me()->CreateCtrShared(me()->name());
    		me()->allocator().RegisterCtrShared(shared);

    		PageG node 		  = me()->CreateRoot();

    		me()->allocator().SetRoot(me()->name(), node->id());

    		shared->root_log() 	= node->id();
    		shared->updated() 	= true;

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
    	BigInt 	name_;
    	ID		root_;

    	SetRootIDFn(BigInt name, const ID& root): name_(name), root_(root) {}

    	template <typename Node>
    	void operator()(Node* node)
    	{
    		node->metadata().roots(name_) = root_;
    	}
    };

    virtual void  SetRoot(BigInt name, const ID& root_id)
    {
    	NodeBaseG root 	= me()->allocator().GetPage(me()->root(), Allocator::UPDATE);

    	SetRootIDFn fn(name, root_id);
    	RootDispatcher::Dispatch(root.page(), fn);
    }

MEMORIA_BTREE_MODEL_BASE_CLASS_END


}}

#endif
