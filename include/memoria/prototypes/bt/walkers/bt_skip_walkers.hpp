
// Copyright Victor Smirnov 2013-2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_SKIP_WALKERS_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_SKIP_WALKERS_HPP

#include <memoria/prototypes/bt/walkers/bt_find_walkers.hpp>

namespace memoria {
namespace bt {

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
