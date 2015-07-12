
// Copyright Victor Smirnov 2013-2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_FIND_WALKERS_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_FIND_WALKERS_HPP

#include <memoria/prototypes/bt/walkers/bt_walker_base.hpp>


namespace memoria {
namespace bt {



/***********************************************************************/

template <typename Types, typename MyType>
class FindWalkerBase: public WalkerBase<Types, MyType> {

protected:
    using Base 			= WalkerBase<Types, MyType>;

    using LeafPath 		= typename Types::LeafPath;

    using TargetType 	= typename Types::template TargetType<LeafPath>;

    using ThisType = FindWalkerBase<Types, MyType>;
    using Iterator = typename Base::Iterator;

    TargetType sum_			= 0;

    TargetType target_;

    SearchType search_type_ = SearchType::GT;

public:

    FindWalkerBase(Int leaf_index, TargetType target, SearchType search_type):
        Base(leaf_index), target_(target), search_type_(search_type)
    {}

    const SearchType& search_type() const {
    	return search_type_;
    }

    TargetType sum() const {
    	return sum_;
    }

    TargetType result() const {
    	return sum_;
    }

    TargetType target() const {
    	return target_;
    }
};


template <typename Types, typename MyType>
class FindForwardWalkerBase: public FindWalkerBase<Types, MyType> {

protected:
    using Base 			= FindWalkerBase<Types, MyType>;
    using TargetType 	= typename Base::TargetType;



public:

    using LeafPath 		 = typename Base::LeafPath;

    FindForwardWalkerBase(Int leaf_index, TargetType target, SearchType search_type):
        Base(leaf_index, target, search_type)
    {}

    template <typename... Args>
    auto treeNode(Args&&... args) ->
    decltype(std::declval<Base>().treeNode(std::declval<Args>()...))
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
    			self.processLeafIteratorAccumulator(node, this->leaf_accumulator(), start, end);
    		}
    		else if (cmd == WalkCmd::FIRST_LEAF)
    		{
    			// FIXME: is this call necessary here?
    			self.processLeafIteratorAccumulator(node, this->leaf_accumulator(), start, end);
    			self.processLeafIteratorAccumulator(node, this->branch_accumulator());

    			self.processLeafSizePrefix(node);
    		}
    		else if (cmd == WalkCmd::LAST_LEAF) {
    			self.processLeafIteratorAccumulator(node, this->leaf_accumulator(), start, end);
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
        	auto k = Base::target_ - Base::sum_;

        	auto result = tree->findForward(Base::search_type_, index, start, k);

        	Base::sum_ += result.prefix();

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
    		Base::sum_ -= tree->value(index, end);
    	}
    }


    template <Int StreamIdx, typename Tree>
    StreamOpResult find_leaf(const Tree* tree, Int start)
    {
    	if (tree != nullptr)
    	{
    		if (start < tree->size())
    		{
    			auto k = Base::target_ - Base::sum_;

    			Int index   = this->leaf_index();

    			auto result = tree->findForward(Base::search_type_, index, start, k);

    			Base::sum_ += result.prefix();

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

	template <Int StreamIdx, typename StreamType>
	void branch_size_prefix(const StreamType* stream, Int start, Int end)
	{
        auto sum = stream->sum(0, start, end);

		Base::branch_size_prefix()[StreamIdx] += sum;
	}


	template <Int StreamIdx, typename StreamType>
	void leaf_size_prefix(const StreamType* stream)
	{
        auto size = stream->size();

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
		static_assert(To <= StreamObj::Indexes, "Invalid BTree structure");

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
			const Int Idx = Offset - std::remove_reference<decltype(item)>::type::From;

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
		const Int Idx = Offset - std::remove_reference<decltype(item)>::type::From;

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
class FindForwardWalker: public FindForwardWalkerBase<Types,FindForwardWalker<Types>> {

    using Base  = FindForwardWalkerBase<Types,FindForwardWalker<Types>>;
protected:
    using TargetType 	= typename Base::TargetType;

public:
    FindForwardWalker(Int leaf_index, TargetType target, SearchType search_type = SearchType::GE):
        Base(leaf_index, target, search_type)
    {}
};


template <
    typename Types
>
class FindGTForwardWalker: public FindForwardWalker<Types> {

    using Base  		= FindForwardWalker<Types>;
    using TargetType 	= typename Base::TargetType;

public:
    FindGTForwardWalker(Int leaf_index, TargetType target):
        Base(leaf_index, target, SearchType::GT)
    {}

    FindGTForwardWalker(Int stream, Int leaf_index, TargetType target):
    	Base(leaf_index, target, SearchType::GT)
    {}
};

template <
    typename Types
>
class FindGEForwardWalker: public FindForwardWalkerBase<Types, FindGTForwardWalker<Types>> {

    using Base  		= FindForwardWalkerBase<Types, FindGTForwardWalker<Types>>;
    using TargetType 	= typename Base::TargetType;

public:
    FindGEForwardWalker(Int leaf_index, TargetType target):
        Base(leaf_index, target, SearchType::GE)
    {}

    FindGEForwardWalker(Int stream, Int leaf_index, TargetType target):
    	Base(leaf_index, target, SearchType::GE)
    {}
};











template <typename Types, typename MyType>
class FindBackwardWalkerBase: public FindWalkerBase<Types, MyType> {

protected:
    using Base = FindWalkerBase<Types, MyType>;

    using TargetType 	= typename Base::TargetType;
    using LeafPath 		= typename Base::LeafPath;

public:

    FindBackwardWalkerBase(Int leaf_index, TargetType target, SearchType search_type):
        Base(leaf_index, target, search_type)
    {}

    template <Int StreamIdx, typename Tree>
    StreamOpResult find_non_leaf(const Tree* tree, bool root, Int index, Int start)
    {
    	if (start > tree->size()) start = tree->size() - 1;

    	if (start >= 0)
    	{
    		auto k          = Base::target_ - Base::sum_;

    		auto result     = tree->findBackward(Base::search_type_, index, start, k);
    		Base::sum_      += result.prefix();

    		Int idx = result.idx();

    		return StreamOpResult(idx, start, idx < 0);
    	}
    	else {
    		return StreamOpResult(start, start, true, true);
    	}
    }


    template <Int StreamIdx, typename Tree>
    StreamOpResult find_leaf(const Tree* tree, Int start)
    {
    	if (start > tree->size()) start = tree->size();

    	if (start >= 0)
    	{
    		auto k          = Base::target_ - Base::sum_;

    		Int index       = this->leaf_index();

    		Int start1      = start == tree->size() ? start - 1 : start;

    		auto result     = tree->findBackward(Base::search_type_, index, start1, k);
    		Base::sum_      += result.prefix();

    		Int idx = result.idx();

    		return StreamOpResult(idx, start, idx < 0);
    	}
    	else {
    		return StreamOpResult(start, start, true, true);
    	}
    }

    template <typename... Args>
    auto treeNode(Args&&... args) ->
    decltype(std::declval<Base>().treeNode(std::declval<Args>()...))
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

    	if (cmd == WalkCmd::THE_ONLY_LEAF)
    	{
    		self.processLeafIteratorAccumulator(node, this->leaf_accumulator(), start, end);
    	}
    	else if (cmd == WalkCmd::FIRST_LEAF)
    	{
    		self.processLeafIteratorAccumulator(node, this->leaf_accumulator(), true);
    	}
    	else if (cmd == WalkCmd::LAST_LEAF)  {
    		self.processLeafIteratorAccumulator(node, this->leaf_accumulator(), start, end);
    		self.processLeafIteratorAccumulator(node, this->branch_accumulator(), false);
    		self.processLeafSizePrefix(node);
    	}
    	else {
    		// ?
    	}
    }

    template <Int StreamIdx, typename Tree>
    void process_branch_cmd(const Tree* tree, WalkCmd cmd, Int index, Int start, Int end)
    {
    	if (cmd == WalkCmd::FIX_TARGET)
    	{
    		Base::sum_ -= tree->value(index, end + 1);
    	}
    }


	template <Int StreamIdx, typename StreamType>
	void branch_size_prefix(const StreamType* obj, Int start, Int end)
	{
		Int s = start > (obj->size() - 1) ? obj->size() - 1 : start;

		obj->_sub(0, end + 1, s + 1, Base::branch_size_prefix()[StreamIdx]);
	}


	template <Int StreamIdx, typename StreamType>
	void leaf_size_prefix(const StreamType* obj)
	{
		Base::branch_size_prefix()[StreamIdx] -= obj->size();
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
		static_assert(To <= StreamObj::Indexes, "Invalid BTree structure");

		Int s = start > (obj->size() - 1) ? obj->size() - 1 : start;

		for (Int c = 0; c < To - From; c++)
		{
			obj->_sub(c + From, end + 1, s + 1, item[c]);
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
		const Int Idx = Offset - std::remove_reference<decltype(item)>::type::From;

		if (start >= obj->size()) start = obj->size() - 1;

		for (Int c = 0; c < Size; c++)
		{
			item[Idx + c] = 0;
			obj->_add(c + From, end, item[Idx + c]);
		}
	}

	template <Int Offset, Int From, Int Size, typename StreamObj, typename AccumItem>
	void leaf_iterator_accumulator(const StreamObj* obj, AccumItem& item, bool leaf)
	{
		const Int Idx = Offset - std::remove_reference<decltype(item)>::type::From;

		if (leaf)
		{
			for (Int c = 0; c < Size; c++)
			{
				item[Idx + c] = 0;
			}
		}
		else {
			for (Int c = 0; c < Size; c++)
			{
				obj->_sub(c + From, item[Idx + c]);
			}
		}
	}
};


template <
    typename Types
>
class FindBackwardWalker: public FindBackwardWalkerBase<Types, FindBackwardWalker<Types>> {

    using Base  = FindBackwardWalkerBase<Types, FindBackwardWalker<Types>>;

    using TargetType 	= typename Base::TargetType;

public:
    FindBackwardWalker(Int leaf_index, TargetType target, SearchType search_type = SearchType::GE):
        Base(leaf_index, target, search_type)
    {}
};


template <
    typename Types
>
class FindGTBackwardWalker: public FindBackwardWalkerBase<Types, FindGTBackwardWalker<Types>> {

    using Base  = FindBackwardWalkerBase<Types, FindGTBackwardWalker<Types>>;

    using TargetType 	= typename Base::TargetType;

public:
    FindGTBackwardWalker(Int leaf_index, TargetType target):
        Base(leaf_index, target, SearchType::GT)
    {}

    FindGTBackwardWalker(Int stream, Int leaf_index, TargetType target):
    	Base(leaf_index, target, SearchType::GT)
    {}
};

template <
    typename Types
>
class FindGEBackwardWalker: public FindBackwardWalkerBase<Types, FindGTBackwardWalker<Types>> {

    using Base  = FindBackwardWalkerBase<Types, FindGTBackwardWalker<Types>>;

    using TargetType = typename Base::TargetType;

public:
    FindGEBackwardWalker(Int leaf_index, TargetType target):
        Base(leaf_index, target, SearchType::GE)
    {}

    FindGEBackwardWalker(Int stream, Int leaf_index, TargetType target):
    	Base(leaf_index, target, SearchType::GE)
    {}
};




}
}

#endif
