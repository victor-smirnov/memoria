
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINERS_SMRKMAP_SELECT_WALKERS_HPP
#define _MEMORIA_CONTAINERS_SMRKMAP_SELECT_WALKERS_HPP

#include <memoria/core/tools/static_array.hpp>
#include <memoria/prototypes/bt/bt_walkers.hpp>

#include <ostream>

namespace memoria       {
namespace smrk_map      {

template <typename Types>
class SelectForwardWalker: public bt::FindForwardWalkerBase<Types, SelectForwardWalker<Types>> {

    typedef bt::FindForwardWalkerBase<Types, SelectForwardWalker<Types>>        Base;
    typedef typename Base::Key                                                  Key;

public:
    typedef typename Base::ResultType                                           ResultType;
    typedef typename Base::Iterator                                             Iterator;

    BigInt prefix_;

    SelectForwardWalker(Int stream, Int symbol, Key target): Base(stream, symbol + 2, target)
    {
        Base::search_type_ = SearchType::LE;
    }

    template <Int Idx, typename Tree>
    ResultType stream(const Tree* tree, Int start)
    {
        return Base::template stream<Idx>(tree, start);
    }

    template <Int StreamIdx, typename StreamType, typename Result>
    void postProcessStream(const StreamType* stream, Int start, const Result& result)
    {
        Int size    = stream->size();

        if (result.idx() < size)
        {
            prefix_ += stream->sum(1, start, result.idx());
        }
        else {
            prefix_ += stream->sum(1, start, size);
        }
    }

    template <Int Idx, typename StreamTypes>
    ResultType stream(const PackedFSESearchableMarkableMap<StreamTypes>* seq, Int start)
    {
        MEMORIA_ASSERT_TRUE(seq != nullptr);

        auto& sum       = Base::sum_;
        auto symbol     = Base::index_ - 2;

        BigInt target   = Base::target_ - sum;

        auto result     = seq->selectFw(start, symbol, target);

        if (result.is_found())
        {
            prefix_ += seq->sum(0, start, result.idx());

        	return result.idx();
        }
        else {
            Int size = seq->size();

            sum  += result.rank();

            prefix_ += seq->sum(0, start, seq->size());

            return size;
        }
    }

    void prepare(Iterator& iter)
    {
    	Base::prepare(iter);

    	prefix_ = iter.cache().prefix();
    }


    BigInt finish(Iterator& iter, Int idx)
    {
    	iter.cache().setup(prefix_);

    	return Base::finish(iter, idx);
    }
};



}
}

#endif
