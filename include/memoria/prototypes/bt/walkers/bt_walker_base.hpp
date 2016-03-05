
// Copyright Victor Smirnov 2013-2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_WALKER_BASE_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_WALKER_BASE_HPP

#include <memoria/core/types/types.hpp>
#include <memoria/core/types/list/list_tree.hpp>
#include <memoria/prototypes/bt/walkers/bt_walker_base_detail.hpp>

#include <memoria/core/types/algo/for_each.hpp>

#include <tuple>

namespace memoria {

namespace bt {
template <typename Types, typename LeafPath_>
struct WalkerTypes: Types {
	using LeafPath 		= LeafPath_;
};


class StreamOpResult {
	Int idx_;
	Int start_;
	bool out_of_range_;
	bool empty_;

public:
	StreamOpResult(Int idx, Int start, bool out_of_range, bool empty = false): idx_(idx), start_(start), out_of_range_(out_of_range), empty_(empty) {}

	Int idx() const {
		return idx_;
	}

	Int start() const {
		return start_;
	}

	bool out_of_range(){
		return out_of_range_;
	}

	bool empty(){
		return empty_;
	}
};



template <
    typename Types,
    typename MyType
>
class WalkerBase {
public:
    typedef Iter<typename Types::IterTypes>                                     Iterator;
    typedef typename Types::IteratorBranchNodeEntry                             IteratorBranchNodeEntry;
    typedef typename Types::LeafStreamsStructList                               LeafStructList;
    typedef typename Types::LeafRangeList                                 		LeafRangeList;
    typedef typename Types::LeafRangeOffsetList                                 LeafRangeOffsetList;



    static const Int Streams                                                    = Types::Streams;

    using Position = typename Types::Position;
    using LeafPath = typename Types::LeafPath;

    static const Int Stream = ListHead<LeafPath>::Value;

protected:

    Int leaf_index_;

    Int idx_backup_;
    Position branch_size_prefix_backup_;

    Position branch_size_prefix_;
    IteratorBranchNodeEntry branch_prefix_;
    IteratorBranchNodeEntry leaf_prefix_;

    bool compute_branch_ 	= true;
    bool compute_leaf_ 		= true;


public:

    template <typename LeafPath>
    using AccumItemH = typename Types::template AccumItemH<LeafPath>;

protected:

    struct FindBranchFn {
        MyType& walker_;

        FindBranchFn(MyType& walker): walker_(walker) {}

        template <Int ListIdx, typename StreamType, typename... Args>
        StreamOpResult stream(const StreamType* stream, bool root, Int index, Int start, Args&&... args)
        {
            StreamOpResult result = walker_.template find_non_leaf<ListIdx>(stream, root, index, start, std::forward<Args>(args)...);

            // TODO: should we also forward args... to this call?
            walker_.template postProcessBranchStream<ListIdx>(stream, start, result.idx());

            return result;
        }
    };


    struct FindLeafFn {
        MyType& walker_;

        FindLeafFn(MyType& walker): walker_(walker) {}

        template <Int ListIdx, typename StreamType, typename... Args>
        StreamOpResult stream(const StreamType* stream, Int start, Args&&... args)
        {
        	StreamOpResult result = walker_.template find_leaf<ListIdx>(stream, start, std::forward<Args>(args)...);

        	// TODO: should we also forward args... to this call?
            walker_.template postProcessLeafStream<ListIdx>(stream, start, result.idx());

            return result;
        }
    };

    struct ProcessBranchCmdFn {
        MyType& walker_;

        ProcessBranchCmdFn(MyType& walker): walker_(walker) {}

        template <Int ListIdx, typename StreamType, typename... Args>
        void stream(const StreamType* stream, WalkCmd cmd, Args&&... args)
        {
        	walker_.template process_branch_cmd<ListIdx>(stream, cmd, std::forward<Args>(args)...);
        }
    };

    struct ProcessLeafCmdFn {
    	MyType& walker_;

    	ProcessLeafCmdFn(MyType& walker): walker_(walker) {}

    	template <Int ListIdx, typename StreamType, typename... Args>
    	void stream(const StreamType* stream, WalkCmd cmd, Args&&... args)
    	{
    		walker_.template process_leaf_cmd<ListIdx>(stream, cmd, std::forward<Args>(args)...);
    	}
    };


public:

    WalkerBase(Int leaf_index):
        leaf_index_(leaf_index)
    {}

    void result() const {}


    void empty(Iterator& iter)
    {
        iter.idx() = 0;
    }

    static constexpr Int current_stream() {
        return ListHead<LeafPath>::Value;
    }

    Int leaf_index() const
    {
        return leaf_index_;
    }

    const bool& compute_branch() const {
    	return compute_branch_;
    }

    bool& compute_branch() {
    	return compute_branch_;
    }

    const bool& compute_leaf() const {
    	return compute_leaf_;
    }

    bool& compute_leaf() {
    	return compute_leaf_;
    }

    static constexpr Int branchIndex(Int leaf_index)
    {
    	return memoria::bt::LeafToBranchIndexTranslator<LeafStructList, LeafPath, 0>::BranchIndex + leaf_index;
    }


    template <typename LeafPath>
    auto branch_index(Int index)
    {
    	return AccumItemH<LeafPath>::value(index, branch_prefix_);
    }

    template <typename LeafPath>
    auto branch_index(Int index) const
    {
    	return AccumItemH<LeafPath>::cvalue(index, branch_prefix_);
    }

    template <typename LeafPath>
    auto leaf_index(Int index)
    {
    	return AccumItemH<LeafPath>::value(index, leaf_prefix_);
    }

    template <typename LeafPath>
    auto leaf_index(Int index) const
    {
    	return AccumItemH<LeafPath>::cvalue(index, leaf_prefix_);
    }


    template <typename LeafPath>
    auto total_index(Int index) const
    {
    	return AccumItemH<LeafPath>::cvalue(index, branch_prefix_) +
    			AccumItemH<LeafPath>::cvalue(index, leaf_prefix_);
    }

    void prepare(Iterator& iter)
    {
        branch_prefix_ 		= iter.cache().prefixes();
        leaf_prefix_ 		= iter.cache().leaf_prefixes();
        branch_size_prefix_	= iter.cache().size_prefix();

        branch_size_prefix_backup_	= iter.cache().size_prefix();
        idx_backup_			= iter.idx();
    }

    void finish(Iterator& iter, Int idx, WalkCmd cmd) const
    {
    	iter.finish_walking(idx, self(), cmd);

    	iter.idx() = idx;

        iter.cache().prefixes() 	 = branch_prefix_;
        iter.cache().leaf_prefixes() = leaf_prefix_;
        iter.cache().size_prefix() 	 = branch_size_prefix_;
    }

    const IteratorBranchNodeEntry& branch_BranchNodeEntry() const {
    	return branch_prefix_;
    }

    IteratorBranchNodeEntry& branch_BranchNodeEntry() {
    	return branch_prefix_;
    }

    const IteratorBranchNodeEntry& leaf_BranchNodeEntry() const {
    	return leaf_prefix_;
    }

    IteratorBranchNodeEntry& leaf_BranchNodeEntry() {
    	return leaf_prefix_;
    }

    const Position& branch_size_prefix() const {
    	return branch_size_prefix_;
    }

    Position& branch_size_prefix() {
    	return branch_size_prefix_;
    }

    const Position& branch_size_prefix_backup() const {
    	return branch_size_prefix_backup_;
    }

    Int idx_backup() const {
    	return idx_backup_;
    }

    MyType& self() {return *T2T<MyType*>(this);}
    const MyType& self() const {return *T2T<const MyType*>(this);}

    template <typename NodeTypes>
    StreamOpResult treeNode(const bt::BranchNode<NodeTypes>* node, WalkDirection direction, Int start)
    {
    	auto& self = this->self();

    	Int index = node->template translateLeafIndexToBranchIndex<LeafPath>(self.leaf_index());

    	using BranchPath = typename bt::BranchNode<NodeTypes>::template BuildBranchPath<LeafPath>;
        return node->template processStream<BranchPath>(FindBranchFn(self), node->is_root(), index, start);
    }

    template <typename NodeTypes>
    StreamOpResult treeNode(const bt::LeafNode<NodeTypes>* node, WalkDirection direction, Int start)
    {
    	auto& self = this->self();
    	return node->template processStream<LeafPath>(FindLeafFn(self), start);
    }


    template <typename Node, typename... Args>
    void processCmd(const Node* node, WalkCmd cmd, Args&&... args){}

    template <typename NodeTypes, typename... Args>
    void processCmd(const bt::BranchNode<NodeTypes>* node, WalkCmd cmd, Args&&... args)
    {
    	auto& self = this->self();

    	Int index = node->template translateLeafIndexToBranchIndex<LeafPath>(self.leaf_index());

    	using BranchPath = typename bt::BranchNode<NodeTypes>::template BuildBranchPath<LeafPath>;

    	return node->template processStream<BranchPath>(ProcessBranchCmdFn(self), cmd, index, std::forward<Args>(args)...);
    }


    template <typename Node, typename... Args>
    void processLeafIteratorBranchNodeEntry(Node* node, IteratorBranchNodeEntry&accum, Args&&... args)
    {
    	detail::LeafAccumWalker<
    		LeafStructList,
    		LeafRangeList,
    		LeafRangeOffsetList,
    		Node::template StreamStartIdx<
    			ListHead<LeafPath>::Value
    		>::Value
    	> w;

    	Node::template StreamDispatcher<
    		ListHead<LeafPath>::Value
    	>
    	::dispatchAll(node->allocator(), w, self(), accum, std::forward<Args>(args)...);
    }


    struct ProcessBranchIteratorBranchNodeEntryWithLeaf
	{
    	template <Int StreamIdx, typename Node, typename Walker, typename Accum, typename... Args>
    	static bool process(Node* node, Walker&& walker, Accum&& accum, Args&&... args)
    	{
        	detail::LeafAccumWalker<
        		LeafStructList,
        		LeafRangeList,
        		LeafRangeOffsetList,
        		Node::template StreamStartIdx<
        			StreamIdx
        		>::Value
        	> w;

        	Node::template StreamDispatcher<
        		StreamIdx
        	>
        	::dispatchAll(node->allocator(), w, walker, accum, std::forward<Args>(args)...);

        	return true;
    	}
    };


    template <typename Node, typename... Args>
    void processBranchIteratorBranchNodeEntryWithLeaf(Node* node, IteratorBranchNodeEntry&accum, Args&&... args)
    {
    	ForEach<0, Streams>::process(ProcessBranchIteratorBranchNodeEntryWithLeaf(), node, self(), accum, std::forward<Args>(args)...);
    }


    template <typename Node, typename... Args>
    void processBranchIteratorBranchNodeEntry(Node* node, Args&&... args)
    {
    	using ItrAccList = memoria::list_tree::MakeValueList<Int, 0, Node::Streams>;

    	detail::IteratorStreamRangesListWalker<
    		//IteratorBranchNodeEntry,
    		ItrAccList
    	>::
    	process(self(), node, branch_BranchNodeEntry(), std::forward<Args>(args)...);
    }

    struct BranchSizePrefix
    {
    	template <Int GroupIdx, Int AllocatorIdx, Int ListIdx, typename StreamObj, typename... Args>
    	void stream(const StreamObj* obj, MyType& walker, Args&&... args)
    	{
    		walker.template branch_size_prefix<ListIdx>(obj, std::forward<Args>(args)...);
    	}
    };


    struct LeafSizePrefix
    {
    	template <Int GroupIdx, Int AllocatorIdx, Int ListIdx, typename StreamObj, typename... Args>
    	void stream(const StreamObj* obj, MyType& walker, Args&&... args)
    	{
    		walker.template leaf_size_prefix<ListIdx>(obj, std::forward<Args>(args)...);
    	}
    };



    template <typename Node, typename... Args>
    void processBranchSizePrefix(Node* node, Args&&... args)
    {
    	node->template processStreamsStart(BranchSizePrefix(), self(), std::forward<Args>(args)...);
    }

    template <typename Node, typename... Args>
    void processLeafSizePrefix(Node* node, Args&&... args)
    {
    	node->template processStreamsStart(LeafSizePrefix(), self(), std::forward<Args>(args)...);
    }

    template <Int StreamIdx, typename StreamType>
    void postProcessBranchStream(const StreamType*, Int, Int) {}

    template <Int StreamIdx, typename StreamType>
    void postProcessLeafStream(const StreamType*, Int, Int) {}
};




}
}

#endif
