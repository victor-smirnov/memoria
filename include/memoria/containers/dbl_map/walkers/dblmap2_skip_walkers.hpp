
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINERS_DBLMAP2_SKIP_WALKERS_HPP
#define _MEMORIA_CONTAINERS_DBLMAP2_SKIP_WALKERS_HPP

#include <memoria/prototypes/bt/bt_tools.hpp>
#include <memoria/core/tools/static_array.hpp>

#include <memoria/core/container/container.hpp>

#include <memoria/prototypes/bt/bt_walkers.hpp>

#include <memoria/core/packed/tree/packed_fse_tree.hpp>
#include <memoria/core/packed/map/packed_map.hpp>
#include <memoria/core/packed/map/packed_fse_mark_map.hpp>

#include <ostream>

namespace memoria	{
namespace dblmap	{

namespace inner 	{


template <typename Types>
class SkipForwardWalker: public bt::FindForwardWalkerBase<Types, SkipForwardWalker<Types>> {
    typedef bt::FindForwardWalkerBase<Types, SkipForwardWalker<Types>>              Base;
    typedef typename Base::Key                                                  	Key;

    typedef typename Types::Accumulator												Accumulator;
    typedef typename Base::Iterator													Iterator;

    Accumulator prefix_;

public:
    typedef typename Base::ResultType												ResultType;

    SkipForwardWalker(Int stream, Int index, Key target): Base(stream, index, target)
    {}

    template <Int StreamIdx, typename StreamType, typename SearchResult>
    void postProcessStream(const StreamType* tree, Int start, const SearchResult& result)
    {
    	tree->sums(start, result.idx(), std::get<StreamIdx>(prefix_));
    }

    template <Int Idx, typename Tree>
    ResultType stream(const Tree* tree, Int start)
    {
    	return Base::template tree<Idx>(tree, start);
    }

    template <Int Idx, typename TreeTypes>
    ResultType stream(const PackedFSEMarkableMap<TreeTypes>* tree, Int start)
    {
    	auto& sum = Base::sum_;

    	BigInt offset = Base::target_ - sum;

    	Int size = tree->size();

    	if (start + offset < size)
    	{
    		sum += offset;

    		tree->sums(start, start + offset, std::get<Idx>(prefix_));

    		return start + offset;
    	}
    	else {
    		sum += (size - start);

    		tree->sums(start, size, std::get<Idx>(prefix_));

    		return size;
    	}
    }


    void prepare(Iterator& iter)
    {
    	prefix_ = iter.cache().prefixes();
    }

    BigInt finish(Iterator& iter, Int idx)
    {
    	iter.idx() = idx;
    	iter.cache().setup(prefix_);
    	return Base::sum_;
    }
};


template <typename Types>
class SkipBackwardWalker: public bt::FindBackwardWalkerBase<Types, SkipBackwardWalker<Types>> {
    typedef bt::FindBackwardWalkerBase<Types, SkipBackwardWalker<Types>>            Base;
    typedef typename Base::Key                                                  	Key;

    typedef typename Types::Accumulator												Accumulator;
    typedef typename Base::Iterator													Iterator;

    Accumulator prefix_;

public:
    typedef typename Base::ResultType												ResultType;

    SkipBackwardWalker(Int stream, Int index, Key target): Base(stream, index, target)
    {}

    template <Int StreamIdx, typename StreamType, typename SearchResult>
    void postProcessStream(const StreamType* tree, Int start, const SearchResult& result)
    {
    	auto begin = result.idx();

    	Accumulator sums;

    	tree->sums(begin >= 0 ? begin + 1 : 0, start + 1, std::get<StreamIdx>(sums));

    	VectorSub(prefix_, sums);
    }


    template <Int Idx, typename Tree>
    ResultType stream(const Tree* tree, Int start)
    {
    	return Base::template tree<Idx>(tree, start);
    }

    template <Int Idx, typename TreeTypes>
    ResultType stream(const PackedFSEMarkableMap<TreeTypes>* tree, Int start)
    {
    	Int pos = Base::template array<Idx>(tree, start);

    	Accumulator sums;

    	tree->sums(pos >= 0 ? pos : 0, start, std::get<Idx>(sums));

    	VectorSub(prefix_, sums);

    	return pos;
    }

    void prepare(Iterator& iter)
    {
    	prefix_ = iter.cache().prefixes();
    }

    BigInt finish(Iterator& iter, Int idx)
    {
    	iter.idx() = idx;
    	iter.cache().setup(prefix_);
    	return idx;
    }
};


}

}
}

#endif
