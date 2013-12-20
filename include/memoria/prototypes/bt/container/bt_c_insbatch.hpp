
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

    typedef typename Base::Metadata                                             Metadata;

    typedef typename Types::Accumulator                                         Accumulator;
    typedef typename Types::Position                                            Position;

    typedef typename Types::PageUpdateMgr                                       PageUpdateMgr;

    typedef typename Types::CtrSizeT                                            CtrSizeT;

    typedef typename Types::Source												Source;


    static const Int Streams                                                    = Types::Streams;

    struct NonLeafNodeKeyValuePair
    {
        Accumulator keys;
        ID          value;
        CtrSizeT    key_count;
    };

    struct ISubtreeProvider
    {
        typedef MyType                      CtrType;

        virtual ~ISubtreeProvider() {}


        virtual NonLeafNodeKeyValuePair getKVPair(const ID& parent_id, Int parent_idx, CtrSizeT count, Int level) = 0;

        virtual CtrSizeT            getTotalLeafCount()                          = 0;
        virtual Position            getTotalSize()                              = 0;
        virtual Position            getTotalInserted()                          = 0;

        virtual Position           	remainder()                                 = 0;

        virtual ISubtreeProvider&   getProvider() {return *this;}

        virtual NodeBaseG 			createLeaf()								= 0;

        virtual UBigInt             getActiveStreams()                          = 0;
    };


    struct ISubtreeLeafProvider: ISubtreeProvider
    {
    	virtual Accumulator insertIntoLeaf(NodeBaseG& leaf)                                             = 0;
    	virtual Accumulator insertIntoLeaf(NodeBaseG& leaf, const Position& from)                       = 0;
    	virtual Accumulator insertIntoLeaf(NodeBaseG& leaf, const Position& from, const Position& size) = 0;
    };



    class AbstractSubtreeProviderBase: public ISubtreeProvider {
    protected:
        MyType& ctr_;
        UBigInt active_streams_;

    public:
        AbstractSubtreeProviderBase(MyType& ctr, UBigInt active_streams = -1ull):
            ctr_(ctr),
            active_streams_(active_streams)
        {}

        virtual UBigInt getActiveStreams() {
            return active_streams_;
        }

        virtual NonLeafNodeKeyValuePair getKVPair(const ID& parent_id, Int parent_idx, CtrSizeT total, Int level)
        {
            CtrSizeT local_count = 0;
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
                CtrSizeT& count,
                const CtrSizeT total,
                Int level
        )
        {
            const Int MAX_CHILDREN = 200;

            NonLeafNodeKeyValuePair pair;
            pair.key_count = 0;

            if (level > 0)
            {
                Int max_keys 		= ctr_.getMaxKeyCountForNode(level == 0, level);

                NodeBaseG node      = ctr_.createNode1(level, false, false);
                node->parent_id()   = parent_id;
                node->parent_idx()  = parent_idx;

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
                    ){
                        children[batch_idx]     =  BuildTree(
                                                        node->id(),
                                                        c,
                                                        count,
                                                        total,
                                                        level - 1
                                                   );

                        pair.key_count          += children[batch_idx].key_count;
                    }

                    setINodeData(children, node, start, local);

                    if (local < MAX_CHILDREN)
                    {
                        break;
                    }
                }

                ctr_.reindex(node);

                ctr_.sums(node, pair.keys);
                pair.value = node->id();
            }
            else
            {
            	NodeBaseG node = this->createLeaf();

            	node->parent_id()   = parent_id;
            	node->parent_idx()  = parent_idx;

            	count++;
            	pair.keys = ctr_.sums(node);

            	pair.value      =  node->id();
            	pair.key_count  += 1;

            }

            return pair;
        }

        template <typename PairType>
        struct SetNodeValuesFn
        {
            const PairType* pairs_;
            Int             start_;
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


    class AbstractSubtreeLeafProviderBase: public AbstractSubtreeProviderBase, public ISubtreeLeafProvider {
    	using ProviderBase = AbstractSubtreeProviderBase;

    public:
    	AbstractSubtreeLeafProviderBase(MyType& ctr, UBigInt active_streams = -1ull):
    		AbstractSubtreeProviderBase(ctr, active_streams)
    {}

    	virtual NodeBaseG createLeaf()
    	{
    		NodeBaseG node = this->ctr_.createNode1(0, false, true);
    		this->insertIntoLeaf(node);

    		return node;
    	}

    	virtual NonLeafNodeKeyValuePair getKVPair(const ID& parent_id, Int parent_idx, CtrSizeT total, Int level) {
    		return ProviderBase::getKVPair(parent_id, parent_idx, total, level);
    	}

    	virtual UBigInt getActiveStreams() {
    		return ProviderBase::getActiveStreams();
    	}
    };



    class DefaultSubtreeProvider: public AbstractSubtreeLeafProviderBase {

        typedef AbstractSubtreeLeafProviderBase                 ProviderBase;

        MyType&     ctr_;
        Position    total_;
        Position    inserted_;

        ISource&    data_source_;

    public:
        DefaultSubtreeProvider(MyType& ctr, const Position& total, ISource& data_source):
            ProviderBase(ctr, total.gtZero()),
            ctr_(ctr),
            total_(total),
            data_source_(data_source)
        {}

        virtual CtrSizeT getTotalLeafCount()
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
                    const Position& pos,
                    const Position& remainder
            )
            {
                LayoutManager<Node> manager(node);

                Position capacity;

                provider->data_source_.newNode(&manager, capacity.values());

                Position size;
                if (remainder.lteAll(capacity))
                {
                    size = remainder;
                }
                else {
                    size = capacity;
                }

                node->insert(provider->data_source_, pos, size);

                //FIXME: remove explicit node reindexing
                node->reindex();

                provider->inserted_ += size;

                Accumulator sums;
                node->sums(pos, pos + size, sums, provider->getActiveStreams());
                return sums;
            }
        };



        virtual Accumulator insertIntoLeaf(NodeBaseG& leaf)
        {
            Position pos;
            Position remainder = this->remainder();
            return LeafDispatcher::dispatchRtn(leaf, InsertIntoLeafFn(), this, pos, remainder);
        }

        virtual Accumulator insertIntoLeaf(NodeBaseG& leaf, const Position& from)
        {
            Position remainder = this->remainder();
            return LeafDispatcher::dispatchRtn(leaf, InsertIntoLeafFn(), this, from, remainder);
        }

        virtual Accumulator insertIntoLeaf(NodeBaseG& leaf, const Position& from, const Position& size)
        {
            return LeafDispatcher::dispatchRtn(leaf, InsertIntoLeafFn(), this, from, size);
        }
    };


    class LeafNodeListSubtreeProvider: public AbstractSubtreeProviderBase {

    	typedef AbstractSubtreeProviderBase                 ProviderBase;

    	MyType&     ctr_;
    	Position    total_sizes_;
    	Position    inserted_;

    	CtrSizeT	total_leafs_;

    	ID 			list_head_;


    public:
    	LeafNodeListSubtreeProvider(MyType& ctr, const Position& total, CtrSizeT total_leafs, const ID& list_head):
    		ProviderBase(ctr, total.gtZero()),
    		ctr_(ctr),
    		total_sizes_(total),
    		total_leafs_(total_leafs),
    		list_head_(list_head)
    {}

    	virtual CtrSizeT getTotalLeafCount() {
    		return total_leafs_;
    	}

    	virtual Position getTotalSize()
    	{
    		return total_sizes_;
    	}

    	virtual Position getTotalInserted()
    	{
    		return inserted_;
    	}

    	virtual Position remainder()
    	{
    		return total_sizes_ - inserted_;
    	}

    	virtual NodeBaseG createLeaf()
    	{
    		NodeBaseG next = ctr_.allocator().getPageForUpdate(list_head_, ctr_.name());
    		list_head_ = next->next_leaf_id();

    		next->next_leaf_id().clear();

    		inserted_ += ctr_.getNodeSizes(next);

    		return next;
    	}
    };





    struct InsertSharedData {

        ISubtreeProvider& provider;

        Accumulator accumulator;

        CtrSizeT start;
        CtrSizeT end;
        CtrSizeT total;

        CtrSizeT remains;

        CtrSizeT first_cell_key_count;

        UBigInt active_streams;

        InsertSharedData(ISubtreeProvider& provider_):
            provider(provider_),
            start(0),
            end(0),
            total(provider_.getTotalLeafCount()),
            remains(total),
            first_cell_key_count(0),
            active_streams(provider.getActiveStreams())
        {}
    };



    CtrSizeT divide(CtrSizeT op1, CtrSizeT op2)
    {
        return (op1 / op2) + ((op1 % op2 == 0) ?  0 : 1);
    }

    CtrSizeT getSubtreeSize(Int level) const;

    void makeRoom(NodeBaseG& node, const Position& start, const Position& count);
    void makeRoom(NodeBaseG& node, Int stream, Int start, Int count);

    Accumulator insertSubtree(NodeBaseG& node, Position& idx, ISubtreeLeafProvider& provider);



    Accumulator insertSource(NodeBaseG& node, Position& idx, Source& source);
    bool insertToLeaf(NodeBaseG& node, Position& idx, Source& source, Accumulator& sums);
    std::pair<CtrSizeT, ID> createLeafList(Source& source);

    // These methods have to be defined in actual containers
    //Accumulator insertSourceToLeaf(NodeBaseG& node, Position& idx, Source& source) {}
    //Accumulator appendToLeaf(NodeBaseG& node, Position& idx, Source& source) {}
    //void fillNewLeaf(NodeBaseG& node, Source& source) {}



    void newRootP(NodeBaseG& root);

    void insertInternalSubtree(
            NodeBaseG& left_path,
            Int left_idx,

            NodeBaseG& right_path,
            Int& right_idx,

            InsertSharedData& data
    );



    void reindexAndUpdateCounters(NodeBaseG& node, Int from, Int count) const
    {
        self().reindex(node);
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
typename M_TYPE::Accumulator M_TYPE::insertSubtree(NodeBaseG& leaf, Position& idx, ISubtreeLeafProvider& provider)
{
    auto& self = this->self();

    Position sizes = provider.getTotalSize();

    if (self.checkCapacities(leaf, sizes))
    {
        self.updatePageG(leaf);

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

        self.updatePageG(leaf);

        Accumulator sums = provider.insertIntoLeaf(leaf, idx);

        self.updateParent(leaf, sums);

        Position remainder = provider.remainder();

        if (remainder.gtAny(0))
        {
            Int path_parent_idx     = leaf->parent_idx() + 1;
            Int right_parent_idx    = right->parent_idx();

            InsertSharedData data(provider);

            NodeBaseG leaf_parent = self.getNodeParentForUpdate(leaf);
            NodeBaseG right_parent = self.getNodeParentForUpdate(right);

            self.insertInternalSubtree(leaf_parent, path_parent_idx, right_parent, right_parent_idx, data);

            return sums + data.accumulator;
        }
        else {
            return sums;
        }
    }
}



M_PARAMS
typename M_TYPE::Accumulator M_TYPE::insertSource(NodeBaseG& leaf, Position& idx, Source& source)
{
    auto& self = this->self();

    Position sizes = self.getRemainderSize(source);

    UBigInt active_streams = sizes.gtZero();

    Accumulator sums;
    if (self.insertToLeaf(leaf, idx, source, sums))
    {
        self.updateParent(leaf, sums);

        return sums;
    }
    else {
        auto right = leaf;

        if (leaf->is_root())
        {
            self.newRootP(leaf);
        }

        if (!self.isAfterEnd(leaf, idx, active_streams))
        {
            right = self.splitLeafP(leaf, idx);
        }

        Accumulator sums = self.appendToLeaf(leaf, idx, source);

        self.updateParent(leaf, sums);

        Position remainder = self.getRemainderSize(source);

        if (remainder.gtAny(0))
        {
            Int path_parent_idx     = leaf->parent_idx() + 1;
            Int right_parent_idx    = right->parent_idx();

            auto pair = self.createLeafList(source);

            LeafNodeListSubtreeProvider provider(self, remainder, pair.first, pair.second);

            InsertSharedData data(provider);

            NodeBaseG leaf_parent = self.getNodeParentForUpdate(leaf);
            NodeBaseG right_parent = self.getNodeParentForUpdate(right);

            self.insertInternalSubtree(leaf_parent, path_parent_idx, right_parent, right_parent_idx, data);

            return sums + data.accumulator;
        }
        else {
            return sums;
        }
    }
}


M_PARAMS
bool M_TYPE::insertToLeaf(NodeBaseG& leaf, Position& idx, Source& source, Accumulator& sums)
{
	auto& self = this->self();

	Position sizes = self.getRemainderSize(source);

	if (self.checkCapacities(leaf, sizes))
	{
		self.updatePageG(leaf);
		sums = self.insertSourceToLeaf(leaf, idx, source);
		return true;
	}
	else {
		return false;
	}
}




M_PARAMS
std::pair<typename M_TYPE::CtrSizeT, typename M_TYPE::ID> M_TYPE::createLeafList(Source& source)
{
	auto& self = this->self();

	CtrSizeT 	total 	= 0;
	NodeBaseG	head;
	NodeBaseG	current;

	Int page_size = self.getRootMetadata().page_size();

	while (true)
	{
		Position remainder = self.getRemainderSize(source);

		if (remainder.gtAny(0))
		{
			total++;

			NodeBaseG node = self.createNode1(0, false, true, page_size);

			self.fillNewLeaf(node, source);

			if (head.isSet())
			{
				current->next_leaf_id() = node->id();
				current 				= node;
			}
			else {
				head = current = node;
			}
		}
		else {
			break;
		}
	}

	return std::make_pair(total, head.isSet() ? head->id() : ID(0));
}











M_PARAMS
typename M_TYPE::CtrSizeT M_TYPE::getSubtreeSize(Int level) const
{
    MEMORIA_ASSERT(level, >=, 0);

    CtrSizeT result = 1;

    for (int c = 1; c < level; c++)
    {
        CtrSizeT children_at_level = self().getMaxKeyCountForNode(c == 0, c);

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

    CtrSizeT subtree_size	= self.getSubtreeSize(left_node->level());
    CtrSizeT leaf_count     = data.total - data.start - data.end;

    if (left_node == right_node)
    {
        Int node_capacity = self.getNonLeafCapacity(left_node, data.active_streams);

        if (leaf_count <= subtree_size * node_capacity)
        {
            // We have enough free space for all subtrees in the current node
            CtrSizeT total  = divide(leaf_count, subtree_size);

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


        CtrSizeT max_leaf_count = subtree_size * total_capacity;

        if (leaf_count <= max_leaf_count)
        {
            // Otherwise fill nodes 'equally'. Each node will have almost
            // equal free space after processing.

            CtrSizeT total_leafs = divide(leaf_count, subtree_size);

            Int left_count, right_count;

            if (total_leafs <= total_capacity)
            {
                Int left_usage  = self.getNodeSize(left_node, 0);
                Int right_usage = self.getNodeSize(right_node, 0);

                if (left_usage < right_usage)
                {
                    left_count  = start_capacity > total_leafs ? total_leafs : start_capacity;
                    right_count = total_leafs - left_count;
                }
                else {
                    right_count = end_capacity > total_leafs ? total_leafs : end_capacity;
                    left_count  = total_leafs - right_count;
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

            NodeBaseG left_parent = self.getNodeParent(left_node);
            NodeBaseG right_parent = self.getNodeParent(right_node);

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

    CtrSizeT subtree_size = self.getSubtreeSize(node->level());

    self.makeRoom(node, Position(from), Position(count));

    for (Int c = from; c < from + count; c++)
    {
        CtrSizeT requested_size;

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
	CtrSizeT subtree_size = self().getSubtreeSize(level);

    if (level > 0)
    {
        CtrSizeT total = subtree_size * count;

        if (data.remains >= total)
        {
            data.end        += total;
            data.remains    -= total;
        }
        else {
            CtrSizeT remainder = data.remains - (subtree_size * count - subtree_size);

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

    self.updatePageG(node);

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

        self.updatePageG(node);
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

//M_PARAMS
//void M_TYPE::forAllIDs(const NodeBaseG& node, Int start, Int end, std::function<void (const ID&, Int)> fn) const
//{
//    NonLeafDispatcher::dispatchConst(node, ForAllIDsFn(), start, end, fn);
//}


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

    self.forAllIDs(node, start, end, [&self, &node_id](const ID& id, Int idx)
    {
        NodeBaseG child = self.allocator().getPageForUpdate(id, self.master_name());

        child->parent_id()  = node_id;
        child->parent_idx() = idx;
    });
}






M_PARAMS
void M_TYPE::newRootP(NodeBaseG& root)
{
    auto& self = this->self();

    self.updatePageG(root);

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
