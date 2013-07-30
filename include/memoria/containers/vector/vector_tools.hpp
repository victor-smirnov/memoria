
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINERS_VECTOR_TOOLS_HPP
#define _MEMORIA_CONTAINERS_VECTOR_TOOLS_HPP

#include <memoria/prototypes/balanced_tree/bt_tools.hpp>

#include <memoria/core/tools/static_array.hpp>
#include <memoria/core/tools/idata.hpp>

#include <memoria/core/container/container.hpp>



namespace memoria       {
namespace mvector      	{

class VectorSource: public ISource {

	IDataBase* source_;
public:
	VectorSource(IDataBase* source): source_(source) {}

	virtual Int streams()
	{
		return 1;
	}

	virtual IData* stream(Int stream)
	{
		return source_;
	}

	virtual void newNode(INodeLayoutManager* layout_manager, BigInt* sizes)
	{
		Int allocated[1] = {0};
		Int capacity = layout_manager->getNodeCapacity(allocated, 0);

		sizes[0] = capacity;
	}

	virtual BigInt getTotalNodes(INodeLayoutManager* manager)
	{
		Int sizes[1] = {0};

		SizeT capacity 	= manager->getNodeCapacity(sizes, 0);
		SizeT remainder = source_->getRemainder();

		return remainder / capacity + (remainder % capacity ? 1 : 0);
	}
};


class VectorTarget: public ITarget {

	IDataBase* target_;
public:
	VectorTarget(IDataBase* target): target_(target) {}

	virtual Int streams()
	{
		return 1;
	}

	virtual IData* stream(Int stream)
	{
		return target_;
	}
};




template <typename Iterator, typename Container>
class VectorIteratorPrefixCache: public balanced_tree::BTreeIteratorCache<Iterator, Container> {
    typedef balanced_tree::BTreeIteratorCache<Iterator, Container> 				Base;
    typedef typename Container::Position 										Position;
    typedef typename Container::Accumulator 									Accumulator;

    Position prefix_;
    Position current_;

    static const Int Indexes = 1;

public:

    VectorIteratorPrefixCache(): Base(), prefix_(), current_() {}

    const BigInt& prefix(int num = 0) const
    {
        return prefix_[num];
    }

    const Position& sizePrefix() const
    {
    	return prefix_;
    }

    void setSizePrefix(const Position& prefix)
    {
    	prefix_ = prefix;
    }

    const Position& prefixes() const
    {
        return prefix_;
    }

    void nextKey(bool end)
    {
        prefix_ += current_;

        Clear(current_);
    };

    void prevKey(bool start)
    {
        prefix_ -= current_;

        Clear(current_);
    };

    void Prepare()
    {
        if (Base::iterator().key_idx() >= 0)
        {
//            current_ = Base::iterator().getRawKeys();
        }
        else {
            Clear(current_);
        }
    }

    void setup(const Position& prefix)
    {
        prefix_ = prefix;
    }

    void setup(BigInt prefix)
    {
    	prefix_[0] = prefix;
    }

    void Clear(Position& v) {v = Position();}

    void initState()
    {
        Clear(prefix_);

        auto node = Base::iterator().leaf();

        auto& ctr = Base::iterator().ctr();

        while (!node->is_root())
        {
        	Int idx = node->parent_idx();
        	node = ctr.getNodeParent(node);

        	Accumulator acc;

        	ctr.sumKeys(node, 0, idx, acc);

        	prefix_ += std::get<0>(acc)[0];
        }
    }

private:

    void init_()
    {

    }

};




}
}

#endif
