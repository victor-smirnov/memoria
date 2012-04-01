
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef __MEMORIA_PROTOTYPES_ITREE_ITERATOR_BASE_H
#define __MEMORIA_PROTOTYPES_ITREE_ITERATOR_BASE_H

#include <iostream>

#include <memoria/core/types/types.hpp>
#include <memoria/prototypes/bstree/names.hpp>
#include <memoria/prototypes/bstree/macros.hpp>

#include <memoria/core/tools/hash.hpp>

namespace memoria {

using namespace memoria::bstree;


MEMORIA_BSTREE_ITERATOR_BASE_CLASS_NO_CTOR_BEGIN(ITreeIteratorBase)
public:

	typedef typename Base::Container::Key                                        	 	Key;
    typedef typename Base::Container::NodeBase											NodeBase;
    typedef typename Base::Container::Accumulator										Accumulator;

private:

    Accumulator prefix_;

public:
    ITreeIteratorBase(): Base() {}

    ITreeIteratorBase(ThisType&& other): Base(std::move(other)), prefix_(std::move(other.prefix_))
    {}

    ITreeIteratorBase(const ThisType& other): Base(other), prefix_(other.prefix_)
    {}


    void Assign(const ThisType& other)
    {
    	prefix_ = other.prefix_;

    	Base::Assign(other);
    }

    void Assign(ThisType&& other)
    {
    	prefix_ = other.prefix_;

    	Base::Assign(std::move(other));
    }


    bool IsEqual(const ThisType& other) const
    {
    	return prefix_ == other.prefix_ && Base::IsEqual(other);
    }

    bool IsNotEqual(const ThisType& other) const
    {
    	return prefix_ != other.prefix_ || Base::IsNotEqual(other);
    }

    Key& prefix(Int i)
    {
    	return prefix_[i];
    }

    const Key prefix(Int i) const
    {
    	return prefix_[i];
    }

    Accumulator& prefix()
    {
    	return prefix_;
    }

    const Accumulator& prefix() const
    {
    	return prefix_;
    }

MEMORIA_BSTREE_ITERATOR_BASE_CLASS_END

}



#endif
