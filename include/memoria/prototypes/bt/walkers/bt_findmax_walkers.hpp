
// Copyright Victor Smirnov 2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_FINDMAX_WALKERS_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_FINDMAX_WALKERS_HPP

#include <memoria/prototypes/bt/walkers/bt_walker_base.hpp>
#include <memoria/prototypes/bt/walkers/bt_find_walkers.hpp>

namespace memoria {
namespace bt {

/***********************************************************************/

template <typename Types, typename MyType>
class FindMaxWalkerBaseBase: public WalkerBase<Types, MyType> {

public:
    using Base 			= WalkerBase<Types, MyType>;

    using LeafPath 		= typename Types::LeafPath;

    using TargetType 	= typename Types::template TargetType<LeafPath>;

    using ThisType = FindMaxWalkerBaseBase<Types, MyType>;
    using Iterator = typename Base::Iterator;

protected:

    TargetType target_;

    SearchType search_type_ = SearchType::GT;

public:

    FindMaxWalkerBaseBase(Int leaf_index, TargetType target, SearchType search_type):
        Base(leaf_index), target_(target), search_type_(search_type)
    {}

    const SearchType& search_type() const {
    	return search_type_;
    }

    TargetType target() const {
    	return target_;
    }
};


template <typename Types, typename MyType>
class FindMaxWalkerBase: public FindMaxWalkerBaseBase<Types, MyType> {
	using Base 			= FindMaxWalkerBaseBase<Types, MyType>;

public:

    using TargetType 	= typename Base::TargetType;
    using Position 		= typename Base::Position;
    using LeafPath 		 = typename Base::LeafPath;

    FindMaxWalkerBase(Int leaf_index, TargetType target, SearchType search_type):
        Base(leaf_index, target, search_type)
    {}

    template <typename... Args>
    auto treeNode(Args&&... args) -> decltype(Base::treeNode(std::forward<Args>(args)...))
    {
    	return Base::treeNode(std::forward<Args>(args)...);
    }

    template <typename NodeTypes>
    void treeNode(const bt::BranchNode<NodeTypes>* node, WalkCmd cmd, Int start, Int end)
    {
    	auto& self = this->self();

        if (cmd == WalkCmd::FIX_TARGET)
        {
        	self.processCmd(node, cmd, start, end);
        	self.processBranchSizePrefix(node, start, end, FixTargetTag());
        }
        else if (cmd == WalkCmd::PREFIXES)
        {
        	self.processBranchIteratorAccumulator(node, start, end);
        	self.processBranchSizePrefix(node, start, end);
        }
    }


    template <typename NodeTypes>
    void treeNode(const bt::LeafNode<NodeTypes>* node, WalkCmd cmd, Int start, Int end)
    {
    	auto& self = this->self();

    	if (this->compute_leaf_)
    	{
    		if (cmd == WalkCmd::THE_ONLY_LEAF)
    		{
//    			self.processLeafIteratorAccumulator(node, this->leaf_accumulator(), start, end);
    		}
    		else if (cmd == WalkCmd::FIRST_LEAF)
    		{
    			// FIXME: is this call necessary here?
//    			self.processLeafIteratorAccumulator(node, this->leaf_accumulator(), start, end);
    			self.processBranchIteratorAccumulatorWithLeaf(node, this->branch_accumulator());

    			self.processLeafSizePrefix(node);
    		}
    		else if (cmd == WalkCmd::LAST_LEAF) {
//    			self.processLeafIteratorAccumulator(node, this->leaf_accumulator(), start, end);
    		}
    		else {
    			// throw exception ?
    		}
    	}
    }



    template <Int StreamIdx, typename Tree>
    StreamOpResult find_non_leaf(const Tree* tree, bool root, Int index, Int start)
    {
    	auto size = tree->size();

        if (start < size)
        {
        	MEMORIA_ASSERT(start, ==, 0);

        	auto result = tree->findForward(Base::search_type_, index, Base::target_);

        	return StreamOpResult(result.idx(), start, result.idx() >= size, false);
        }
        else {
        	return StreamOpResult(size, start, true, true);
        }
    }


    template <Int StreamIdx, typename Tree>
    void process_branch_cmd(const Tree* tree, WalkCmd cmd, Int index, Int start, Int end)
    {
    	if (cmd == WalkCmd::FIX_TARGET)
    	{
//    		Base::sum_ -= tree->value(index, end);
    	}
    }


    template <Int StreamIdx, typename Tree>
    StreamOpResult find_leaf(const Tree* tree, Int start)
    {
    	if (tree != nullptr)
    	{
    		if (start < tree->size())
    		{
    			MEMORIA_ASSERT(start, ==, 0);

    			Int index   = this->leaf_index();

    			auto result = tree->findForward(Base::search_type_, index, Base::target_);

    			return StreamOpResult(result.idx(), start, result.idx() >= tree->size());
    		}
    		else {
    			return StreamOpResult(start, start, true, true);
    		}
    	}
    	else {
    		return StreamOpResult(0, 0, true, true);
    	}
    }

    auto& branch_size_prefix() {
    	return Base::branch_size_prefix();
    }

    const auto& branch_size_prefix() const {
    	return Base::branch_size_prefix();
    }


	template <Int StreamIdx, typename StreamType>
	void branch_size_prefix(const StreamType* stream, Int start, Int end)
	{
        auto sum = stream->sum(0, start, end);

		Base::branch_size_prefix()[StreamIdx] += sum;
	}


	template <Int StreamIdx, typename StreamType>
	void branch_size_prefix(const StreamType* stream, Int start, Int end, FixTargetTag)
	{
//        auto sum = stream->sum(0, start, end);
//
//        if (DebugCounter) {
//        	stream->dump();
//        }
//
//		Base::branch_size_prefix()[StreamIdx] += sum;
	}


	template <Int StreamIdx, typename StreamType>
	void leaf_size_prefix(const StreamType* stream)
	{
        auto size = stream->size();

        if (DebugCounter) {
        	stream->dump();
        }

		Base::branch_size_prefix()[StreamIdx] += size;
	}


	template <
		typename StreamObj,
		typename T,
		Int From,
		Int To,
		template <typename, Int, Int> class IterAccumItem
	>
	void branch_iterator_accumulator(const StreamObj* obj, IterAccumItem<T, From, To>& item, Int start, Int end)
	{
		static_assert(To <= StructSizeProvider<StreamObj>::Value, "Invalid BTree structure");

		for (Int c = 0; c < To - From; c++)
		{
			obj->_add(c + From, start, end, item[c]);
		}
	}

	template <
		typename StreamObj,
		typename T,
		template <typename> class AccumItem
	>
	void branch_iterator_accumulator(const StreamObj* obj, AccumItem<T>& item, Int start, Int end){}

	template <Int Offset, Int From, Int Size, typename StreamObj, typename AccumItem>
	void leaf_iterator_accumulator(const StreamObj* obj, AccumItem& item, Int start, Int end)
	{
		if (obj != nullptr)
		{
			const Int Idx = Offset - AccumItem::From;

			if (end - start == 1 && start > 0)
			{
				for (Int c = 0; c < Size; c++)
				{
					item[Idx + c] += obj->value(c + From, start);
				}
			}
			else {
				for (Int c = 0; c < Size; c++)
				{
					item[Idx + c] = 0;
					obj->_add(c + From, end, item[Idx + c]);
				}
			}
		}
	}

	template <Int Offset, Int From, Int Size, typename StreamObj, typename AccumItem>
	void leaf_iterator_accumulator(const StreamObj* obj, AccumItem& item)
	{
		const Int Idx = Offset - AccumItem::From;

		if (obj != nullptr) {
			for (Int c = 0; c < Size; c++)
			{
				obj->_add(c + From, item[Idx + c]);
			}
		}
	}
};


template <
    typename Types
>
class FindMaxWalker: public FindMaxWalkerBase<Types,FindMaxWalker<Types>> {

    using Base  = FindMaxWalkerBase<Types,FindMaxWalker<Types>>;
protected:
    using TargetType 	= typename Base::TargetType;

public:
    FindMaxWalker(Int leaf_index, TargetType target, SearchType search_type = SearchType::GE):
        Base(leaf_index, target, search_type)
    {}
};


template <
    typename Types
>
class FindMaxGTWalker: public FindMaxWalker<Types> {

    using Base  		= FindMaxWalker<Types>;
    using TargetType 	= typename Base::TargetType;

public:
    FindMaxGTWalker(Int leaf_index, TargetType target):
        Base(leaf_index, target, SearchType::GT)
    {}

    FindMaxGTWalker(Int stream, Int leaf_index, TargetType target):
    	Base(leaf_index, target, SearchType::GT)
    {}
};

template <
    typename Types
>
class FindMaxGEWalker: public FindMaxWalkerBase<Types, FindMaxGTWalker<Types>> {

    using Base  		= FindMaxWalkerBase<Types, FindMaxGTWalker<Types>>;
    using TargetType 	= typename Base::TargetType;

public:
    FindMaxGEWalker(Int leaf_index, TargetType target):
        Base(leaf_index, target, SearchType::GE)
    {}

    FindMaxGEWalker(Int stream, Int leaf_index, TargetType target):
    	Base(leaf_index, target, SearchType::GE)
    {}
};


}
}

#endif
