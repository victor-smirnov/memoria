
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_SELECT_WALKERS_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_SELECT_WALKERS_HPP

#include <memoria/prototypes/bt/walkers/bt_walker_base.hpp>

namespace memoria {
namespace bt1 	  {



template <typename Types, typename MyType>
class SelectWalkerBase: public WalkerBase<Types, MyType> {

protected:
    typedef SelectWalkerBase<Types, MyType>                                     Base;
    typedef typename Base::Key                                                  Key;

public:
    typedef Int                                                                 ResultType;


	SelectWalkerBase(Int stream, Int block, Key target):
		Base(stream, block, target)
	{
		this->search_type_ = SearchType::GE;
	}

    template <Int StreamIdx, typename StreamObj>
    ResultType stream(const StreamObj* stream, Int start)
    {
    	if (this->current_tree_level_)
    	{
    		return self().template find<StreamIdx>(stream, start);
    	}
    	else
    	{
    		return self().template select<StreamIdx>(stream, start);
    	}
    }
};




template <typename Types, typename MyType>
class SelectForwardWalkerBase: public SelectWalkerBase<Types, MyType> {

protected:
    typedef SelectWalkerBase<Types, MyType>                                     Base;
    typedef typename Base::Key                                                  Key;

public:
    SelectForwardWalkerBase(Int stream, Int block, Key target):
    	Base(stream, block, target)
    {}

    typedef Int                                                                 ResultType;

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

    template <Int StreamIdx, typename Tree>
    ResultType find(const Tree* tree, Int start)
    {
    	auto k = Base::target_ - Base::sum_;

    	auto result = tree->findForward(Base::search_type_, Base::index_, start, k);

    	Base::sum_ += result.prefix();

    	self().template postProcessStream<StreamIdx>(tree, start, result);

    	this->end_ = result.idx() >= tree->size();

    	return result.idx();
    }


    template <Int StreamIdx, typename Tree>
    ResultType select(const Tree* tree, Int start)
    {
        MEMORIA_ASSERT_TRUE(seq != nullptr);

        auto& sum       = Base::sum_;
        auto symbol     = Base::index_ - 1;

        BigInt target   = Base::target_ - sum;
        auto result     = seq->selectFw(start, symbol, target);

        this->end_ 		= !result.is_found();

        if (result.is_found())
        {
        	self().template postProcessStream<StreamIdx>(tree, start, result.idx());

            return result.idx();
        }
        else {
            Int size = seq->size();

            sum  += result.rank();

            self().template postProcessStream<StreamIdx>(tree, start, size);

            return size;
        }
    }


    template <Int StreamIdx, typename StreamType, typename Result>
    void postProcessStream(const StreamType* stream, Int start, const Result& result)
    {
    	PostProcessStreamFw(stream, std::get<StreamIdx>(Base::prefix_), start, result);
    }



    template <Int StreamIdx, typename StreamType>
    void postProcessNonLeafOtherStreams(const StreamType* stream, Int start, Int end)
    {
    	PostProcessStreamFw(stream, std::get<StreamIdx>(Base::prefix_), start, end);
    }

    template <Int StreamIdx, typename StreamType>
    void postProcessLeafOtherStreams(const StreamType* stream)
    {
    	if (this->end_)
    	{
    		PostProcessStreamFw(stream, std::get<StreamIdx>(Base::prefix_), 0, stream->size());
    	}
    }

    MyType& self() {
        return *T2T<MyType*>(this);
    }

    const MyType& self() const {
        return *T2T<const MyType*>(this);
    }
};





template <typename Types, typename MyType>
class SelectBackwardWalkerBase: public SelectWalkerBase<Types, MyType> {

    typedef SelectWalkerBase<Types, MyType>                                     Base;

protected:
    typedef typename Base::Key                                                  Key;

public:
    typedef Int                                                                 ResultType;

    SelectBackwardWalkerBase(Int stream, Int index, Key target):
    	Base(stream, index, target)
    {}

    template <Int StreamIdx, typename Tree>
    ResultType find(const Tree* tree, Int start)
    {
        auto k          = Base::target_ - Base::sum_;
        auto result     = tree->findBackward(Base::search_type_, Base::index_, start, k);
        Base::sum_      += result.prefix();

        self().template postProcessStream<Idx>(tree, start, result);

        this->end_ 		= result.idx() >= 0;

        return result.idx();
    }


    template <Int StreamIdx, typename Array>
    ResultType select(const Array* array, Int start)
    {
    	MEMORIA_ASSERT_TRUE(seq != nullptr);

    	BigInt target   = Base::target_ - Base::sum_;

    	auto& sum       = Base::sum_;
    	auto symbol     = Base::index_ - 1;

    	auto result     = seq->selectBw(start, symbol, target);

    	this->end_ 		= !result.is_found();

    	if (result.is_found())
    	{
    		self().template postProcessStream<StreamIdx>(array, start, start - result.idx());

    		return result.idx();
    	}
    	else {
    		sum += result.rank();

    		self().template postProcessStream<StreamIdx>(array, start, 0);

    		return -1;
    	}
    }


    template <Int StreamIdx, typename StreamType, typename Result>
    void postProcessStream(const StreamType* stream, Int start, const Result& result)
    {
    	PostProcessStreamBw(stream, std::get<StreamIdx>(Base::prefix_), start, result);
    }

    template <Int StreamIdx, typename StreamType>
    void postProcessNonLeafOtherStreams(const StreamType* stream, Int start, Int end)
    {
    	PostProcessStreamBw(stream, std::get<StreamIdx>(Base::prefix_), start, end);
    }

    template <Int StreamIdx, typename StreamType>
    void postProcessLeafOtherStreams(const StreamType* stream)
    {
    	if (this->end_)
    	{
    		PostProcessStreamBw(stream, std::get<StreamIdx>(Base::prefix_), stream->size(), 0);
    	}
    }

    MyType& self() {
        return *T2T<MyType*>(this);
    }

    const MyType& self() const {
        return *T2T<const MyType*>(this);
    }
};




}
}

#endif
