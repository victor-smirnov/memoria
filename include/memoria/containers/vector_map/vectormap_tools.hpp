
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
    typedef balanced_tree::BTreeIteratorCache<Iterator, Container> Base;
    typedef typename Container::Accumulator     Accumulator;

    Accumulator prefix_;
    Accumulator current_;

    static const Int Indexes = 1;

public:

    VectorMapIteratorPrefixCache(): Base(), prefix_(0), current_(0) {}

    const BigInt& prefix(Int stream, Int num = 0) const
    {
        return GetValue(prefix_, stream, num);
    }

    const Accumulator prefixes() const
    {
        return prefix_;
    }

    void nextLeaf(bool end)
    {
        prefix_ += current_;

        Clear(current_);
    };

    void prevLeaf(bool start)
    {
        prefix_ -= current_;

        Clear(current_);
    };

    void Prepare()
    {
    	current_ = Base::iterator().keys();
    }

    void setup(const Accumulator& prefix)
    {
        prefix_ = prefix;
    }

    void Clear(Accumulator& prefix)
    {
    	prefix = Accumulator();
    }

    void initState()
    {
        Clear(prefix_);

        typedef typename Iterator::Container::TreePath TreePath;
        const TreePath& path = Base::iterator().path();

        for (Int c = 1; c < path.getSize(); c++)
        {
        	Int idx  = path[c - 1].parent_idx();

        	Base::iterator().model().sumKeys(path[c].node(), 0, idx, prefix_);
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
