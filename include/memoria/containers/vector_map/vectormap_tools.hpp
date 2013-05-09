
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

typedef std::pair<BigInt, BigInt> VectorMapEntry;


class VectorMapSource: public ISource {

	IDataBase* sources_[2];

	EmptyDataSource<VectorMapEntry> map_source_;

public:
	VectorMapSource(IDataBase* source)
	{
		sources_[0] = &map_source_;
		sources_[1] = source;
	}

	virtual Int streams()
	{
		return 2;
	}

	virtual IData* stream(Int stream)
	{
		return sources_[stream];
	}

	virtual void newNode(INodeLayoutManager* layout_manager, BigInt* sizes)
	{
		Int allocated[2] = {0, 0};
		Int capacity = layout_manager->getNodeCapacity(allocated, 1);

		sizes[1] = capacity;
	}

	virtual BigInt getTotalNodes(INodeLayoutManager* manager)
	{
		Int sizes[2] = {0, 0};

		SizeT capacity 	= manager->getNodeCapacity(sizes, 1);
		SizeT remainder = sources_[1]->getRemainder();

		return remainder / capacity + (remainder % capacity ? 1 : 0);
	}
};


class VectorMapTarget: public ITarget {

	IDataBase* targets_[2];

	EmptyDataTarget<VectorMapEntry> map_target_;
public:
	VectorMapTarget(IDataBase* target)
	{
		targets_[0] = &map_target_;
		targets_[1] = target;
	}

	virtual Int streams()
	{
		return 2;
	}

	virtual IData* stream(Int stream)
	{
		return targets_[stream];
	}
};


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

    void add(BigInt id_entry, BigInt size)
    {
    	id_prefix_ 	+= id_entry_;
    	base_ 		+= size_;

    	id_entry_   = id_entry;
    	size_ 		= size;
    }

    void sub(BigInt id_entry, BigInt size)
    {
    	id_prefix_ 	-= id_entry_;
    	base_ 		-= size_;

    	id_entry_   = id_entry;
    	size_ 		= size;
    }
};








}
}

#endif
