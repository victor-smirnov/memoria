
// Copyright Victor Smirnov 2013-2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_SKIP_WALKERS_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_SKIP_WALKERS_HPP

#include <memoria/prototypes/bt/walkers/bt_find_walkers.hpp>

namespace memoria {
namespace bt1     {

template <
    typename Types,
    typename IteratorPrefixFn,
    typename MyType
>
class SkipForwardWalkerBase: public FindForwardWalkerBase<Types, IteratorPrefixFn, MyType> {
protected:
    using Base  = FindForwardWalkerBase<Types, IteratorPrefixFn, MyType>;
    using Key   = typename Base::Key;

public:


    SkipForwardWalkerBase(Int stream, Int leaf_index, Key target):
        Base(stream, leaf_index, target, SearchType::GT)
    {}


    template <Int StreamIdx, typename Array>
    Int find_leaf(const Array* array, Int start)
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

                self().template postProcessLeafStream<StreamIdx>(array, start, start + offset);

                return start + offset;
            }
            else {
                sum += (size - start);

                fn.processLeafFw(array, std::get<StreamIdx>(Base::prefix_), start, size, 0, size - start);

                this->end_ = true;

                self().template postProcessLeafStream<StreamIdx>(array, start, size);

                return size;
            }
        }
        else {
            return 0;
        }
    }

    MyType& self() {return *T2T<MyType*>(this);}
    const MyType& self() const {return *T2T<const MyType*>(this);}
};



template <
    typename Types,
    typename IteratorPrefixFn = EmptyIteratorPrefixFn
>
class SkipForwardWalker: public SkipForwardWalkerBase<
                                    Types,
                                    IteratorPrefixFn,
                                    SkipForwardWalker<
                                        Types,
                                        IteratorPrefixFn
                                    >
> {
    using Base  = SkipForwardWalkerBase<
                    Types,
                    IteratorPrefixFn,
                    SkipForwardWalker<
                        Types,
                        IteratorPrefixFn
                    >
    >;

    using Key   = typename Base::Key;

public:

    SkipForwardWalker(Int stream, Int block, Key target):
        Base(stream, block, target)
    {}
};


template <
    typename Types,
    typename IteratorPrefixFn,
    typename MyType
>
class SkipBackwardWalkerBase: public FindBackwardWalkerBase<
                                        Types,
                                        IteratorPrefixFn,
                                        MyType
> {
protected:
    using Base  = FindBackwardWalkerBase<
                    Types,
                    IteratorPrefixFn,
                    MyType
    >;

    using Key   = typename Base::Key;

public:

    SkipBackwardWalkerBase(Int stream, Int leaf_index, Key target):
        Base(stream, leaf_index, target, SearchType::GE)
    {}


    template <Int StreamIdx, typename Array>
    Int find_leaf(const Array* array, Int start)
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

                self().template postProcessLeafStream<StreamIdx>(array, start - offset, start);

                return start - offset;
            }
            else {
                sum += start;

                fn.processLeafBw(array, std::get<StreamIdx>(Base::prefix_), 0, start, 0, start);

                this->end_ = true;

                self().template postProcessLeafStream<StreamIdx>(array, 0, start);

                return -1;
            }
        }
        else {
            return 0;
        }
    }

    MyType& self() {return *T2T<MyType*>(this);}
    const MyType& self() const {return *T2T<const MyType*>(this);}
};


template <
    typename Types,
    typename IteratorPrefixFn = EmptyIteratorPrefixFn
>
class SkipBackwardWalker: public SkipBackwardWalkerBase<
                                    Types,
                                    IteratorPrefixFn,
                                    SkipBackwardWalker<
                                        Types,
                                        IteratorPrefixFn
                                    >
> {
    using Base  = SkipBackwardWalkerBase<
                    Types,
                    IteratorPrefixFn,
                    SkipBackwardWalker<Types, IteratorPrefixFn>
    >;

    using Key   = typename Base::Key;

public:
    SkipBackwardWalker(Int stream, Int block, Key target):
        Base(stream, block, target)
    {}
};







/***************************************************************************************/


template <
    typename Types,
    typename MyType
>
class SkipForwardWalkerBase2: public FindForwardWalkerBase2<Types, MyType> {
protected:
    using Base  = FindForwardWalkerBase2<Types, MyType>;
    using CtrSizeT = typename Types::CtrSizeT;

public:

    SkipForwardWalkerBase2(CtrSizeT target):
        Base(-1, target, SearchType::GT)
    {}


    template <Int StreamIdx, typename Tree>
    StreamOpResult find_leaf(const Tree* tree, Int start)
    {
        if (DebugCounter == 1) {
        	int a = 0; a++;
        }

    	auto& sum = Base::sum_;

        BigInt offset = Base::target_ - sum;

        if (tree != nullptr)
        {
            Int size = tree->size();

            if (start + offset < size)
            {
                sum += offset;

                return StreamOpResult(start + offset, start, false);
            }
            else {
                sum += (size - start);

                return StreamOpResult(size, start, true);
            }
        }
        else {
            return StreamOpResult(0, start, true);
        }
    }

    MyType& self() {return *T2T<MyType*>(this);}
    const MyType& self() const {return *T2T<const MyType*>(this);}
};



template <
    typename Types
>
class SkipForwardWalker2: public SkipForwardWalkerBase2<Types, SkipForwardWalker2<Types>> {
    using Base  = SkipForwardWalkerBase2<Types, SkipForwardWalker2<Types>>;

    using CtrSizeT   = typename Base::CtrSizeT;

public:

    SkipForwardWalker2(CtrSizeT target):
        Base(target)
    {}
};




template <
    typename Types,
    typename MyType
>
class SkipBackwardWalkerBase2: public FindBackwardWalkerBase2<Types,MyType> {
protected:
    using Base  = FindBackwardWalkerBase2<Types,MyType>;

    using CtrSizeT = typename Types::CtrSizeT;

public:

    SkipBackwardWalkerBase2(CtrSizeT target):
        Base(-1, target, SearchType::GE)
    {}


    template <Int StreamIdx, typename Tree>
    StreamOpResult find_leaf(const Tree* tree, Int start)
    {
    	if (start > tree->size()) start = tree->size();

    	if (start >= 0)
    	{
    		BigInt offset = Base::target_ - Base::sum_;

    		auto& sum = Base::sum_;

    		if (tree != nullptr)
    		{
    			if (start - offset >= 0)
    			{
    				sum += offset;

    				return StreamOpResult(start - offset, start, false);
    			}
    			else {
    				sum += start;

    				return StreamOpResult(-1, start, true);
    			}
    		}
    		else {
    			return StreamOpResult(-1, start, true, true);
    		}
    	}
    	else {
    		return StreamOpResult(-1, start, true, true);
    	}
    }

    MyType& self() {return *T2T<MyType*>(this);}
    const MyType& self() const {return *T2T<const MyType*>(this);}
};


template <
    typename Types
>
class SkipBackwardWalker2: public SkipBackwardWalkerBase2<Types, SkipBackwardWalker2<Types>> {
    using Base  = SkipBackwardWalkerBase2<Types,SkipBackwardWalker2<Types>>;

    using CtrSizeT = typename Base::CtrSizeT;

public:
    SkipBackwardWalker2(CtrSizeT target):
        Base(target)
    {}
};




}
}

#endif
