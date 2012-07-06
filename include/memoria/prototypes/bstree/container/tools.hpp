
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_MODELS_IDX_MAP_MODEL_TOOLS_HPP
#define	_MEMORIA_MODELS_IDX_MAP_MODEL_TOOLS_HPP


#include <memoria/prototypes/bstree/names.hpp>
#include <memoria/core/container/container.hpp>



namespace memoria    {

MEMORIA_CONTAINER_PART_BEGIN(memoria::bstree::ToolsName)

    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Allocator::Page                                            Page;
    typedef typename Page::ID                                                   ID;
    typedef typename Allocator::Transaction                                     Transaction;

    typedef typename Types::NodeBase                                            NodeBase;
    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;

    typedef typename Types::Pages::NodeDispatcher                               NodeDispatcher;
    typedef typename Types::Pages::RootDispatcher                               RootDispatcher;
    typedef typename Types::Pages::LeafDispatcher                               LeafDispatcher;
    typedef typename Types::Pages::NonLeafDispatcher                            NonLeafDispatcher;

    typedef typename Types::Pages::Node2RootMap                                 Node2RootMap;
    typedef typename Types::Pages::Root2NodeMap                                 Root2NodeMap;

    typedef typename Types::Pages::NodeFactory                                  NodeFactory;

    typedef typename Base::Key                                                  Key;
    typedef typename Base::Value                                                Value;
    typedef typename Base::Element                                              Element;
    typedef typename Base::Accumulator											Accumulator;

    typedef typename Base::TreePath                                             TreePath;
    typedef typename Base::TreePathItem                                         TreePathItem;


    static const Int Indexes                                                    = Types::Indexes;

    
    template <typename Node>
    bool checkNodeContent(Node *node);


    template <typename Node1, typename Node2>
    bool checkNodeWithParentContent(Node1 *node, Node2 *parent, Int parent_idx);


    //FIXME: SumKeysFn is used in MoveElementsFn
    struct SumKeysFn {

    	Int         	from_;
    	Int         	count_;
    	Accumulator&    keys_;

    public:
    	SumKeysFn(Int from, Int count, Accumulator& keys):
    		from_(from), count_(count), keys_(keys) {}

    	template <typename T>
    	void operator()(T *node)
    	{
    		node->map().Sum(from_, from_ + count_, keys_);
    	}
    };

private:


    struct SumKeysInOneBlockFn {

    	Int         	from_;
    	Int         	count_;
    	Key&    		sum_;
    	Int 			block_num_;

    public:
    	SumKeysInOneBlockFn(Int from, Int count, Key& sum, Int block_num):
    		from_(from), count_(count), sum_(sum), block_num_(block_num) {}

    	template <typename T>
    	void operator()(T *node)
    	{
    		node->map().Sum(block_num_, from_, from_ + count_, sum_);
    	}
    };

    struct AddKeysFn {
    	Int idx_;
    	const Key *keys_;
    	bool reindex_fully_;

    	AddKeysFn(Int idx, const Key* keys, bool reindex_fully): idx_(idx), keys_(keys), reindex_fully_(reindex_fully) {}

    	template <typename Node>
    	void operator()(Node *node)
    	{
    		for (Int c = 0; c < Indexes; c++)
    		{
    			node->map().updateUp(c, idx_, keys_[c]);

    			if (reindex_fully_) {
    				node->map().reindex(c);
    			}
    		}
    	}
    };

public:

    void SumKeys(const NodeBase *node, Int from, Int count, Accumulator& keys) const
    {
    	SumKeysFn fn(from, count, keys);
    	NodeDispatcher::DispatchConst(node, fn);
    }

    void SumKeys(const NodeBase *node, Int block_num, Int from, Int count, Key& sum) const
    {
    	SumKeysInOneBlockFn fn(from, count, sum, block_num);
    	NodeDispatcher::DispatchConst(node, fn);
    }

    void AddKeys(NodeBaseG& node, int idx, const Accumulator& keys, bool reindex_fully = false) const
    {
    	node.update();

    	AddKeysFn fn(idx, keys.keys(), reindex_fully);
    	NodeDispatcher::Dispatch(node, fn);
    }

    bool updateCounters(NodeBaseG& node, Int idx, const Accumulator& counters, bool reindex_fully = false) const;

MEMORIA_CONTAINER_PART_END



#define M_TYPE 		MEMORIA_CONTAINER_TYPE(memoria::bstree::ToolsName)
#define M_PARAMS 	MEMORIA_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
bool M_TYPE::updateCounters(NodeBaseG& node, Int idx, const Accumulator& counters, bool reindex_fully) const
{
	node.update();
	me()->AddKeys(node, idx, counters.keys(), reindex_fully);

	return false; //proceed further unconditionally
}



M_PARAMS
template <typename Node>
bool M_TYPE::checkNodeContent(Node *node) {
	bool errors = false;

	for (Int i = 0; i < Indexes; i++) {
		Key key = 0;

		for (Int c = 0; c < node->children_count(); c++) {
			key += node->map().key(i, c);
		}

		if (key != node->map().maxKey(i))
		{
			//me()->Dump(node);
			MEMORIA_ERROR(me(), "Sum of keys doen't match maxKey for key", i, key, node->map().maxKey(i));
			errors = true;
		}
	}

	return errors;
}

M_PARAMS
template <typename Node1, typename Node2>
bool M_TYPE::checkNodeWithParentContent(Node1 *node, Node2 *parent, Int parent_idx)
{
	bool errors = false;
	for (Int c = 0; c < Indexes; c++)
	{
		if (node->map().maxKey(c) != parent->map().key(c, parent_idx))
		{
			MEMORIA_ERROR(me(), "Invalid parent-child nodes chain", c, node->map().maxKey(c), parent->map().key(c, parent_idx), "for", node->id(), parent->id(), parent_idx);
			errors = true;
		}
	}

	errors = checkNodeContent(node) || errors;

	return errors;
}


#undef M_TYPE
#undef M_PARAMS

}

#endif
