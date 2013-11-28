
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_FIND_WALKERS_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_FIND_WALKERS_HPP

#include <memoria/prototypes/bt/walkers/bt_walker_base.hpp>


namespace memoria {
namespace bt1 	  {


template <typename Types, typename MyType>
class FindWalkerBase: public WalkerBase<Types, MyType> {

protected:
    typedef WalkerBase<Types, MyType>                                       	Base;
    typedef typename Base::Key                                                  Key;

public:
    typedef Int                                                                 ResultType;


	FindWalkerBase(Int stream, Int block, Key target, SearchType search_type):
		Base(stream, block, target)
	{
		Base::search_type() = search_type;
	}


    template <Int StreamIdx, typename StreamObj>
    ResultType stream(const StreamObj* stream, Int start)
    {
    	if (this->index_ || this->current_tree_level_)
    	{
    		return self().template find<StreamIdx>(stream, start);
    	}
    	else
    	{
    		return self().template skip<StreamIdx>(stream, start);
    	}
    }

    MyType& self() {
    	return *T2T<MyType*>(this);
    }

    const MyType& self() const {
    	return *T2T<const MyType*>(this);
    }
};


template <typename Types, typename IteratorPrefixFn, typename MyType>
class FindForwardWalkerBase: public FindWalkerBase<Types, MyType> {

protected:
    typedef FindWalkerBase<Types, MyType>                                       Base;
    typedef typename Base::Key                                                  Key;

public:
    FindForwardWalkerBase(Int stream, Int block, Key target, SearchType search_type):
    	Base(stream, block, target, search_type)
    {}

    typedef Int                                                                 ResultType;

    template <Int StreamIdx, typename Tree>
    ResultType find(const Tree* tree, Int start)
    {
    	auto k = Base::target_ - Base::sum_;

    	Int shift = this->current_tree_level_ == 0;

    	auto result = tree->findForward(Base::search_type_, Base::index_ - shift, start, k);

    	Base::sum_ += result.prefix();

    	IteratorPrefixFn fn;

    	if (this->current_tree_level_ > 0)
    	{
    	    fn.processNonLeafFw(tree, std::get<StreamIdx>(Base::prefix_), start, result.idx(), Base::index_, result.prefix());
    	}
    	else {
    		fn.processLeafFw(tree, std::get<StreamIdx>(Base::prefix_), start, result.idx(), Base::index_, result.prefix());
    	}

    	this->end_ = result.idx() >= tree->size();

    	return result.idx();
    }

    template <Int StreamIdx, typename Array>
    ResultType skip(const Array* array, Int start)
    {
    	auto& sum = Base::sum_;

    	BigInt offset = Base::target_ - sum;

    	if (array != nullptr)
    	{
    		Int size = array->size();

    		IteratorPrefixFn fn;

    		if (start + offset < size)
    		{
    			sum += offset;

    			fn.processLeafFw(array, std::get<StreamIdx>(Base::prefix_), start, start + offset, 0, offset);

    			this->end_ = false;

    			return start + offset;
    		}
    		else {
    			sum += (size - start);

    			fn.processLeafFw(array, std::get<StreamIdx>(Base::prefix_), start, size, 0, size - start);

    			this->end_ = true;

    			return size;
    		}
    	}
    	else {
    		return 0;
    	}
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
class FindForwardWalker: public FindForwardWalkerBase<
									Types,
									IteratorPrefixFn,
									FindForwardWalker<Types, IteratorPrefixFn>> {

	using Base 	= FindForwardWalkerBase<Types, IteratorPrefixFn, FindForwardWalker<Types, IteratorPrefixFn>>;
	using Key 	= typename Base::Key;

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

	using Base 	= FindForwardWalkerBase<Types, IteratorPrefixFn, FindGTForwardWalker<Types, IteratorPrefixFn>>;
	using Key 	= typename Base::Key;

public:
	FindGTForwardWalker(Int stream, Int block, Key target):
		Base(stream, block, target, SearchType::GT)
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

	using Base 	= FindForwardWalkerBase<Types, IteratorPrefixFn, FindGEForwardWalker<Types, IteratorPrefixFn>>;
	using Key 	= typename Base::Key;

public:
	FindGEForwardWalker(Int stream, Int block, Key target):
		Base(stream, block, target, SearchType::GE)
	{}
};

template <typename Types, typename IteratorPrefixFn, typename MyType>
class FindBackwardWalkerBase: public FindWalkerBase<Types, MyType> {

    typedef FindWalkerBase<Types, MyType>                                       Base;

protected:
    typedef typename Base::Key                                                  Key;

public:
    typedef Int                                                                 ResultType;

    FindBackwardWalkerBase(Int stream, Int index, Key target, SearchType search_type):
    	Base(stream, index, target, search_type)
    {}

    template <Int StreamIdx, typename Tree>
    ResultType find(const Tree* tree, Int start)
    {
        auto k          = Base::target_ - Base::sum_;
        Int shift 		= this->current_tree_level_ == 0;

        auto result     = tree->findBackward(Base::search_type_, Base::index_ - shift, start, k);
        Base::sum_      += result.prefix();

        IteratorPrefixFn fn;

        Int idx = result.idx() < 0 ? 0 : result.idx();

        if (this->current_tree_level_ > 0)
        {
        	fn.processNonLeafBw(tree, std::get<StreamIdx>(Base::prefix_), idx, start, Base::index_, result.prefix());
        }
        else {
        	fn.processLeafBw(tree, std::get<StreamIdx>(Base::prefix_), idx, start, Base::index_, result.prefix());
        }

        this->end_ = result.idx() >= 0;

        return result.idx();
    }


    template <Int StreamIdx, typename Array>
    ResultType skip(const Array* array, Int start)
    {
    	BigInt offset = Base::target_ - Base::sum_;

    	auto& sum = Base::sum_;

    	if (array != nullptr)
    	{
    		IteratorPrefixFn fn;

    		if (start - offset >= 0)
    		{
    			sum += offset;

    			fn.processLeafBw(array, std::get<StreamIdx>(Base::prefix_), start - offset, start, 0, offset);

    			this->end_ = false;

    			return start - offset;
    		}
    		else {
    			sum += start;

    			fn.processLeafBw(array, std::get<StreamIdx>(Base::prefix_), 0, start, 0, start);

    			this->end_ = true;

    			return -1;
    		}
    	}
    	else {
    		return 0;
    	}
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

	using Base 	= FindBackwardWalkerBase<Types, IteratorPrefixFn, FindForwardWalker<Types, IteratorPrefixFn>>;
	using Key 	= typename Base::Key;

public:
	FindBackwardWalker(Int stream, Int block, Key target, SearchType search_type = SearchType::GE):
		Base(stream, block, target, search_type)
	{}
};




}
}

#endif
