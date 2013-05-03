
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINERS_VECTORMAP_TOOLS_HPP
#define _MEMORIA_CONTAINERS_VECTORMAP_TOOLS_HPP

#include <memoria/prototypes/balanced_tree/baltree_tools.hpp>

#include <memoria/core/tools/static_array.hpp>
#include <memoria/core/tools/idata.hpp>

#include <memoria/core/container/container.hpp>



namespace memoria       {
namespace vmap     		{


template <typename Iterator, typename Container>
class VectorMapIteratorPrefixCache: public balanced_tree::BTreeIteratorCache<Iterator, Container> {
    typedef balanced_tree::BTreeIteratorCache<Iterator, Container> 				Base;
    typedef typename Container::Accumulator     								Accumulator;

    BigInt id_prefix_	= 0;
    BigInt id_entry_	= 0;
    BigInt size_		= 0;
    BigInt base_		= 0;

    static const Int MapIndexes 												= 2;

public:

    VectorMapIteratorPrefixCache(): Base() {}


    BigInt id() const
    {
    	return id_prefix_ + id_entry_;
    }

    BigInt id_prefix() const
    {
    	return id_prefix_;
    }

    BigInt id_entry() const
    {
    	return id_entry_;
    }

    BigInt size() const
    {
    	return size_;
    }

    BigInt blob_base() const
    {
    	return base_;
    }


    void setup(BigInt id_prefix, BigInt id_entry, BigInt base, BigInt size)
    {
    	id_prefix_ 	= id_prefix;
    	id_entry_	= id_entry;

    	size_		= size;
    	base_		= base;
    }

    void initState()
    {
    }

private:

    void init_()
    {

    }

};








}
}

#endif
