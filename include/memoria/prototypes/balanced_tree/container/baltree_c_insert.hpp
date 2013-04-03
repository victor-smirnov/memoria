
// Copyright Victor Smirnov 2011-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_MODEL_INSERT_BATCH_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_MODEL_INSERT_BATCH_HPP

#include <memoria/prototypes/balanced_tree/baltree_tools.hpp>
#include <memoria/core/container/macros.hpp>

#include <vector>

namespace memoria {

using namespace memoria::balanced_tree;
using namespace memoria::core;

using namespace std;

MEMORIA_CONTAINER_PART_BEGIN(memoria::balanced_tree::InsertBatchName)

    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Base::ID                                                   ID;
    
    typedef typename Types::NodeBase                                            NodeBase;
    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::TreeNodePage                                         TreeNodePage;
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

        virtual NonLeafNodeKeyValuePair getKVPair(BigInt begin, BigInt count, Int level)    	= 0;
        virtual BigInt				    getTotalKeyCount()                          			= 0;
        virtual Position				getTotalSize()       									= 0;
        virtual Position				getTotalInserted()       								= 0;

        virtual Position				remainder()       										= 0;

        virtual ISubtreeProvider&       getProvider() {return *this;}

        virtual Accumulator insertIntoLeaf(NodeBaseG& leaf)  											= 0;
        virtual Accumulator insertIntoLeaf(NodeBaseG& leaf, const Position& from)  						= 0;
        virtual Accumulator insertIntoLeaf(NodeBaseG& leaf, const Position& from, const Position& size)	= 0;
    };

    BigInt divide(BigInt op1, BigInt op2)
    {
        return (op1 / op2) + ((op1 % op2 == 0) ?  0 : 1);
    }

    BigInt getSubtreeSize(Int level) const;

    MEMORIA_DECLARE_NODE_FN_RTN(GetCountersFn, getCounters, Accumulator);
    Accumulator getCounters(const NodeBaseG& node, const Position& from, const Position& count) const;

    void makeRoom(TreePath& path, Int level, const Position& start, const Position& count) const;

    void updateUp(TreePath& path, Int level, Int idx, const Accumulator& counters, bool reindex_fully = false);

    void updateParentIfExists(TreePath& path, Int level, const Accumulator& counters);

    Accumulator insertSubtree(TreePath& path, Position& idx, ISubtreeProvider& provider);

    void splitPath(TreePath& left, TreePath& right, Int level, const Position& idx);

    void newRoot(TreePath& path);

    class DefaultSubtreeProviderBase: public ISubtreeProvider {

        MyType& ctr_;

    public:
        DefaultSubtreeProviderBase(MyType& ctr): ctr_(ctr) {}

        virtual NonLeafNodeKeyValuePair getKVPair(BigInt begin, BigInt total, Int level)
        {
            BigInt local_count = 0;
            return BuildTree(begin, local_count, total, level - 1);
        }

        MyType& ctr() {
            return ctr_;
        }

        const MyType& ctr() const {
            return ctr_;
        }


    private:
        NonLeafNodeKeyValuePair BuildTree(BigInt start, BigInt& count, const BigInt total, Int level)
        {
            NonLeafNodeKeyValuePair pair;
            pair.key_count = 0;



            if (level > 0)
            {
            	Int max_keys = ctr_.getMaxKeyCountForNode(false, level == 0, level);

            	// FIXME: buffer size can be too small
                NonLeafNodeKeyValuePair children[2000];

                Int local = 0;
                for (Int c = 0; c < max_keys && count < total; c++, local++)
                {
                    children[c]     =  BuildTree(start, count, total, level - 1);
                    pair.key_count  += children[c].key_count;
                }

                NodeBaseG node = ctr_.createNode(level, false, false);

                setINodeData(children, node, local);

                pair.keys  = ctr_.getMaxKeys(node);
                pair.value = node->id();
            }
            else
            {
                NodeBaseG node = ctr_.createNode(level, false, true);

                count++;
                pair.keys = this->insertIntoLeaf(node);

                pair.value      =  node->id();
                pair.key_count  += 1;
            }

            return pair;
        }

        template <typename PairType, typename ParentPairType>
        struct SetNodeValuesFn
        {
            PairType*       pairs_;
            Int             count_;

            ParentPairType  total_;

            SetNodeValuesFn(PairType* pairs, Int count): pairs_(pairs), count_(count) {}

            template <typename Node>
            void operator()(Node* node)
            {
                for (Int c = 0; c < count_; c++)
                {
                    for (Int d = 0; d < Indexes; d++)
                    {
                        node->map().key(d, c) = pairs_[c].keys[d];
                    }

                    node->map().data(c) = pairs_[c].value;
                }

                node->set_children_count(count_);

                node->map().reindex();

                for (Int d = 0; d < Indexes; d++)
                {
                    total_.keys[d] = node->map().maxKey(d);
                }

                total_.value = node->id();
            }
        };

        template <typename PairType>
        NonLeafNodeKeyValuePair setINodeData(PairType* data, NodeBaseG& node, Int count)
        {
            SetNodeValuesFn<PairType, NonLeafNodeKeyValuePair> fn(data, count);
            NonLeafDispatcher::dispatch(node, fn);
            return fn.total_;
        }
    };




//private:

    struct InsertSharedData
    {
        ISubtreeProvider& provider;

        Accumulator accumulator;

        BigInt start;
        BigInt end;
        BigInt total;

        BigInt remains;

        BigInt first_cell_key_count;

        InsertSharedData(ISubtreeProvider& provider_):
            provider(provider_), start(0), end(0),
            total(provider_.getTotalKeyCount()), remains(total), first_cell_key_count(0) {}
    };


    void insertSubtree(TreePath& path, Int &idx, InsertSharedData& data)
    {
        TreePath left_path = path;
        insertSubtree(left_path, idx, path, idx, 0, data);

        me()->finishPathStep(path, idx);

        me()->addTotalKeyCount(data.provider.getTotalKeyCount());
    }


    void insertInternalSubtree(
            TreePath& left_path,
            Int left_idx,
            TreePath& right_path,
            Int& right_idx,
            Int level,
            InsertSharedData& data);



    void reindexAndUpdateCounters(NodeBaseG& node, Int from, Int count) const
    {
        if (from + count == node->children_count())
        {
            me()->reindexRegion(node, from, from + count);
        }
        else {
            me()->reindex(node);
        }
    }

    Accumulator moveElements(
    		NodeBaseG& srt,
    		NodeBaseG& tgt,
    		const Position& from,
    		const Position& tgt_shift = Position()
    ) const;

    void fillNodeLeft(TreePath& path, Int level, Int from, Int count, InsertSharedData& data);
    void prepareNodeFillmentRight(Int level, Int count, InsertSharedData& data);

    TreePathItem split(TreePath& path, Int level, const Position& idx);
    void         split(TreePath& left, TreePath& right, Int level, const Position& idx);

public:
    MEMORIA_DECLARE_NODE_FN(MakeRoomFn, insertSpace);
    MEMORIA_DECLARE_NODE_FN_RTN(MoveElementsFn, moveElements, Accumulator);
    MEMORIA_DECLARE_NODE_FN_RTN(IsEmptyFn, isEmpty, bool);
    MEMORIA_DECLARE_NODE_FN_RTN(IsAfterEndFn, isAfterEnd, bool);

    bool isEmpty(const NodeBaseG& node) {
    	return NodeDispatcher::dispatchConstRtn(node, IsEmptyFn());
    }

    bool isAfterEnd(const NodeBaseG& node, const Position& idx) {
    	return NodeDispatcher::dispatchConstRtn(node, IsAfterEndFn(), idx);
    }


MEMORIA_CONTAINER_PART_END


#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::balanced_tree::InsertBatchName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS


M_PARAMS
typename M_TYPE::Accumulator M_TYPE::insertSubtree(TreePath& path, Position& idx, ISubtreeProvider& provider)
{
    auto& self = this->self();

	NodeBaseG& leaf = path.leaf();

    Position sizes = provider.getTotalSize();

    if (self.checkCapacities(leaf, sizes))
    {
    	return provider.insertIntoLeaf(leaf, idx, sizes);
    }
    else {
    	TreePath right = path;

    	if (self.isEmpty(leaf) && path.getSize() == 1)
    	{
    		self.newRoot(path);
    		right = path;
    	}
    	else if (!self.isAfterEnd(leaf, idx))
    	{
    		self.splitPath(path, right, 0, idx);
    	}

    	Accumulator ls = provider.insertIntoLeaf(leaf, idx);
    	self.updateUp(path, 1, path[0].parent_idx(), ls, true);

    	Int parent_idx = path.leaf().parent_idx() + 1;

    	Position remainder = provider.remainder();

    	if (remainder > 0)
    	{
    		Int path_parent_idx 	= parent_idx;
    		Int right_parent_idx 	= parent_idx;

    		InsertSharedData data(provider);

    		self.insertInternalSubtree(path, path_parent_idx, right, right_parent_idx, 1, data);

    		return ls + data.accumulator;
    	}
    	else {
    		return ls;
    	}
    }
}








M_PARAMS
void M_TYPE::splitPath(TreePath& left, TreePath& right, Int level, const Position& idx)
{
	auto& ctr = self();

    if (level < left.getSize() - 1)
    {
        NodeBaseG& parent = left[level + 1].node();

        if (ctr.getCapacity(parent) == 0)
        {
            Int idx_in_parent = left[level].parent_idx();
            ctr.splitPath(left, right, level + 1, Position(idx_in_parent + 1));
        }

        ctr.split(left, right, level, idx);
    }
    else
    {
        ctr.newRoot(left);

        right.resize(left.getSize());
        right[level + 1] = left[level + 1];

        ctr.split(left, right, level, idx);
    }
}




//M_PARAMS
//void M_TYPE::insertBatch(Iterator& iter, const LeafNodeKeyValuePair* pairs, BigInt size)
//{
//    ArraySubtreeProvider provider(*me(), size, pairs);
//
//    me()->insertSubtree(iter, provider);
//}
//
//M_PARAMS
//void M_TYPE::insertBatch(Iterator& iter, const LeafPairsVector& pairs)
//{
//    ArraySubtreeProvider provider(*me(), pairs.size(), &pairs.at(0));
//
//    me()->insertSubtree(iter, provider);
//}


M_PARAMS
BigInt M_TYPE::getSubtreeSize(Int level) const
{
    MEMORIA_ASSERT(level, >=, 0);

    BigInt result = 1;

    for (int c = 1; c < level; c++)
    {
        BigInt children_at_level = me()->getMaxKeyCountForNode(false, c == 0, c);

        result *= children_at_level;
    }

    return result;
}



M_PARAMS
typename M_TYPE::Accumulator M_TYPE::getCounters(const NodeBaseG& node, const Position& from, const Position& count) const
{
	return NodeDispatcher::dispatchConstRtn(node, GetCountersFn(), from, count);
}



M_PARAMS
void M_TYPE::updateUp(TreePath& path, Int level, Int idx, const Accumulator& counters, bool reindex_fully)
{
    for (Int c = level; c < path.getSize(); c++)
    {
        if (me()->updateCounters(path[c].node(), idx, counters, reindex_fully))
        {
            break;
        }
        else {
            idx = path[c].parent_idx();
        }
    }
}

M_PARAMS
void M_TYPE::updateParentIfExists(TreePath& path, Int level, const Accumulator& counters)
{
    if (level < path.getSize() - 1)
    {
        me()->updateUp(path, level + 1, path[level].parent_idx(), counters);
    }
}





//// ==================================================  PRIVATE API ================================================== ////



M_PARAMS
void M_TYPE::insertInternalSubtree(
        TreePath& left_path,
        Int left_idx,
        TreePath& right_path,
        Int& right_idx,
        Int level,
        InsertSharedData& data)
{
    auto& self = this->self();

	//FIXME: check node->level() after deletion;

    BigInt  subtree_size    = me()->getSubtreeSize(level);

    BigInt  key_count       = data.total - data.start - data.end;

    NodeBaseG& left_node    = left_path[level].node();
    NodeBaseG& right_node   = right_path[level].node();

    if (left_node == right_node)
    {
        Int node_capacity = me()->getCapacity(left_node);

        if (key_count <= subtree_size * node_capacity)
        {
            // We have enough free space for all subtrees in the current node
            BigInt total    = divide(key_count, subtree_size);

            fillNodeLeft(left_path, level, left_idx, total, data);

            right_path.moveRight(level - 1, 0, total);

            right_idx       += total;

            data.remains    = data.total - data.start;
        }
        else
        {
            // There is no enough free space in current node for all subtrees
            // split the node and proceed

            //FIXME:
        	self.splitPath(left_path, right_path, level, Position(left_idx));

            right_idx  = 0;

            insertInternalSubtree(left_path, left_idx, right_path, right_idx, level, data);
        }
    }
    else {
        Int start_capacity  = me()->getCapacity(left_node);
        Int end_capacity    = me()->getCapacity(right_node);
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
                Int left_usage  = left_path[level]->children_count();
                Int right_usage = right_path[level]->children_count();

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

            fillNodeLeft(left_path, level,   left_idx,  left_count,  data);

            prepareNodeFillmentRight(level, right_count, data);

            data.remains    = data.total - data.start;

            fillNodeLeft(right_path, level, 0, right_count,  data);

            right_idx = right_count;
        }
        else
        {
            // If there are more keys than these nodes can store,
            // fill nodes fully

            fillNodeLeft(left_path,   level, left_idx,  start_capacity,  data);

            prepareNodeFillmentRight(level, end_capacity, data);

            right_idx += end_capacity;

            // There is something more to insert.
            Int parent_right_idx = right_path[level].parent_idx();
            insertInternalSubtree
            (
                    left_path,
                    left_path[level].parent_idx() + 1,
                    right_path,
                    parent_right_idx,
                    level + 1,
                    data
            );

            fillNodeLeft(right_path, level, 0, end_capacity, data);
        }
    }
}






M_PARAMS
void M_TYPE::fillNodeLeft(TreePath& path, Int level, Int from, Int count, InsertSharedData& data)
{
    auto& self = this->self();

	NodeBaseG& node = path[level].node();

    BigInt subtree_size = me()->getSubtreeSize(level);

    self.makeRoom(path, level, Position(from), Position(count));

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
    			data.start,
    			requested_size,
    			level
    	);

    	self.setKeys(node, c, pair.keys);
    	self.setChildID(node, c, pair.value);

    	data.start      += pair.key_count;
    	data.remains    -= pair.key_count;
    }

    reindexAndUpdateCounters(node, from, count);

    Accumulator accumulator = self.getCounters(node, Position(from), Position(count));

    self.updateParentIfExists(path, level, accumulator);

    data.accumulator += accumulator;
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
void M_TYPE::makeRoom(TreePath& path, Int level, const Position& start, const Position& count) const
{
    //if (count > 0)
    {
        path[level].node().update();
        NodeDispatcher::dispatch(path[level].node(), MakeRoomFn(), start, count);

        if (level > 0)
        {
        	path.moveRight(level - 1, start.get(), count.get());
        }
    }
}

M_PARAMS
typename M_TYPE::Accumulator M_TYPE::moveElements(
		NodeBaseG& src,
		NodeBaseG& tgt,
		const Position& from,
		const Position& shift
) const
{
    return NodeDispatcher::dispatchRtn(src, tgt, MoveElementsFn(), from, shift);
}





M_PARAMS
typename M_TYPE::TreePathItem M_TYPE::split(TreePath& path, Int level, const Position& idx)
{
    auto& ctr = self();

	NodeBaseG& node     = path[level].node();
    NodeBaseG& parent   = path[level + 1].node();

    Int parent_idx      = path[level].parent_idx();

    node.update();
    parent.update();

    NodeBaseG other = ctr.createNode(level, false, node->is_leaf(), node->page_size());

    Accumulator keys = ctr.moveElements(node, other, idx);

    //FIXME:: Make room in the parent
    ctr.makeRoom(path, level + 1, parent_idx + 1, 1);

    ctr.setChildID(parent, parent_idx + 1, other->id());

    //FIXME: Should we proceed up to the root here in general case?
    ctr.updateCounters(parent, parent_idx,    -keys);
    ctr.updateCounters(parent, parent_idx + 1, keys, true);

    if (level > 0)
    {
        if (path[level - 1].parent_idx() < idx)
        {
            return TreePathItem(other, parent_idx + 1);
        }
        else {
            TreePathItem item = path[level];

            path[level    ].node()          = other;
            path[level    ].parent_idx()++;

            path[level - 1].parent_idx()    -= idx;

            return item;
        }
    }
    else {
        return TreePathItem(other, parent_idx + 1);
    }
}

M_PARAMS
void M_TYPE::split(TreePath& left, TreePath& right, Int level, const Position& idx)
{
	auto& ctr = self();

    NodeBaseG& left_node    = left[level].node();

    NodeBaseG& left_parent  = left[level + 1].node();
    NodeBaseG& right_parent = right[level + 1].node();

    left_node.update();
    left_parent.update();
    right_parent.update();

    NodeBaseG other = ctr.createNode(level, false, left_node->is_leaf(), left_node->page_size());

    Accumulator keys = ctr.moveElements(left_node, other, idx);

    Int parent_idx = left[level].parent_idx();

    if (right_parent == left_parent)
    {
    	ctr.makeRoom(left, level + 1,  Position(parent_idx + 1), Position(1));
        ctr.setChildID(left_parent, parent_idx + 1, other->id());

        //FIXME: should we proceed up to the root?
        ctr.updateCounters(left_parent, parent_idx,    -keys);
        ctr.updateCounters(left_parent, parent_idx + 1, keys, true);

        right[level].node()         = other;
        right[level].parent_idx()   = parent_idx + 1;
    }
    else {
    	ctr.makeRoom(right, level + 1, Position(0), Position(1));
        ctr.setChildID(right_parent, 0, other->id());

        ctr.updateUp(left,  level + 1, parent_idx, -keys);
        ctr.updateUp(right, level + 1, 0,           keys, true);

        right[level].node()         = other;
        right[level].parent_idx()   = 0;
    }

    if (level > 0)
    {
    	right.moveLeft(level - 1, 0, idx.get());
    }
}




M_PARAMS
void M_TYPE::newRoot(TreePath& path)
{
    NodeBaseG& root         = path[path.getSize() - 1].node(); // page == root
    root.update();

    NodeBaseG new_root      = me()->createNode(root->level() + 1, true, false, root->page_size());

    me()->copyRootMetadata(root, new_root);

    me()->root2Node(root);

    Accumulator keys = root->is_leaf() ? me()->getLeafMaxKeys(root) : me()->getMaxKeys(root);
    me()->setKeys(new_root, 0, keys);
    me()->setChildID(new_root, 0, root->id());
    me()->setChildrenCount(new_root, 1);
    me()->reindex(new_root);

    me()->set_root(new_root->id());

    path.append(TreePathItem(new_root));

    path[path.getSize() - 2].parent_idx() = 0;
}

#undef M_TYPE
#undef M_PARAMS

} //memoria



#endif
