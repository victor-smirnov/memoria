
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_FIND_WALKERS_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_FIND_WALKERS_HPP

#include <memoria/prototypes/bt/walkers/bt_walker_base.hpp>


namespace memoria {
namespace bt1     {


template <typename Types, typename MyType, typename BranchPath, typename LeafPath>
class FindWalkerBase: public WalkerBase<Types, MyType, BranchPath, LeafPath> {

protected:
    using Base = WalkerBase<Types, MyType, BranchPath, LeafPath>;
    typedef typename Base::Key                                                  Key;

public:

    FindWalkerBase(Int stream, Int branch_index, Int leaf_index, Key target, SearchType search_type):
        Base(stream, branch_index, leaf_index, target)
    {
        Base::search_type() = search_type;
    }
};


template <typename Types, typename BranchPath, typename LeafPath, typename IteratorPrefixFn, typename MyType>
class FindForwardWalkerBase: public FindWalkerBase<Types, MyType, BranchPath, LeafPath> {

protected:
    using Base = FindWalkerBase<Types, MyType, BranchPath, LeafPath>;
    typedef typename Base::Key                                                  Key;

public:
    FindForwardWalkerBase(Int stream, Int branch_index, Int leaf_index, Key target, SearchType search_type):
        Base(stream, branch_index, leaf_index, target, search_type)
    {}

    template <Int StreamIdx, typename Tree>
    Int find_non_leaf(const Tree* tree, Int start)
    {
        auto k = Base::target_ - Base::sum_;

        Int index   = this->branch_index();

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
    typename BranchPath,
    typename LeafPath,
    typename IteratorPrefixFn = EmptyIteratorPrefixFn
>
class FindForwardWalker: public FindForwardWalkerBase<
                                    Types,
                                    BranchPath,
                                    LeafPath,
                                    IteratorPrefixFn,
                                    FindForwardWalker<Types, BranchPath, LeafPath, IteratorPrefixFn>> {

    using Base  = FindForwardWalkerBase<
                    Types,
                    BranchPath,
                    LeafPath,
                    IteratorPrefixFn,
                    FindForwardWalker<
                        Types,
                        BranchPath,
                        LeafPath,
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
    typename BranchPath,
    typename LeafPath,
    typename IteratorPrefixFn = EmptyIteratorPrefixFn
>
class FindGTForwardWalker: public FindForwardWalkerBase<
                                    Types,
                                    BranchPath, LeafPath,
                                    IteratorPrefixFn,
                                    FindGTForwardWalker<Types, BranchPath, LeafPath, IteratorPrefixFn>> {

    using Base  = FindForwardWalkerBase<
                    Types,
                    BranchPath,
                    LeafPath,
                    IteratorPrefixFn,
                    FindGTForwardWalker<
                        Types,
                        BranchPath,
                        LeafPath,
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
    typename BranchPath,
    typename LeafPath,
    typename IteratorPrefixFn = EmptyIteratorPrefixFn
>
class FindGEForwardWalker: public FindForwardWalkerBase<
                                    Types,
                                    BranchPath,
                                    LeafPath,
                                    IteratorPrefixFn,
                                    FindGEForwardWalker<Types, BranchPath, LeafPath, IteratorPrefixFn>> {

    using Base  = FindForwardWalkerBase<
                    Types,
                    BranchPath,
                    LeafPath,
                    IteratorPrefixFn,
                    FindGEForwardWalker<
                        Types,
                        BranchPath,
                        LeafPath,
                        IteratorPrefixFn
                    >
    >;

    using Key   = typename Base::Key;

public:

    static const Int StreamIdx = 0;
    static const Int SubstreamIdx = 0;

    FindGEForwardWalker(Int stream, Int branch_index, Int leaf_index, Key target):
        Base(stream, branch_index, leaf_index, target, SearchType::GE)
    {}
};





template <
    typename Types,
    typename BranchPath,
    typename LeafPath,
    typename IteratorPrefixFn,
    typename MyType
>
class FindBackwardWalkerBase: public FindWalkerBase<Types, MyType, BranchPath, LeafPath> {
protected:
    using Base = FindWalkerBase<Types, MyType, BranchPath, LeafPath>;

protected:
    typedef typename Base::Key                                                  Key;

public:

    FindBackwardWalkerBase(Int stream, Int branch_index, Int leaf_index, Key target, SearchType search_type):
        Base(stream, branch_index, leaf_index, target, search_type)
    {}

    template <Int StreamIdx, typename Tree>
    Int find_non_leaf(const Tree* tree, Int start)
    {
        auto k          = Base::target_ - Base::sum_;

        Int index       = this->branch_index();

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
    typename BranchPath,
    typename LeafPath,
    typename IteratorPrefixFn = EmptyIteratorPrefixFn
>
class FindBackwardWalker: public FindBackwardWalkerBase<
                                    Types,
                                    BranchPath,
                                    LeafPath,
                                    IteratorPrefixFn,
                                    FindBackwardWalker<Types, BranchPath, LeafPath, IteratorPrefixFn>> {

    using Base  = FindBackwardWalkerBase<
                    Types,
                    BranchPath, LeafPath,
                    IteratorPrefixFn,
                    FindBackwardWalker<
                        Types,
                        BranchPath, LeafPath,
                        IteratorPrefixFn
                    >
    >;

    using Key   = typename Base::Key;

public:
    FindBackwardWalker(Int stream, Int branch_index, Int leaf_index, Key target, SearchType search_type = SearchType::GE):
        Base(stream, branch_index, leaf_index, target, search_type)
    {}
};




}
}

#endif
