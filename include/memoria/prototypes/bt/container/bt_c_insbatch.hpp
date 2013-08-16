
// Copyright Victor Smirnov 2011-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_MODEL_INSERT_BATCH_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_MODEL_INSERT_BATCH_HPP

#include <memoria/prototypes/bt/bt_tools.hpp>
#include <memoria/core/container/macros.hpp>

#include <vector>

namespace memoria {

using namespace memoria::bt;
using namespace memoria::core;

using namespace std;

MEMORIA_CONTAINER_PART_BEGIN(memoria::bt::InsertBatchName)

    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Base::ID                                                   ID;
    
    typedef typename Types::NodeBase                                            NodeBase;
    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;

    typedef typename Base::NodeDispatcher                                       NodeDispatcher;
    typedef typename Base::RootDispatcher                                       RootDispatcher;
    typedef typename Base::LeafDispatcher                                       LeafDispatcher;
    typedef typename Base::NonLeafDispatcher                                    NonLeafDispatcher;


    typedef typename Base::Key                                                  Key;
    typedef typename Base::Value                                                Value;
    typedef typename Base::Element                                              Element;

    typedef typename Base::Metadata                                             Metadata;

    typedef typename Types::Accumulator                                         Accumulator;
    typedef typename Types::Position 											Position;

    typedef typename Base::TreePath                                             TreePath;
    typedef typename Base::TreePathItem                                         TreePathItem;

    typedef typename Types::PageUpdateMgr										PageUpdateMgr;

    static const Int Indexes                                                    = Types::Indexes;
    static const Int Streams                                                    = Types::Streams;

    struct NonLeafNodeKeyValuePair
    {
        Accumulator keys;
        ID          value;
        BigInt      key_count;
    };

    struct ISubtreeProvider
    {
        typedef MyType                      CtrType;

        virtual NonLeafNodeKeyValuePair getKVPair(
        									const ID& parent_id,
        									Int parent_idx,
        									BigInt count,
        									Int level
        								)    													= 0;

        virtual BigInt				    getTotalKeyCount()                          			= 0;
        virtual Position				getTotalSize()       									= 0;
        virtual Position				getTotalInserted()       								= 0;

        virtual Position				remainder()       										= 0;

        virtual ISubtreeProvider&       getProvider() {return *this;}

        virtual Accumulator insertIntoLeaf(NodeBaseG& leaf)  											= 0;
        virtual Accumulator insertIntoLeaf(NodeBaseG& leaf, const Position& from)  						= 0;
        virtual Accumulator insertIntoLeaf(NodeBaseG& leaf, const Position& from, const Position& size)	= 0;

        virtual UBigInt					getActiveStreams()										= 0;
    };

    class AbstractSubtreeProviderBase: public ISubtreeProvider {

    	MyType& ctr_;
    	UBigInt active_streams_;

    public:
    	AbstractSubtreeProviderBase(MyType& ctr, UBigInt active_streams = -1ull):
    		ctr_(ctr),
    		active_streams_(active_streams)
    	{}

    	virtual UBigInt	getActiveStreams() {
    		return active_streams_;
    	}

    	virtual NonLeafNodeKeyValuePair getKVPair(const ID& parent_id, Int parent_idx, BigInt total, Int level)
    	{
    		BigInt local_count = 0;
    		return BuildTree(parent_id, parent_idx, local_count, total, level - 1);
    	}

    	MyType& ctr() {
    		return ctr_;
    	}

    	const MyType& ctr() const {
    		return ctr_;
    	}


    private:
    	NonLeafNodeKeyValuePair BuildTree(
    			const ID& parent_id,
    			Int parent_idx,
    			BigInt& count,
    			const BigInt total,
    			Int level
    	)
    	{
    		const Int MAX_CHILDREN = 200;

    		NonLeafNodeKeyValuePair pair;
    		pair.key_count = 0;

    		if (level > 0)
    		{
    			Int max_keys = ctr_.getMaxKeyCountForNode(level == 0, level);

    			NodeBaseG node 		= ctr_.createNode1(level, false, false);
    			node->parent_id() 	= parent_id;
    			node->parent_idx() 	= parent_idx;

    			ctr_.layoutNonLeafNode(node, active_streams_);

    			// FIXME: buffer size can be too small
    			NonLeafNodeKeyValuePair children[MAX_CHILDREN];

    			Int start = 0;
    			for (Int c = 0; c < max_keys && count < total; start += MAX_CHILDREN)
    			{
    				Int local = 0;
    				for (
    						Int batch_idx = 0;
    						batch_idx < MAX_CHILDREN && c < max_keys && count < total;
    						c++, batch_idx++, local++
    				)
    				{
    					children[batch_idx]     =  BuildTree(
    													node->id(),
    													c,
    													count,
    													total,
    													level - 1
    												);

    					pair.key_count  		+= children[batch_idx].key_count;
    				}

    				setINodeData(children, node, start, local);

    				if (local < MAX_CHILDREN)
    				{
    					break;
    				}
    			}

    			ctr_.reindex(node);

    			pair.keys  = ctr_.getMaxKeys(node);
    			pair.value = node->id();
    		}
    		else
    		{
    			NodeBaseG node = ctr_.createNode1(level, false, true);

    			node->parent_id() 	= parent_id;
    			node->parent_idx() 	= parent_idx;

    			count++;
    			pair.keys = this->insertIntoLeaf(node);

    			pair.value      =  node->id();
    			pair.key_count  += 1;
    		}

    		return pair;
    	}

    	template <typename PairType>
    	struct SetNodeValuesFn
    	{
    		const PairType* pairs_;
    		Int 			start_;
    		Int             count_;

    		SetNodeValuesFn(const PairType* pairs, Int start, Int count):
    			pairs_(pairs),
    			start_(start),
    			count_(count) {}

    		template <Int Idx, typename Tree>
    		void stream(Tree* tree)
    		{
    			Int c = 0;
    			tree->insert(start_, count_, [&](){
    				return std::get<Idx>(pairs_[c++].keys);
    			});
    		}

    		template <typename Node>
    		void treeNode(Node* node)
    		{
    			Int size = node->size();

    			node->processNotEmpty(*this);

    			Int c = 0;
    			node->insertValues(size, start_, count_, [&](){
    				return pairs_[c++].value;
    			});
    		}
    	};

    	template <typename PairType>
    	void setINodeData(const PairType* data, NodeBaseG& node, Int start, Int count)
    	{
    		SetNodeValuesFn<PairType> fn(data, start, count);
    		NonLeafDispatcher::dispatch(node, fn);
    	}
    };




    class DefaultSubtreeProvider: public AbstractSubtreeProviderBase {

    	typedef AbstractSubtreeProviderBase 				ProviderBase;

    	MyType& 	ctr_;
    	Position 	total_;
    	Position 	inserted_;

    	ISource& 	data_source_;

    public:
    	DefaultSubtreeProvider(MyType& ctr, const Position& total, ISource& data_source):
    		ProviderBase(ctr, total.gtZero()),
    		ctr_(ctr),
    		total_(total),
    		data_source_(data_source)
    	{}

    	virtual BigInt getTotalKeyCount()
    	{
    		StaticLayoutManager<LeafDispatcher> manager(ctr_.getRootMetadata().page_size());
    		return data_source_.getTotalNodes(&manager);
    	}

    	virtual Position getTotalSize()
    	{
    		return total_;
    	}

    	virtual Position getTotalInserted()
    	{
    		return inserted_;
    	}

    	virtual Position remainder()
    	{
    		return total_ - inserted_;
    	}

    	struct InsertIntoLeafFn {

    		typedef Accumulator ReturnType;

    		template <typename Node>
    		Accumulator treeNode(
    				Node* node,
    				DefaultSubtreeProvider* provider,
    				const Position* pos,
    				const Position* remainder
    		)
    		{
    			LayoutManager<Node> manager(node);

    			Position capacity;

    			provider->data_source_.newNode(&manager, capacity.values());

    			Position size;
    			if (remainder->lteAll(capacity))
    			{
    				size = *remainder;
    			}
    			else {
    				size = capacity;
    			}

    			node->insert(provider->data_source_, *pos, size);

    			node->reindex();

    			provider->inserted_ += size;

    			return node->sum(*pos, (*pos) + size, provider->getActiveStreams());
    		}
    	};

    	virtual Accumulator insertIntoLeaf(NodeBaseG& leaf)
    	{
    		Position pos;
    		Position remainder = this->remainder();
    		return LeafDispatcher::dispatchRtn(leaf, InsertIntoLeafFn(), this, &pos, &remainder);
    	}

    	virtual Accumulator insertIntoLeaf(NodeBaseG& leaf, const Position& from)
    	{
    		Position remainder = this->remainder();
    		return LeafDispatcher::dispatchRtn(leaf, InsertIntoLeafFn(), this, &from, &remainder);
    	}

    	virtual Accumulator insertIntoLeaf(NodeBaseG& leaf, const Position& from, const Position& size)
    	{
    		return LeafDispatcher::dispatchRtn(leaf, InsertIntoLeafFn(), this, &from, &size);
    	}
    };


    struct InsertSharedData {

    	ISubtreeProvider& provider;

    	Accumulator accumulator;

    	BigInt start;
    	BigInt end;
    	BigInt total;

    	BigInt remains;

    	BigInt first_cell_key_count;

    	UBigInt active_streams;

    	InsertSharedData(ISubtreeProvider& provider_):
    		provider(provider_),
    		start(0),
    		end(0),
    		total(provider_.getTotalKeyCount()),
    		remains(total),
    		first_cell_key_count(0),
    		active_streams(provider.getActiveStreams())
    	{}
    };



    BigInt divide(BigInt op1, BigInt op2)
    {
    	return (op1 / op2) + ((op1 % op2 == 0) ?  0 : 1);
    }

    BigInt getSubtreeSize(Int level) const;

    void makeRoom(NodeBaseG& node, const Position& start, const Position& count);
    void makeRoom(NodeBaseG& node, Int stream, Int start, Int count);

    Accumulator insertSubtree(NodeBaseG& node, Position& idx, ISubtreeProvider& provider);


    void newRootP(NodeBaseG& root);


    void insertSubtree(NodeBaseG& node, Int &idx, InsertSharedData& data)
    {
    	NodeBaseG left_node = node;
    	insertSubtree(left_node, idx, node, idx, 0, data);

    	self().addTotalKeyCount(data.provider.getTotalKeyCount());
    }


    void insertInternalSubtree(
    		NodeBaseG& left_path,
    		Int left_idx,

    		NodeBaseG& right_path,
    		Int& right_idx,

    		InsertSharedData& data
    );



    void reindexAndUpdateCounters(NodeBaseG& node, Int from, Int count) const
    {
    	auto& self = this->self();

    	if (from + count == self.getNodeSize(node, 0))
    	{
    		self.reindexRegion(node, from, from + count);
    	}
    	else {
    		self.reindex(node);
    	}
    }

    MEMORIA_DECLARE_NODE_FN_RTN(SplitNodeFn, splitTo, Accumulator);
    Accumulator splitLeafNode(NodeBaseG& src, NodeBaseG& tgt, const Position& split_at);
    Accumulator splitNonLeafNode(NodeBaseG& src, NodeBaseG& tgt, Int split_at);

    Accumulator splitNode(NodeBaseG& src, NodeBaseG& tgt, const Position& split_at)
    {
    	auto& self = this->self();

    	if (src->is_leaf())
    	{
    		return self.splitLeafNode(src, tgt, split_at);
    	}
    	else {
    		return self.splitNonLeafNode(src, tgt, split_at.get());
    	}
    }


    MEMORIA_DECLARE_NODE_FN(ForAllIDsFn, forAllValues);
    void forAllIDs(const NodeBaseG& node, Int start, Int end, std::function<void (const ID&, Int)> fn) const;

    void updateChildren(const NodeBaseG& node);
    void updateChildren(const NodeBaseG& node, Int start);
    void updateChildren(const NodeBaseG& node, Int start, Int end);

private:
    void updateChildrenInternal(const NodeBaseG& node, Int start, Int end);
public:



    void fillNodeLeft(NodeBaseG& node, Int from, Int count, InsertSharedData& data);
    void prepareNodeFillmentRight(Int level, Int count, InsertSharedData& data);


    MEMORIA_DECLARE_NODE_FN(MakeRoomFn, insertSpace);
    MEMORIA_DECLARE_NODE_FN_RTN(MoveElementsFn, splitTo, Accumulator);
    MEMORIA_DECLARE_NODE_FN_RTN(IsEmptyFn, isEmpty, bool);
    MEMORIA_DECLARE_NODE_FN_RTN(IsAfterEndFn, isAfterEnd, bool);

    bool isEmpty(const NodeBaseG& node) {
    	return NodeDispatcher::dispatchConstRtn(node, IsEmptyFn());
    }

    bool isAfterEnd(const NodeBaseG& node, const Position& idx, UBigInt active_streams)
    {
    	return NodeDispatcher::dispatchConstRtn(node, IsAfterEndFn(), idx, active_streams);
    }


MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::bt::InsertBatchName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS


M_PARAMS
typename M_TYPE::Accumulator M_TYPE::insertSubtree(NodeBaseG& leaf, Position& idx, ISubtreeProvider& provider)
{
    auto& self = this->self();

	Position sizes = provider.getTotalSize();

    if (self.checkCapacities(leaf, sizes))
    {
    	leaf.update();
    	Accumulator sums = provider.insertIntoLeaf(leaf, idx, sizes);
    	self.updateParent(leaf, sums);
    	return sums;
    }
    else {
    	auto right = leaf;

    	if (leaf->is_root())
    	{
    		self.newRootP(leaf);
    	}

    	if (!self.isAfterEnd(leaf, idx, provider.getActiveStreams()))
    	{
    		right = self.splitLeafP(leaf, idx);
    	}

    	leaf.update();

    	Accumulator sums = provider.insertIntoLeaf(leaf, idx);
    	self.updateParent(leaf, sums);

    	Position remainder = provider.remainder();

    	if (remainder.gtAny(0))
    	{
    		Int path_parent_idx 	= leaf->parent_idx() + 1;
    		Int right_parent_idx 	= right->parent_idx();

    		InsertSharedData data(provider);

    		NodeBaseG leaf_parent = self.getNodeParent(leaf, Allocator::UPDATE);
    		NodeBaseG right_parent = self.getNodeParent(right, Allocator::UPDATE);

    		self.insertInternalSubtree(leaf_parent, path_parent_idx, right_parent, right_parent_idx, data);

    		return sums + data.accumulator;
    	}
    	else {
    		return sums;
    	}
    }
}













M_PARAMS
BigInt M_TYPE::getSubtreeSize(Int level) const
{
    MEMORIA_ASSERT(level, >=, 0);

    BigInt result = 1;

    for (int c = 1; c < level; c++)
    {
        BigInt children_at_level = self().getMaxKeyCountForNode(c == 0, c);

        result *= children_at_level;
    }

    return result;
}





//// ==================================================  PRIVATE API ================================================== ////



M_PARAMS
void M_TYPE::insertInternalSubtree(
        NodeBaseG& left_node,
        Int left_idx,
        NodeBaseG& right_node,
        Int& right_idx,

        InsertSharedData& data)
{
    auto& self = this->self();

	//FIXME: check node->level() after deletion;

    BigInt  subtree_size    = self.getSubtreeSize(left_node->level());

    BigInt  key_count       = data.total - data.start - data.end;

    if (left_node == right_node)
    {
        Int node_capacity = self.getNonLeafCapacity(left_node, data.active_streams);

        if (key_count <= subtree_size * node_capacity)
        {
            // We have enough free space for all subtrees in the current node
            BigInt total    = divide(key_count, subtree_size);

            fillNodeLeft(left_node, left_idx, total, data);

            right_idx       += total;

            data.remains    = data.total - data.start;
        }
        else
        {
            // There is no enough free space in current node for all subtrees
            // split the node and proceed

            //FIXME:
        	right_node = self.splitPathP(left_node, left_idx);

            right_idx = 0;

            insertInternalSubtree(left_node, left_idx, right_node, right_idx, data);
        }
    }
    else {
        Int start_capacity  = self.getNonLeafCapacity(left_node, data.active_streams);
        Int end_capacity    = self.getNonLeafCapacity(right_node, data.active_streams);
        Int total_capacity  = start_capacity + end_capacity;


        BigInt max_key_count = subtree_size * total_capacity;

        if (key_count <= max_key_count)
        {
            // Otherwise fill nodes 'equally'. Each node will have almost
            // equal free space after processing.

            BigInt total_keys           = divide(key_count, subtree_size);

            Int left_count, right_count;

            if (total_keys <= total_capacity)
            {
                Int left_usage  = self.getNodeSize(left_node, 0);
                Int right_usage = self.getNodeSize(right_node, 0);

                if (left_usage < right_usage)
                {
                    left_count  = start_capacity > total_keys ? total_keys : start_capacity;
                    right_count = total_keys - left_count;
                }
                else {
                    right_count = end_capacity > total_keys ? total_keys : end_capacity;
                    left_count  = total_keys - right_count;
                }
            }
            else {
                left_count  = start_capacity;
                right_count = end_capacity;
            }

            fillNodeLeft(left_node, left_idx, left_count, data);

            prepareNodeFillmentRight(right_node->level(), right_count, data);

            data.remains    = data.total - data.start;

            fillNodeLeft(right_node, 0, right_count,  data);

            right_idx = right_count;
        }
        else
        {
            // If there are more keys than these nodes can store,
            // fill nodes fully

            fillNodeLeft(left_node, left_idx, start_capacity, data);

            prepareNodeFillmentRight(left_node->level(), end_capacity, data);

            right_idx += end_capacity;

            // There is something more to insert.
            Int parent_right_idx = right_node->parent_idx();

            NodeBaseG left_parent = self.getNodeParent(left_node, Allocator::READ);
            NodeBaseG right_parent = self.getNodeParent(right_node, Allocator::READ);

            insertInternalSubtree
            (
                    left_parent,
                    left_node->parent_idx() + 1,
                    right_parent,
                    parent_right_idx,
                    data
            );

            fillNodeLeft(right_node, 0, end_capacity, data);
        }
    }
}



M_PARAMS
void M_TYPE::fillNodeLeft(NodeBaseG& node, Int from, Int count, InsertSharedData& data)
{
    auto& self = this->self();

	BigInt subtree_size = me()->getSubtreeSize(node->level());

    self.makeRoom(node, Position(from), Position(count));

    for (Int c = from; c < from + count; c++)
    {
    	BigInt requested_size;

    	if (data.first_cell_key_count > 0)
    	{
    		requested_size              = data.first_cell_key_count;
    		data.first_cell_key_count   = 0;
    	}
    	else {
    		requested_size  = data.remains >= subtree_size ? subtree_size : data.remains;
    	}

    	NonLeafNodeKeyValuePair pair    = data.provider.getKVPair(
    			node->id(),
    			c,
    			requested_size,
    			node->level()
    	);

    	self.setNonLeafKeys(node, c, pair.keys);
    	self.setChildID(node, c, pair.value);

    	data.start      += pair.key_count;
    	data.remains    -= pair.key_count;
    }

    reindexAndUpdateCounters(node, from, count);

    Accumulator sums = self.sums(node, from, from + count);

    self.updateParent(node, sums);

    VectorAdd(data.accumulator, sums);
}


M_PARAMS
void M_TYPE::prepareNodeFillmentRight(Int level, Int count, InsertSharedData& data)
{
    BigInt subtree_size = me()->getSubtreeSize(level);

    if (level > 0)
    {
        BigInt total = subtree_size * count;

        if (data.remains >= total)
        {
            data.end        += total;
            data.remains    -= total;
        }
        else {
            BigInt remainder = data.remains - (subtree_size * count - subtree_size);

            data.end        += data.remains;
            data.remains    = 0;

            data.first_cell_key_count = remainder;
        }
    }
    else {
        data.end        += count;
        data.remains    -= count;
    }
}



M_PARAMS
void M_TYPE::makeRoom(NodeBaseG& node, const Position& start, const Position& count)
{
	auto& self = this->self();

	node.update();

	NodeDispatcher::dispatch(node, MakeRoomFn(), start, count);

	if (!node->is_leaf())
	{
		self.updateChildren(node, start.get() + count.get());
	}
}

M_PARAMS
void M_TYPE::makeRoom(NodeBaseG& node, Int stream, Int start, Int count)
{
    if (count > 0)
    {
    	auto& self = this->self();

    	node.update();
        NodeDispatcher::dispatch(node, MakeRoomFn(), stream, start, count);

        if (!node->is_leaf())
        {
        	self.updateChildren(node, start + count);
        }
    }
}


M_PARAMS
typename M_TYPE::Accumulator M_TYPE::splitLeafNode(NodeBaseG& src, NodeBaseG& tgt, const Position& split_at)
{
	return LeafDispatcher::dispatchRtn(src, tgt, SplitNodeFn(), split_at);
}

M_PARAMS
typename M_TYPE::Accumulator M_TYPE::splitNonLeafNode(NodeBaseG& src, NodeBaseG& tgt, Int split_at)
{
	auto& self = this->self();

	Accumulator accum = NonLeafDispatcher::dispatchRtn(src, tgt, SplitNodeFn(), split_at);

	self.updateChildren(tgt);

	return accum;
}

M_PARAMS
void M_TYPE::forAllIDs(const NodeBaseG& node, Int start, Int end, std::function<void (const ID&, Int)> fn) const
{
	NonLeafDispatcher::dispatchConst(node, ForAllIDsFn(), start, end, fn);
}


M_PARAMS
void M_TYPE::updateChildren(const NodeBaseG& node)
{
	if (!node->is_leaf())
	{
		auto& self = this->self();
		self.updateChildrenInternal(node, 0, self.getNodeSize(node, 0));
	}
}

M_PARAMS
void M_TYPE::updateChildren(const NodeBaseG& node, Int start)
{
	if (!node->is_leaf())
	{
		auto& self = this->self();
		self.updateChildrenInternal(node, start, self.getNodeSize(node, 0));
	}
}

M_PARAMS
void M_TYPE::updateChildren(const NodeBaseG& node, Int start, Int end)
{
	if (!node->is_leaf())
	{
		auto& self = this->self();
		self.updateChildrenInternal(node, start, end);
	}
}


M_PARAMS
void M_TYPE::updateChildrenInternal(const NodeBaseG& node, Int start, Int end)
{
	auto& self = this->self();

	ID node_id = node->id();

	forAllIDs(node, start, end, [&self, &node_id](const ID& id, Int idx)
	{
		NodeBaseG child = self.allocator().getPage(id, Allocator::UPDATE);

		child->parent_id() 	= node_id;
		child->parent_idx() = idx;
	});
}






M_PARAMS
void M_TYPE::newRootP(NodeBaseG& root)
{
	auto& self = this->self();

	root.update();

    NodeBaseG new_root = self.createNode1(root->level() + 1, true, false, root->page_size());

    UBigInt root_active_streams = self.getActiveStreams(root);
    self.layoutNonLeafNode(new_root, root_active_streams);

    self.copyRootMetadata(root, new_root);

    self.root2Node(root);

    Accumulator keys = self.sums(root);

    self.insertNonLeaf(new_root, 0, keys, root->id());

    root->parent_id()  = new_root->id();
    root->parent_idx() = 0;

    self.set_root(new_root->id());
}



#undef M_TYPE
#undef M_PARAMS

} //memoria



#endif
