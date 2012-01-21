
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

namespace memoria    {

using namespace memoria::itree;


MEMORIA_BSTREE_ITERATOR_BASE_CLASS_NO_CTOR_BEGIN(ITreeIteratorBase)
public:

	typedef typename Base::Container::Key                                        	 	Key;
    typedef typename Base::Container::NodeBase											NodeBase;

private:

    static const Int Indexes = Base::Container::Indexes;

    Key prefix_[Indexes];

public:
    ITreeIteratorBase(): Base()
    {
    	for (Int c = 0; c < Indexes; c++)
    	{
    		prefix_[c] = 0;
    	}
    }

    ITreeIteratorBase(ThisType&& other): Base(std::move(other))
    {
    	for (Int c = 0; c < Indexes; c++)
    	{
    		prefix_[c] = other.prefix_[c];
    	}
    }


    ITreeIteratorBase(const ThisType& other): Base(other)
    {
    	for (Int c = 0; c < Indexes; c++)
    	{
    		prefix_[c] = other.prefix_[c];
    	}
    }


    void operator=(const ThisType& other)
    {
    	for (Int c = 0; c < Indexes; c++)
    	{
    		prefix_[c] = other.prefix_[c];
    	}

    	Base::operator=(other);
    }


    bool operator==(const MyType& other) const
    {
    	for (Int c = 0; c < Indexes; c++)
    	{
    		if (prefix_[c] != other.prefix_[c])
    		{
    			return false;
    		}
    	}

    	return Base::operator==(other);
    }

    Int BuildHash() const {
    	return Base::BuildHash();
    }

    Key& prefix(Int i) {
    	return prefix_[i];
    }

    const Key prefix(Int i) const {
    	return prefix_[i];
    }

MEMORIA_BSTREE_ITERATOR_BASE_CLASS_END

} //memoria



#endif
