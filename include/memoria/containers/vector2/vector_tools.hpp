
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINERS_VECTOR2_TOOLS_HPP
#define _MEMORIA_CONTAINERS_VECTOR2_TOOLS_HPP

#include <memoria/prototypes/btree/tools.hpp>

#include <memoria/core/tools/static_array.hpp>
#include <memoria/core/tools/idata.hpp>

#include <memoria/core/container/container.hpp>



namespace memoria       {
namespace mvector2     	{

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

	virtual void newNode(INodeLayoutManager* layout_manager)
	{}

	virtual BigInt getTotalNodes(INodeLayoutManager* manager)
	{
		Int sizes[1] = {0};

		SizeT capacity 	= manager->getNodeCapacity(sizes, 0);
		SizeT remainder = source_->getRemainder();

		return remainder / capacity + (remainder % capacity ? 1 : 0);
	}
};


template <typename Iterator, typename Container>
class VectorIteratorPrefixCache: public balanced_tree::BTreeIteratorCache<Iterator, Container> {
    typedef balanced_tree::BTreeIteratorCache<Iterator, Container> Base;
    typedef typename Container::Accumulator     Accumulator;

    Accumulator prefix_;
    Accumulator current_;

    static const Int Indexes = 1;

public:

    VectorIteratorPrefixCache(): Base(), prefix_(), current_() {}

    const BigInt& prefix(int num = 0) const
    {
        return get<0>(prefix_)[num];
    }

    const Accumulator& prefixes() const
    {
        return prefix_;
    }

    void nextKey(bool end)
    {
        VectorAdd(prefix_, current_);

        Clear(current_);
    };

    void prevKey(bool start)
    {
        VectorSub(prefix_, current_);

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

    void setup(BigInt prefix, Int key_num)
    {
        get<0>(prefix_)[key_num] = prefix;

        init_(key_num);
    }

    void setup(const Accumulator& prefix)
    {
        prefix_ = prefix;
    }

    void initState()
    {
        Clear(prefix_);

        Int idx  = Base::iterator().key_idx();

        if (idx >= 0)
        {
            typedef typename Iterator::Container::TreePath TreePath;
            const TreePath& path = Base::iterator().path();

            for (Int c = 1; c < path.getSize(); c++)
            {
                Base::iterator().model().sumKeys(path[c].node(), 0, idx, prefix_);
                idx = path[c].parent_idx();
            }
        }
    }

private:

    void init_(Int skip_num)
    {
        typedef typename Iterator::Container::TreePath TreePath;

        const TreePath& path = Base::iterator().path();
        Int             idx  = Base::iterator().key_idx();

        for (Int c = 0; c < Indexes; c++) {
            if (c != skip_num) prefix_[c] = 0;
        }

        for (Int c = 0; c < path.getSize(); c++)
        {
            for (Int block_num = 0; block_num < Indexes; block_num++)
            {
                if (block_num != skip_num)
                {
                    Base::iterator().model().sumKeys(path[c].node(), block_num, 0, idx, prefix_[block_num]);
                }
            }

            idx = path[c].parent_idx();
        }
    }

};




}
}

#endif
