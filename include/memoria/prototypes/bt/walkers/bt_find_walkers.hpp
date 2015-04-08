
// Copyright Victor Smirnov 2013-2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_FIND_WALKERS_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_FIND_WALKERS_HPP

#include <memoria/prototypes/bt/walkers/bt_walker_base.hpp>


namespace memoria {
namespace bt1     {




template <typename Types, typename MyType>
class FindWalkerBase: public WalkerBase<Types, MyType> {

protected:
    using Base = WalkerBase<Types, MyType>;
    using Key = typename Base::Key;

    using LeafPath 		= typename Types::LeafPath;

public:

    FindWalkerBase(Int stream, Int leaf_index, Key target, SearchType search_type):
        Base(stream, leaf_index, target)
    {
        Base::search_type() = search_type;
    }
};


template <typename Types, typename IteratorPrefixFn, typename MyType>
class FindForwardWalkerBase: public FindWalkerBase<Types, MyType> {

protected:
    using Base = FindWalkerBase<Types, MyType>;
    typedef typename Base::Key                                                  Key;

public:
    FindForwardWalkerBase(Int stream, Int leaf_index, Key target, SearchType search_type):
        Base(stream, leaf_index, target, search_type)
    {}

    template <Int StreamIdx, typename Tree>
    Int find_non_leaf(const Tree* tree, Int index, Int start)
    {
        auto k = Base::target_ - Base::sum_;

        auto result = tree->findForward(Base::search_type_, index, start, k);

        Base::sum_ += result.prefix();

        IteratorPrefixFn fn;

        fn.processNonLeafFw(tree, std::get<StreamIdx>(Base::prefix_), start, result.idx(), index, result.prefix());

        this->end_ = result.idx() >= tree->size();

        self().template postProcessNonLeafStream<StreamIdx>(tree, start, result.idx());

        return result.idx();
    }


    template <Int StreamIdx, typename Tree>
    Int find_leaf(const Tree* tree, Int start)
    {
        auto k = Base::target_ - Base::sum_;

        Int index   = this->leaf_index();

        auto result = tree->findForward(Base::search_type_, index, start, k);

        Base::sum_ += result.prefix();

        IteratorPrefixFn fn;

        fn.processLeafFw(tree, std::get<StreamIdx>(Base::prefix_), start, result.idx(), index + 1, result.prefix());

        this->end_ = result.idx() >= tree->size();

        self().template postProcessLeafStream<StreamIdx>(tree, start, result.idx());

        return result.idx();
    }



    template <Int StreamIdx, typename StreamType>
    void postProcessOtherNonLeafStreams(const StreamType* stream, Int start, Int end)
    {
        IteratorPrefixFn fn;
        fn.processNonLeafFw(stream, std::get<StreamIdx>(Base::prefix_), start, end, 0, end - start);
    }

    template <Int StreamIdx, typename StreamType>
    void postProcessOtherLeafStreams(const StreamType* stream)
    {
        if (this->end_)
        {
            IteratorPrefixFn fn;
            fn.processLeafFw(stream, std::get<StreamIdx>(Base::prefix_), 0, stream->size(), 0, stream->size());
        }
    }

    MyType& self() {return *T2T<MyType*>(this);}
    const MyType& self() const {return *T2T<const MyType*>(this);}
};


template <
    typename Types,
    typename IteratorPrefixFn = EmptyIteratorPrefixFn
>
class FindForwardWalker: public FindForwardWalkerBase<
                                    Types,
                                    IteratorPrefixFn,
                                    FindForwardWalker<Types, IteratorPrefixFn>> {

    using Base  = FindForwardWalkerBase<
                    Types,
                    IteratorPrefixFn,
                    FindForwardWalker<
                        Types,
                        IteratorPrefixFn
                    >
    >;

    using Key   = typename Base::Key;

public:
    FindForwardWalker(Int stream, Int block, Key target, SearchType search_type = SearchType::GE):
        Base(stream, block, target, search_type)
    {}
};


template <
    typename Types,
    typename IteratorPrefixFn = EmptyIteratorPrefixFn
>
class FindGTForwardWalker: public FindForwardWalkerBase<
                                    Types,
                                    IteratorPrefixFn,
                                    FindGTForwardWalker<Types, IteratorPrefixFn>> {

    using Base  = FindForwardWalkerBase<
                    Types,
                    IteratorPrefixFn,
                    FindGTForwardWalker<
                        Types,
                        IteratorPrefixFn
                    >
    >;

    using Key   = typename Base::Key;

public:
    FindGTForwardWalker(Int stream, Int branch_index, Int leaf_index, Key target):
        Base(stream, branch_index, leaf_index, target, SearchType::GT)
    {}
};


template <
    typename Types,
    typename IteratorPrefixFn = EmptyIteratorPrefixFn
>
class FindGEForwardWalker: public FindForwardWalkerBase<
                                    Types,
                                    IteratorPrefixFn,
                                    FindGEForwardWalker<Types, IteratorPrefixFn>> {

    using Base  = FindForwardWalkerBase<
                    Types,
                    IteratorPrefixFn,
                    FindGEForwardWalker<
                        Types,
                        IteratorPrefixFn
                    >
    >;

    using Key   = typename Base::Key;

public:

    static const Int StreamIdx = 0;
    static const Int SubstreamIdx = 0;

    FindGEForwardWalker(Int stream, Int leaf_index, Key target):
        Base(stream, leaf_index, target, SearchType::GE)
    {}
};





template <
    typename Types,
    typename IteratorPrefixFn,
    typename MyType
>
class FindBackwardWalkerBase: public FindWalkerBase<Types, MyType> {
protected:
    using Base = FindWalkerBase<Types, MyType>;

protected:
    typedef typename Base::Key                                                  Key;

public:

    FindBackwardWalkerBase(Int stream, Int leaf_index, Key target, SearchType search_type):
        Base(stream, leaf_index, target, search_type)
    {}

    template <Int StreamIdx, typename Tree>
    Int find_non_leaf(const Tree* tree, Int index, Int start)
    {
        auto k          = Base::target_ - Base::sum_;

        auto result     = tree->findBackward(Base::search_type_, index, start, k);
        Base::sum_      += result.prefix();

        IteratorPrefixFn fn;

        Int idx = result.idx();

        fn.processNonLeafBw(tree, std::get<StreamIdx>(Base::prefix_), idx + 1, start + 1, index, result.prefix());

        this->end_ = result.idx() >= 0;

        self().template postProcessNonLeafStream<StreamIdx>(tree, idx + 1, start + 1);

        return result.idx();
    }


    template <Int StreamIdx, typename Tree>
    Int find_leaf(const Tree* tree, Int start)
    {
        auto k          = Base::target_ - Base::sum_;

        Int index       = this->leaf_index();

        Int start1      = start == tree->size() ? start - 1 : start;

        auto result     = tree->findBackward(Base::search_type_, index, start1, k);
        Base::sum_      += result.prefix();

        IteratorPrefixFn fn;

        Int idx = result.idx() < 0 ? 0 : result.idx();

        fn.processLeafBw(tree, std::get<StreamIdx>(Base::prefix_), idx, start, index + 1, result.prefix());

        this->end_ = result.idx() >= 0;

        self().template postProcessNonLeafStream<StreamIdx>(tree, idx, start);

        return result.idx();
    }




    template <Int StreamIdx, typename StreamType>
    void postProcessOtherNonLeafStreams(const StreamType* stream, Int start, Int end)
    {
        IteratorPrefixFn fn;
        fn.processNonLeafBw(stream, std::get<StreamIdx>(Base::prefix_), end < 0 ? 0 : end, start, 0, start - end);
    }

    template <Int StreamIdx, typename StreamType>
    void postProcessOtherLeafStreams(const StreamType* stream)
    {
        if (this->end_)
        {
            IteratorPrefixFn fn;
            fn.processNonLeafBw(stream, std::get<StreamIdx>(Base::prefix_), 0, stream->size(), 0, stream->size());
        }
    }

    MyType& self() {
        return *T2T<MyType*>(this);
    }

    const MyType& self() const {
        return *T2T<const MyType*>(this);
    }
};


template <
    typename Types,
    typename IteratorPrefixFn = EmptyIteratorPrefixFn
>
class FindBackwardWalker: public FindBackwardWalkerBase<
                                    Types,
                                    IteratorPrefixFn,
                                    FindBackwardWalker<Types, IteratorPrefixFn>> {

    using Base  = FindBackwardWalkerBase<
                    Types,
                    IteratorPrefixFn,
                    FindBackwardWalker<
                        Types,
                        IteratorPrefixFn
                    >
    >;

    using Key   = typename Base::Key;

public:
    FindBackwardWalker(Int stream, Int leaf_index, Key target, SearchType search_type = SearchType::GE):
        Base(stream, leaf_index, target, search_type)
    {}
};






/***********************************************************************/

template <typename Types, typename MyType>
class FindWalkerBase2: public WalkerBase2<Types, MyType> {

protected:
    using Base = WalkerBase2<Types, MyType>;
    using Key = typename Base::Key;

    using LeafPath 		= typename Types::LeafPath;

    BigInt sum_			= 0;

    Key target_;

public:

    FindWalkerBase2(Int leaf_index, Key target, SearchType search_type):
        Base(leaf_index), target_(target)
    {
        Base::search_type() = search_type;
    }

    BigInt sum() const {
    	return sum_;
    }


    Key target() const {
    	return target_;
    }

};


template <typename Types, typename MyType>
class FindForwardWalkerBase2: public FindWalkerBase2<Types, MyType> {

protected:
    using Base = FindWalkerBase2<Types, MyType>;
    typedef typename Base::Key                                                  Key;

public:

    using LeafPath 		 = typename Base::LeafPath;

    FindForwardWalkerBase2(Int leaf_index, Key target, SearchType search_type):
        Base(leaf_index, target, search_type)
    {}


    template <typename NodeTypes>
    StreamOpResult treeNode(const bt::BranchNode<NodeTypes>* node, Int start)
    {
    	auto& self = this->self();

    	Int index = node->template translateLeafIndexToBranchIndex<LeafPath>(this->leaf_index());

    	using BranchPath = typename bt::BranchNode<NodeTypes>::template BuildBranchPath<LeafPath>;
        StreamOpResult result = node->template processStream<BranchPath>(typename Base::FindBranchFn(self), index, start);

        return result;
    }

    template <typename NodeTypes>
    void treeNode(const bt::BranchNode<NodeTypes>* node, Int start, Int end)
    {
    	auto& self = this->self();

        if (this->compute_branch_)
        {
        	self.processBranchIteratorAccumulator(node, start, end);
        	self.processBranchSizePrefix(node, start, end);
        }
    }



    template <typename NodeTypes>
    StreamOpResult treeNode(const bt::LeafNode<NodeTypes>* node, Int start)
    {
    	auto& self = this->self();

    	using Node = bt::LeafNode<NodeTypes>;

    	StreamOpResult result = node->template processStream<LeafPath>(typename Base::FindLeafFn(self), start);

        return result;
    }


    template <typename NodeTypes>
    void treeNode(const bt::LeafNode<NodeTypes>* node, WalkCmd cmd, Int start, Int end)
    {
    	auto& self = this->self();

    	if (this->compute_leaf_)
    	{
    		if (cmd == WalkCmd::FIRST_LEAF)
    		{
    			self.processLeafIteratorAccumulator(node, this->leaf_accumulator(), start, end);
    			self.processLeafIteratorAccumulator(node, this->branch_accumulator());

    			self.processLeafSizePrefix(node);
    		}
    		else {
    			self.processLeafIteratorAccumulator(node, this->leaf_accumulator(), start, end);
    		}
    	}
    }



    template <Int StreamIdx, typename Tree>
    StreamOpResult find_non_leaf(const Tree* tree, Int index, Int start)
    {
        auto k = Base::target_ - Base::sum_;

        auto result = tree->findForward(Base::search_type_, index, start, k);

        auto size = tree->size();

        if (result.idx() < size)
        {
        	Base::sum_ += result.prefix();

        	return StreamOpResult(result.idx(), false);
        }
        else if (size > 0)
        {
        	Base::sum_ += result.prefix() - tree->value(index, size - 1);
        	return StreamOpResult(result.idx() - 1, true);
        }
        else {
        	return StreamOpResult(0, true);
        }
    }


    template <Int StreamIdx, typename Tree>
    StreamOpResult find_leaf(const Tree* tree, Int start)
    {
    	auto k = Base::target_ - Base::sum_;

        Int index   = this->leaf_index();

        auto result = tree->findForward(Base::search_type_, index, start, k);

        Base::sum_ += result.prefix();

        return StreamOpResult(result.idx(), result.idx() >= tree->size());
    }

	template <Int StreamIdx, typename StreamType>
	void branch_size_prefix(const StreamType* stream, Int start, Int end)
	{
		Base::branch_size_prefix()[StreamIdx] += stream->sum(0, start, end);
	}


	template <Int StreamIdx, typename StreamType>
	void leaf_size_prefix(const StreamType* stream)
	{
		auto sum = stream->size();
		auto& prefix = Base::branch_size_prefix();
		prefix[StreamIdx] += sum;

//		Base::branch_size_prefix()[StreamIdx] -= stream->sum(0);
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

        if (DebugCounter) {
        	obj->dump();
        }


		for (Int c = 0; c < To - From; c++)
		{
			item[c] += obj->sum(c + From, start, end);
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
		if (end - start == 1 && start > 0)
		{
			for (Int c = 0; c < Size; c++)
			{
				item[Offset - std::remove_reference<decltype(item)>::type::From + c] += obj->value(c + From, start);
			}
		}
		else {
			for (Int c = 0; c < Size; c++)
			{
				item[Offset - std::remove_reference<decltype(item)>::type::From + c] = obj->sum(c + From, end);
			}
		}
	}

	template <Int Offset, Int From, Int Size, typename StreamObj, typename AccumItem>
	void leaf_iterator_accumulator(const StreamObj* obj, AccumItem& item)
	{
		for (Int c = 0; c < Size; c++)
		{
			item[Offset - std::remove_reference<decltype(item)>::type::From + c] += obj->sum(c + From);
		}
	}
};


template <
    typename Types
>
class FindForwardWalker2: public FindForwardWalkerBase2<Types,FindForwardWalker2<Types>> {

    using Base  = FindForwardWalkerBase2<Types,FindForwardWalker2<Types>>;

    using Key   = typename Base::Key;

public:
    FindForwardWalker2(Int leaf_index, Key target, SearchType search_type = SearchType::GE):
        Base(leaf_index, target, search_type)
    {}
};


template <
    typename Types
>
class FindGTForwardWalker2: public FindForwardWalkerBase2<Types, FindGTForwardWalker2<Types>> {

    using Base  = FindForwardWalkerBase2<Types, FindGTForwardWalker2<Types>>;

    using Key   = typename Base::Key;

public:
    FindGTForwardWalker2(Int leaf_index, Key target):
        Base(leaf_index, target, SearchType::GT)
    {}

    FindGTForwardWalker2(Int stream, Int leaf_index, Key target):
    	Base(leaf_index, target, SearchType::GT)
    {}
};

template <
    typename Types
>
class FindGEForwardWalker2: public FindForwardWalkerBase2<Types, FindGTForwardWalker2<Types>> {

    using Base  = FindForwardWalkerBase2<Types, FindGTForwardWalker2<Types>>;

    using Key   = typename Base::Key;

public:
    FindGEForwardWalker2(Int leaf_index, Key target):
        Base(leaf_index, target, SearchType::GE)
    {}

    FindGEForwardWalker2(Int stream, Int leaf_index, Key target):
    	Base(leaf_index, target, SearchType::GE)
    {}
};




template <typename Types, typename MyType>
class FindBackwardWalkerBase2: public FindWalkerBase2<Types, MyType> {

protected:
    using Base = FindWalkerBase2<Types, MyType>;
    typedef typename Base::Key                                                  Key;

public:
    using StreamOpResult = typename Base::StreamOpResult;

    FindBackwardWalkerBase2(Int leaf_index, Key target, SearchType search_type):
        Base(leaf_index, target, search_type)
    {}

    template <Int StreamIdx, typename Tree>
    StreamOpResult find_non_leaf(const Tree* tree, Int index, Int start)
    {
    	auto k          = Base::target_ - Base::sum_;

    	auto result     = tree->findBackward(Base::search_type_, index, start, k);
    	Base::sum_      += result.prefix();

    	return StreamOpResult(result.idx(), result.idx() < 0);
    }


    template <Int StreamIdx, typename Tree>
    StreamOpResult find_leaf(const Tree* tree, Int start)
    {
    	auto k          = Base::target_ - Base::sum_;

    	Int index       = this->leaf_index();

    	Int start1      = start == tree->size() ? start - 1 : start;

    	auto result     = tree->findBackward(Base::search_type_, index, start1, k);
    	Base::sum_      += result.prefix();

    	return StreamOpResult(result.idx(), result.idx() < 0);
    }

    template <typename NodeTypes>
    Int treeNode(const bt::BranchNode<NodeTypes>* node, BigInt start)
    {
    	return Base::treeNode(node, start);
    }

    template <typename NodeTypes>
    Int treeNode(const bt::LeafNode<NodeTypes>* node, BigInt start)
    {
    	auto& self = this->self();

    	using Node = bt::LeafNode<NodeTypes>;

    	StreamOpResult result = node->template processStream<typename Base::LeafPath>(typename Base::FindLeafFn(self), start);

    	if (this->compute_leaf_)
    	{
    		self.processLeafIteratorAccumulator(node, this->leaf_accumulator(), true, start, std::get<0>(result));

    		if (Base::direction() == WalkDirection::DOWN)
    		{
    			self.processLeafIteratorAccumulator(node, this->branch_accumulator(), false, start, std::get<0>(result));
    			self.processLeafSizePrefix(node, start, std::get<0>(result));
    		}
    	}

        return std::get<0>(result);
    }


    struct LeafSizePrefix
    {
    	template <Int GroupIdx, Int AllocatorIdx, Int ListIdx, typename StreamObj>
    	void stream(const StreamObj* obj, MyType& walker, Int start, Int end)
    	{
    		walker.template leaf_size_prefix<GroupIdx>(obj, start, end);
    	}
    };

    template <typename Node>
    void processLeafSizePrefix(Node* node, Int start, Int end)
    {
    	node->template processStream<IntList<MyType::current_stream()>>(LeafSizePrefix(), this->self(), start, end);
    }



	template <Int StreamIdx, typename StreamType>
	void branch_size_prefix(const StreamType* stream, Int start, Int end)
	{
		Base::branch_size_prefix()[StreamIdx] -= stream->sum(0, end + 1, start + 1);
	}

	template <Int StreamIdx, typename StreamType>
	void leaf_size_prefix(const StreamType* stream, Int start, Int end)
	{
		Base::branch_size_prefix()[StreamIdx] -= stream->sum(0);
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
			item[c] -= obj->sum(c + From, end + 1, start + 1);
		}
	}

	template <
		typename StreamObj,
		typename T,
		template <typename> class AccumItem
	>
	void branch_iterator_accumulator(const StreamObj* obj, AccumItem<T>& item, Int start, Int end){}

	template <Int Offset, Int From, Int Size, typename StreamObj, typename AccumItem>
	void leaf_iterator_accumulator(const StreamObj* obj, AccumItem& item, bool leaf, Int start, Int end)
	{
		if (leaf)
		{
			if (end > 0)
			{
				for (Int c = 0; c < Size; c++)
				{
					item[Offset - std::remove_reference<decltype(item)>::type::From + c] = obj->sum(c + From, 0, end);
				}
			}
			else {
				for (Int c = 0; c < Size; c++)
				{
					item[Offset - std::remove_reference<decltype(item)>::type::From + c] = 0;
				}
			}
		}
		else {
			for (Int c = 0; c < Size; c++)
			{
				item[Offset - std::remove_reference<decltype(item)>::type::From + c] -= obj->sum(c + From);
			}
		}
	}
};


template <
    typename Types
>
class FindBackwardWalker2: public FindBackwardWalkerBase2<
                                    Types,
                                    FindBackwardWalker2<Types>> {

    using Base  = FindBackwardWalkerBase2<
                    Types,
                    FindBackwardWalker2<
                        Types
                    >
    >;

    using Key   = typename Base::Key;

public:
    FindBackwardWalker2(Int leaf_index, Key target, SearchType search_type = SearchType::GE):
        Base(leaf_index, target, search_type)
    {}
};


template <
    typename Types
>
class FindGTBackwardWalker2: public FindBackwardWalkerBase2<
                                    Types,
                                    FindGTBackwardWalker2<Types>> {

    using Base  = FindBackwardWalkerBase2<
                    Types,
                    FindGTBackwardWalker2<
                        Types
                    >
    >;

    using Key   = typename Base::Key;

public:
    FindGTBackwardWalker2(Int leaf_index, Key target):
        Base(leaf_index, target, SearchType::GT)
    {}

    FindGTBackwardWalker2(Int stream, Int leaf_index, Key target):
    	Base(leaf_index, target, SearchType::GT)
    {}
};

template <
    typename Types
>
class FindGEBackwardWalker2: public FindBackwardWalkerBase2<
                                    Types,
                                    FindGTBackwardWalker2<Types>> {

    using Base  = FindBackwardWalkerBase2<
                    Types,
                    FindGTBackwardWalker2<
                        Types
                    >
    >;

    using Key   = typename Base::Key;

public:
    FindGEBackwardWalker2(Int leaf_index, Key target):
        Base(leaf_index, target, SearchType::GE)
    {}

    FindGEBackwardWalker2(Int stream, Int leaf_index, Key target):
    	Base(leaf_index, target, SearchType::GE)
    {}
};




}
}

#endif
