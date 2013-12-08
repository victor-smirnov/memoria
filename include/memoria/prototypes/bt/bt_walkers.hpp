
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_WALKERS_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_WALKERS_HPP

#include <memoria/core/tools/static_array.hpp>
#include <memoria/core/container/iterator.hpp>

#include <memoria/prototypes/bt/nodes/branch_node.hpp>
#include <memoria/prototypes/bt/nodes/leaf_node.hpp>

#include <memoria/core/packed/map/packed_fse_map.hpp>
#include <memoria/core/packed/map/packed_fse_mark_map.hpp>

#include <memoria/prototypes/bt/walkers/bt_misc_walkers.hpp>

#include <ostream>
#include <functional>


namespace memoria       {
namespace bt 			{


template <typename Types, typename MyType>
class FindWalkerBase {
protected:
    typedef typename Types::Position                                            Position;
    typedef BigInt                                                 				Key;

    typedef Iter<typename Types::IterTypes>                                     Iterator;

    static const Int Streams                                                    = Types::Streams;

    SearchType search_type_ = SearchType::GT;

    BigInt sum_         = 0;
    BigInt target_      = 0;

    WalkDirection direction_;

    Int stream_;
    Int index_;

public:

    typedef Int                                                                 ReturnType;

    FindWalkerBase(Int stream, Int index, BigInt target):
        target_(target),
        stream_(stream),
        index_(index)
    {}

    const WalkDirection& direction() const {
        return direction_;
    }

    WalkDirection& direction() {
        return direction_;
    }



    void empty(Iterator& iter)
    {
        iter.idx()  = 0;
    }

    BigInt sum() const {
        return sum_;
    }

    Int stream() const {
        return stream_;
    }

    Int index() const
    {
    	return index_;
    }

    const SearchType& search_type() const {
        return search_type_;
    }

    SearchType& search_type() {
        return search_type_;
    }

    void prepare(Iterator& iter) {}

    BigInt finish(Iterator& iter, Int idx)
    {
        iter.idx() = idx;
        return sum_;
    }


    MyType& self()
    {
        return *T2T<MyType*>(this);
    }

    const MyType& self() const
    {
        return *T2T<const MyType*>(this);
    }

    template <Int StreamIdx>
    void postProcessStreamPrefix(BigInt) {}

    template <Int StreamIdx, typename StreamType, typename Result>
    void postProcessStream(const StreamType*, Int, const Result& result) {}

    template <typename Node>
    void postProcessNode(const Node*, Int, Int) {}

    template <typename Node>
    ReturnType treeNode(const Node* node, BigInt start)
    {
        Int idx = node->find(stream_, self(), start);

        self().postProcessNode(node, start, idx);

        return idx;
    }
};



template <typename Types, typename MyType>
class FindMinWalkerBase {
protected:
    typedef typename Types::Position                                            Position;
    typedef typename Types::Key                                                 Key;

    typedef Iter<typename Types::IterTypes>                                     Iterator;

    static const Int Streams                                                    = Types::Streams;

    SearchType  search_type_ = SearchType::GT;

    BigInt      sum_[Streams];
    BigInt      target_[Streams];

    WalkDirection direction_;

    UBigInt     streams_;
    Int         indexes_[Streams];

    Int         search_results_[Streams];

public:

    typedef Int                                                                 ReturnType;
    typedef Int                                                                 ResultType;

    FindMinWalkerBase(UBigInt streams):
        streams_(streams)
    {
        for (auto& i: sum_)     i = 0;
        for (auto& i: target_)  i = 0;
        for (auto& i: indexes_) i = 0;
    }

    const WalkDirection& direction() const {
        return direction_;
    }

    WalkDirection& direction() {
        return direction_;
    }

    BigInt sum(Int stream) const {
        return sum_[stream];
    }

    const BigInt& target(Int stream) const {
        return target_[stream];
    }

    BigInt& target(Int stream) {
        return target_[stream];
    }

    const Int& indexes(Int stream) const {
        return indexes_[stream];
    }

    Int& indexes(Int stream) {
        return indexes_[stream];
    }

    const SearchType& search_type() const {
        return search_type_;
    }

    SearchType& search_type() {
        return search_type_;
    }

    BigInt finish(Iterator& iter, Int idx)
    {
        iter.key_idx() = idx;

        return sum_;
    }

    void empty(Iterator& iter)
    {
        iter.key_idx()  = 0;
    }

    void prepare(Iterator& iter) {}


    MyType& self()
    {
        return *T2T<MyType*>(this);
    }

    const MyType& self() const
    {
        return *T2T<const MyType*>(this);
    }

    template <Int StreamIdx>
    void postProcessStreamPrefix(BigInt) {}

    template <Int StreamIdx, typename StreamType, typename SearchResult>
    void postProcessStream(const StreamType*, Int, const SearchResult&) {}

    template <typename Node>
    void postProcessNode(const Node*, Int, Int) {}

    bool isEnabled(Int stream) const
    {
        return streams_ & (1ull << stream);
    }
};





template <typename Types, typename MyType>
class FindForwardWalkerBase: public FindWalkerBase<Types, MyType> {

protected:
    typedef FindWalkerBase<Types, MyType>                                       Base;
    typedef typename Base::Key                                                  Key;

public:
    FindForwardWalkerBase(Int stream, Int block, Key target): Base(stream, block, target)
    {}

    typedef Int                                                                 ResultType;

    template <Int Idx, typename TreeTypes>
    ResultType stream(const PkdFTree<TreeTypes>* tree, Int start)
    {
    	return self().template tree<Idx>(tree, start);
    }

    template <Int Idx, typename TreeTypes>
    ResultType stream(const PackedFSEMap<TreeTypes>* tree, Int start)
    {
    	return self().template tree<Idx>(tree, start);
    }

    template <Int Idx, typename TreeTypes>
    ResultType stream(const PackedFSEMarkableMap<TreeTypes>* tree, Int start)
    {
    	return self().template tree<Idx>(tree, start);
    }


    template <Int Idx, typename TreeTypes>
    ResultType stream(const PkdVTree<TreeTypes>* tree, Int start)
    {
    	return self().template tree<Idx>(tree, start);
    }

    template <Int Idx, typename StreamTypes>
    ResultType stream(const PackedFSEArray<StreamTypes>* array, Int start)
    {
    	return self().template array<Idx>(array, start);
    }

    template <Int Idx, typename StreamTypes>
    ResultType stream(const PkdFSSeq<StreamTypes>* array, Int start)
    {
    	return self().template array<Idx>(array, start);
    }



    template <Int Idx, typename Tree>
    ResultType tree(const Tree* tree, Int start)
    {
    	auto k      = Base::target_ - Base::sum_;

    	auto result = tree->findForward(Base::search_type_, Base::index_, start, k);

    	Base::sum_ += result.prefix();

    	self().template postProcessStream<Idx>(tree, start, result);

    	return result.idx();
    }

    template <Int Idx, typename Array>
    ResultType array(const Array* array, Int start)
    {
    	auto& sum = Base::sum_;

    	BigInt offset = Base::target_ - sum;

    	Int size = array != nullptr? array->size() : 0;

    	if (start + offset < size)
    	{
    		sum += offset;

    		return start + offset;
    	}
    	else {
    		sum += (size - start);

    		return size;
    	}
    }


    MyType& self() {
        return *T2T<MyType*>(this);
    }

    const MyType& self() const {
        return *T2T<const MyType*>(this);
    }
};


template <typename Types>
class SkipForwardWalker: public FindForwardWalkerBase<Types, SkipForwardWalker<Types>> {
    typedef FindForwardWalkerBase<Types, SkipForwardWalker<Types>>              Base;
    typedef typename Base::Key                                                  Key;
public:
    SkipForwardWalker(Int stream, Int index, Key target): Base(stream, index, target)
    {}

    SkipForwardWalker(Int stream, Int index, Int index1, Key target): Base(stream, index, target)
    {}
};











template <typename Types, typename MyType>
class NextLeafWalkerBase: public FindForwardWalkerBase<Types, MyType> {
protected:
    typedef FindForwardWalkerBase<Types, MyType>                                Base;
    typedef typename Base::Key                                                  Key;
    typedef typename Base::Position                                             Position;
    typedef typename Base::Iterator                                             Iterator;

public:

    NextLeafWalkerBase(Int stream, Int index): Base(stream, index, 0)
    {}

    void finish(Iterator& iter, bool end)
    {

    }
};



template <typename Types>
class NextLeafWalker: public NextLeafWalkerBase<Types, NextLeafWalker<Types> > {

    typedef NextLeafWalkerBase<Types, NextLeafWalker<Types>>                    Base;
    typedef typename Base::Key                                                  Key;
    typedef typename Base::Position                                             Position;
    typedef typename Base::Iterator                                             Iterator;

public:

    NextLeafWalker(Int stream, Int block): Base(stream, block)
    {}
};






template <typename Types, typename MyType>
class FindMinForwardWalkerBase: public FindMinWalkerBase<Types, MyType> {

    typedef FindMinWalkerBase<Types, MyType>                                    Base;
    typedef typename Base::Key                                                  Key;
    typedef typename Base::Position                                             Position;
    typedef typename Base::Iterator                                             Iterator;

protected:
    static const Int Streams                                                    = Types::Streams;

public:

    typedef Int ReturnType;
    typedef Int ResultType;


    FindMinForwardWalkerBase(UBigInt streams):
        Base(streams)
    {}

    template <typename NTypes>
    ResultType treeNode(const BranchNode<NTypes>* node, Int start)
    {
        for (auto& i: Base::search_results_) i = -1;

        node->processNotEmpty(Base::streams_, self(), start);

        Int min     = 1<<31;
        Int min_idx = Streams;

        for (Int c = 0; c < Streams; c++)
        {
            Int index = Base::search_results_[c];
            if (Base::isEnabled(c) && index < min)
            {
                min     = index;
                min_idx = c;
            }
        }

        MEMORIA_ASSERT_TRUE(min_idx < Streams);
        MEMORIA_ASSERT_TRUE(min_idx >= 0);

        self().postProcessNode(node, start, min_idx);

        return min_idx;
    }


    template <Int Idx, typename Tree>
    void stream(const Tree* tree, Int start)
    {
        auto k      = Base::target_[Idx] - Base::sum_[Idx];
        auto result = tree->findForward(Base::search_type_, Base::indexes_[Idx], start, k);

        Base::sum_[Idx]             += result.prefix();
        Base::search_results_[Idx]  = result.idx();
    }


    template <typename NodeTypes>
    ReturnType treeNode(const LeafNode<NodeTypes>* node, Int start)
    {
        return 0;
    }

    void finish(Iterator& iter, bool end)
    {}


    MyType& self() {
        return *T2T<MyType*>(this);
    }

    const MyType& self() const {
        return *T2T<const MyType*>(this);
    }
};



template <typename Types>
class NextLeafMultistreamWalker: public FindMinForwardWalkerBase<Types, NextLeafMultistreamWalker<Types> > {
    typedef FindMinForwardWalkerBase<Types, NextLeafMultistreamWalker<Types>>   Base;
public:
    NextLeafMultistreamWalker(UBigInt streams): Base(streams) {}
};






template <typename Types, typename MyType>
class FindBackwardWalkerBase: public FindWalkerBase<Types, MyType> {

    typedef FindWalkerBase<Types, MyType>                                       Base;

protected:
    typedef typename Base::Key                                                  Key;

public:
    typedef Int                                                                 ResultType;

    FindBackwardWalkerBase(Int stream, Int index, Key target): Base(stream, index, target)
    {}

    template <Int Idx, typename TreeTypes>
    ResultType stream(const PkdFTree<TreeTypes>* tree, Int start)
    {
    	return self().template tree<Idx>(tree, start);
    }

    template <Int Idx, typename TreeTypes>
    ResultType stream(const PackedFSEMap<TreeTypes>* tree, Int start)
    {
    	return self().template tree<Idx>(tree, start);
    }


    template <Int Idx, typename TreeTypes>
    ResultType stream(const PkdVTree<TreeTypes>* tree, Int start)
    {
    	return self().template tree<Idx>(tree, start);
    }


    template <Int Idx, typename TreeTypes>
    ResultType stream(const PackedFSEArray<TreeTypes>* array, Int start)
    {
    	return self().template array<Idx>(array, start);
    }

    template <Int Idx, typename TreeTypes>
    ResultType stream(const PkdFSSeq<TreeTypes>* array, Int start)
    {
    	return self().template array<Idx>(array, start);
    }


    template <Int Idx, typename Tree>
    ResultType tree(const Tree* tree, Int start)
    {
        auto k          = Base::target_ - Base::sum_;
        auto result     = tree->findBackward(Base::search_type_, Base::index_, start, k);
        Base::sum_      += result.prefix();

        self().template postProcessStreamPrefix<Idx>(result.prefix());
        self().template postProcessStream<Idx>(tree, start, result);

        return result.idx();
    }


    template <Int Idx, typename Array>
    ResultType array(const Array* array, Int start)
    {
    	BigInt offset = Base::target_ - Base::sum_;

    	auto& sum = Base::sum_;

    	if (start - offset >= 0)
    	{
    		sum += offset;
    		return start - offset;
    	}
    	else {
    		sum += start;
    		return -1;
    	}
    }

    MyType& self() {
        return *T2T<MyType*>(this);
    }

    const MyType& self() const {
        return *T2T<const MyType*>(this);
    }
};


template <typename Types>
class SkipBackwardWalker: public FindBackwardWalkerBase<Types, SkipBackwardWalker<Types>> {
    typedef FindBackwardWalkerBase<Types, SkipBackwardWalker<Types>>            Base;
    typedef typename Base::Key                                                  Key;
public:
    SkipBackwardWalker(Int stream, Int index, Key target): Base(stream, index, target)
    {
        Base::search_type_ = SearchType::GE;
    }
};






template <typename Types, typename MyType>
class PrevLeafWalkerBase: public FindBackwardWalkerBase<Types, MyType> {

protected:
    typedef FindBackwardWalkerBase<Types, MyType>                               Base;
    typedef typename Base::Key                                                  Key;
    typedef typename Base::Position                                             Position;
    typedef typename Base::Iterator                                             Iterator;

public:
    PrevLeafWalkerBase(Int stream, Int index): Base(stream, index, 1)
    {}

    void finish(Iterator& iter, bool start)
    {}
};


template <typename Types>
class PrevLeafWalker: public PrevLeafWalkerBase<Types, PrevLeafWalker<Types>> {

protected:

    typedef PrevLeafWalkerBase<Types, PrevLeafWalker<Types>>                    Base;
    typedef typename Base::Key                                                  Key;
    typedef typename Base::Position                                             Position;
    typedef typename Base::Iterator                                             Iterator;

public:
    PrevLeafWalker(Int stream, Int index): Base(stream, index)
    {}
};





template <typename Types, typename MyType>
class FindMinBackwardWalker: public FindMinWalkerBase<Types, MyType> {

    typedef FindMinWalkerBase<Types, MyType>                                    Base;
    typedef typename Base::Key                                                  Key;
    typedef typename Base::Position                                             Position;
    typedef typename Base::Iterator                                             Iterator;

    static const Int Streams                                                    = Types::Streams;

public:
    FindMinBackwardWalker(UBigInt streams):
        Base(streams)
    {}

    typedef Int ReturnType;
    typedef Int ResultType;


    template <typename NodeTypes>
    ReturnType treeNode(const BranchNode<NodeTypes>* node, BigInt start)
    {
        for (auto& i: Base::search_results_) i = -1;

        node->processNotEmpty(Base::streams_, self(), start);

        Int max     = -1;
        Int max_idx = -1;

        for (Int c = 0; c < Streams; c++)
        {
            Int index = Base::search_results_[c];
            if (Base::isEnabled(c) && index > max)
            {
                max     = index;
                max_idx = c;
            }
        }

        MEMORIA_ASSERT_TRUE(max_idx < Streams);
        MEMORIA_ASSERT_TRUE(max_idx >= 0);

        self().postProcessNode(node, start, max_idx);

        return max_idx;
    }

    template <Int Idx, typename Tree>
    void stream(const Tree* tree, Int start)
    {
        auto k      = Base::target_[Idx] - Base::sum_[Idx];
        auto result = tree->findBackward(Base::search_type_, Base::indexes_[Idx], start, k);

        Base::sum_[Idx]             += result.prefix();
        Base::search_results_[Idx]  = result.idx();
    }

    template <typename NodeTypes>
    ReturnType treeNode(const LeafNode<NodeTypes>* node, BigInt start)
    {
        return 0;
    }

    void finish(Iterator& iter, bool start)
    {
    }

    MyType& self() {
        return *T2T<MyType*>(this);
    }

    const MyType& self() const {
        return *T2T<const MyType*>(this);
    }
};


template <typename Types>
class PrevLeafMultistreamWalker: public FindMinForwardWalkerBase<Types, PrevLeafMultistreamWalker<Types> > {
    typedef FindMinForwardWalkerBase<Types, PrevLeafMultistreamWalker<Types>>   Base;
public:
    PrevLeafMultistreamWalker(UBigInt streams): Base(streams) {}
};



template <typename Types>
class SelectForwardWalkerBase: public FindForwardWalkerBase<Types, SelectForwardWalkerBase<Types>> {

    typedef FindForwardWalkerBase<Types, SelectForwardWalkerBase<Types>>        Base;

protected:
    typedef typename Base::Key                                                  Key;

    BigInt pos_ = 0;

    Int size_index_;
    Int symbol_;

public:
    typedef typename Base::ResultType                                           ResultType;
    typedef typename Base::Iterator                                             Iterator;


    SelectForwardWalkerBase(Int stream, Int index, Int symbol, Key target, Int size_index = 0):
    	Base(stream, index + 1, target),
    	size_index_(size_index),
    	symbol_(symbol)
    {
        Base::search_type_ = SearchType::GE;
    }

    template <Int Idx, typename Tree>
    ResultType stream(const Tree* tree, Int start)
    {
        return Base::template stream<Idx>(tree, start);
    }

    template <Int StreamIdx, typename StreamType, typename Result>
    void postProcessStream(const StreamType* stream, Int start, const Result& result)
    {
        Int size    = stream->size();

        if (result.idx() < size)
        {
            pos_ += stream->sum(size_index_, start, result.idx());
        }
        else {
            pos_ += stream->sum(size_index_, start, size);
        }
    }

    template <Int Idx, typename StreamObj>
    ResultType select(const StreamObj* seq, Int start)
    {
        MEMORIA_ASSERT_TRUE(seq != nullptr);

        auto& sum       = Base::sum_;

        BigInt target   = Base::target_ - sum;
        auto result     = seq->selectFw(start, symbol_, target);

        if (result.is_found())
        {
            pos_ += result.idx() - start;
            return result.idx();
        }
        else {
            Int size = seq->size();

            sum  += result.rank();
            pos_ += (size - start);

            return size;
        }
    }
};




template <typename Types>
class SelectBackwardWalkerBase: public FindBackwardWalkerBase<Types, SelectBackwardWalkerBase<Types>> {

    typedef FindBackwardWalkerBase<Types, SelectBackwardWalkerBase<Types>>      Base;

protected:
    typedef typename Base::Key                                                  Key;

    BigInt pos_ = 0;

    Int size_index_;
    Int symbol_;

public:
    typedef typename Base::ResultType                                           ResultType;
    typedef typename Base::Iterator                                             Iterator;

    SelectBackwardWalkerBase(Int stream, Int index, Int symbol, Key target, Int size_index = 0):
    	Base(stream, index + 1, target),
    	size_index_(size_index),
    	symbol_(symbol)
    {
        Base::search_type_ = SearchType::GT;
    }

    template <Int Idx, typename Tree>
    ResultType stream(const Tree* tree, Int start)
    {
        return Base::template stream<Idx>(tree, start);
    }

    template <Int StreamIdx, typename StreamType, typename Result>
    void postProcessStream(const StreamType* stream, Int start, const Result& result)
    {
        if (result.idx() >= 0)
        {
            pos_ += stream->sum(size_index_, result.idx() + 1, start + 1);
        }
        else {
            pos_ += stream->sum(size_index_, 0, start + 1);
        }
    }

    template <Int Idx, typename StreamObj>
    ResultType select(const StreamObj* seq, Int start)
    {
        MEMORIA_ASSERT_TRUE(seq != nullptr);

        BigInt target   = Base::target_ - Base::sum_;

        auto& sum       = Base::sum_;

        auto result     = seq->selectBw(start, symbol_, target);

        if (result.is_found())
        {
            pos_ += start - result.idx();
            return result.idx();
        }
        else {
            pos_ += start;
            sum += result.rank();
            return -1;
        }
    }
};



}
}

#endif

