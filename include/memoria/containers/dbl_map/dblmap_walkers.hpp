
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINERS_DBLMAP_CONTAINER_WALKERS_HPP
#define _MEMORIA_CONTAINERS_DBLMAP_CONTAINER_WALKERS_HPP

#include <memoria/prototypes/bt/bt_tools.hpp>
#include <memoria/core/tools/static_array.hpp>

#include <memoria/core/container/container.hpp>

#include <memoria/prototypes/bt/bt_walkers.hpp>

#include <memoria/prototypes/bt/nodes/leaf_node.hpp>

#include <ostream>

namespace memoria       {
namespace dblmap        {


template <typename Types>
class FindWalkerBase {
protected:
    typedef typename Types::Key                                                 Key;
    typedef typename Types::Accumulator                                         Accumulator;
    typedef Iter<typename Types::IterTypes>                                     Iterator;

    Accumulator prefix_;

    Key sum_ = 0;

    Int stream_;
    Int index_;
    Key key_;


    WalkDirection direction_;

public:

    FindWalkerBase(Int stream, Int index, Key key):
        stream_(stream),
        index_(index),
        key_(key)
    {}

    const WalkDirection& direction() const {
        return direction_;
    }

    WalkDirection& direction() {
        return direction_;
    }

    void prepare(Iterator& iter)
    {
        std::get<0>(prefix_)[0] = iter.cache().id_prefix();
        std::get<0>(prefix_)[1] = iter.cache().blob_base();
    }

    BigInt finish(Iterator& iter, Int idx)
    {
        iter.idx()  = idx;

        BigInt id_prefix    = std::get<0>(prefix_)[0];
        BigInt base         = std::get<0>(prefix_)[1];

        BigInt id_entry     = 0;
        BigInt size         = 0;



        if (idx >=0 && idx < iter.leafSize(stream_))
        {
            if (stream_ == 0)
            {
                auto entry  = iter.entry();

                id_entry    = entry.first;
                size        = entry.second;
            }
        }

        Int entry_idx = 0;
        if (stream_ == 0)
        {
            entry_idx = idx;
        }
        else {
            // FIXME: correct entry_idx for data stream
            entry_idx = iter.cache().entry_idx();
        }

        iter.cache().setup(id_prefix, id_entry, size, base, entry_idx);

        return sum_;
    }


    void empty(Iterator& iter)
    {
        iter.idx()  = 0;
    }

    BigInt prefix() const {
        return prefix_;
    }
};





template <typename Types>
class MapFindWalker: public FindForwardWalkerBase<Types, MapFindWalker<Types>> {

    typedef FindForwardWalkerBase<Types, MapFindWalker<Types>>                  Base;
    typedef typename Base::Key                                                  Key;
    typedef typename Types::Accumulator                                         Accumulator;
    typedef Iter<typename Types::IterTypes>                                     Iterator;

    Accumulator prefix_;

public:
    MapFindWalker(Key key):
        Base(0, 0, key)
    {
        Base::search_type() = SearchType::GE;
    }


    template <Int StreamIdx, typename StreamTypes, typename SearchResult>
    void postProcessStream(const PkdFTree<StreamTypes>* tree, Int start, const SearchResult& result)
    {
        auto& index     = Base::index_;

        std::get<StreamIdx>(prefix_)[index]         += result.prefix();
        std::get<StreamIdx>(prefix_)[1 - index]     += tree->sum(1 - index, start, result.idx());
    }

//    template <Int StreamIdx, typename StreamTypes, typename SearchResult>
//    void postProcessStream(const PackedMap<StreamTypes>* tree, Int start, const SearchResult& result)
//    {
//        auto& index     = Base::index_;
//
//        std::get<StreamIdx>(prefix_)[index]         += result.prefix();
//        std::get<StreamIdx>(prefix_)[1 - index]     += tree->sum(1 - index, start, result.idx());
//    }

    template <Int StreamIdx, typename StreamTypes, typename SearchResult>
    void postProcessStream(const PackedFSEMarkableMap<StreamTypes>* tree, Int start, const SearchResult& result)
    {
        auto& index     = Base::index_;

        std::get<StreamIdx>(prefix_)[index]         += result.prefix();
        std::get<StreamIdx>(prefix_)[1 - index]     += tree->sum(1 - index, start, result.idx());
    }


    void prepare(Iterator& iter)
    {
    }

    BigInt finish(Iterator& iter, Int idx)
    {
        iter.idx()  = idx;

        BigInt id_prefix    = std::get<0>(prefix_)[0];
        BigInt base         = std::get<0>(prefix_)[1];

        BigInt id_entry     = 0;
        BigInt size         = 0;

        BigInt global_pos   = base;

        Int entries         = iter.leaf_size(0);

        if (idx >=0 && idx < entries)
        {
            auto entry  = iter.entry();

            id_entry    = entry.first;
            size        = entry.second;
        }

        iter.cache().setup(id_prefix, id_entry, base, size, idx, entries, global_pos);

        iter.cache().second_prefix() = 0;

        return Base::sum_;
    }
};



template <typename Types>
class SecondMapFindWalkerBase: public FindForwardWalkerBase<Types, SecondMapFindWalkerBase<Types>> {

    typedef FindForwardWalkerBase<Types, SecondMapFindWalkerBase<Types>>        Base;

protected:
    typedef typename Base::Key                                                  Key;
    typedef typename Types::Accumulator                                         Accumulator;
    typedef Iter<typename Types::IterTypes>                                     Iterator;

    BigInt prefix_ = 0;

public:
    SecondMapFindWalkerBase(Int stream, Int index, Key key):
        Base(stream, index, key)
    {}


    template <Int StreamIdx, typename StreamTypes, typename SearchResult>
    void postProcessStream(const PkdFTree<StreamTypes>* tree, Int start, const SearchResult& result)
    {
        prefix_ += tree->sum(0, start, result.idx());
    }

//    template <Int StreamIdx, typename StreamTypes, typename SearchResult>
//    void postProcessStream(const PackedFSEMap<StreamTypes>* tree, Int start, const SearchResult& result)
//    {
//      prefix_ += result.idx() - start;
//    }

    template <Int StreamIdx, typename StreamTypes, typename SearchResult>
    void postProcessStream(const PackedFSEMarkableMap<StreamTypes>* tree, Int start, const SearchResult& result)
    {
        prefix_ += result.idx() - start;
    }



    template <Int Idx, typename StreamObj>
    typename Base::ResultType stream(const StreamObj* obj, Int start)
    {
        Base::index_ = 1;
        return Base::template stream<Idx>(obj, start);
    }


//    template <Int Idx, typename TreeTypes>
//    typename Base::ResultType stream(const PackedFSEMap<TreeTypes>* map, Int start)
//    {
//      Base::index_ = 0;
//      return Base::template tree<Idx>(map, start);
//    }

    template <Int Idx, typename TreeTypes>
    typename Base::ResultType stream(const PackedFSEMarkableMap<TreeTypes>* map, Int start)
    {
        Base::index_ = 0;
        return Base::template tree<Idx>(map, start);
    }


    void prepare(Iterator& iter)
    {}

    BigInt finish(Iterator& iter, Int idx)
    {
        iter.idx() = idx;

        iter.cache().second_prefix() = Base::sum_;

        return prefix_;
    }
};



template <typename Types>
class SecondMapFindGEWalker: public SecondMapFindWalkerBase<Types> {

    typedef SecondMapFindWalkerBase<Types>                                      Base;

protected:
    typedef typename Base::Key                                                  Key;
    typedef typename Types::Accumulator                                         Accumulator;
    typedef Iter<typename Types::IterTypes>                                     Iterator;

public:
    SecondMapFindGEWalker(Int stream, Int index, Key key):
        Base(stream, index, key)
    {
        Base::search_type() = SearchType::GE;
    }
};

template <typename Types>
class SecondMapFindGTWalker: public SecondMapFindWalkerBase<Types> {

    typedef SecondMapFindWalkerBase<Types>                                      Base;

protected:
    typedef typename Base::Key                                                  Key;
    typedef typename Types::Accumulator                                         Accumulator;
    typedef Iter<typename Types::IterTypes>                                     Iterator;

public:
    SecondMapFindGTWalker(Int stream, Int index, Key key):
        Base(stream, index, key)
    {
        Base::search_type() = SearchType::GT;
    }
};




template <typename Types>
class SkipForwardWalker: public FindForwardWalkerBase<Types, SkipForwardWalker<Types>> {

    typedef FindForwardWalkerBase<Types, SkipForwardWalker<Types>>              Base;
    typedef typename Types::Key                                                 Key;
    typedef typename Types::Accumulator                                         Accumulator;

    BigInt prefix_ = 0;

public:

    typedef typename Base::ResultType                                           ResultType;

    SkipForwardWalker(Int stream, Int index, Key distance):
        Base(stream, index, distance)
    {
        Base::search_type() = SearchType::GT;
    }

    template <Int Idx, typename StreamObj>
    ResultType stream(const StreamObj* obj, Int start) {
        return Base::template stream<Idx>(obj, start);
    }


//    template <Int Idx, typename TreeTypes>
//    ResultType stream(const PackedFSEMap<TreeTypes>* map, Int start)
//    {
//      ResultType result = Base::template array<Idx>(map, start);
//
//      prefix_ += map->sum(0, start, result);
//
//      return result;
//    }

    template <Int Idx, typename TreeTypes>
    ResultType stream(const PackedFSEMarkableMap<TreeTypes>* map, Int start)
    {
        ResultType result = Base::template array<Idx>(map, start);

        prefix_ += map->sum(0, start, result);

        return result;
    }



    template <Int StreamIdx, typename StreamTypes, typename SearchResult>
    void postProcessStream(const PkdFTree<StreamTypes>* tree, Int start, const SearchResult& result)
    {
        prefix_ += tree->sum(1, start, result.idx());
    }

    void prepare(typename Base::Iterator& iter)
    {
        prefix_ = iter.cache().second_prefix();

        Base::prepare(iter);
    }

    BigInt finish(typename Base::Iterator& iter, Int idx)
    {
        iter.cache().second_prefix() = prefix_;

        return Base::finish(iter, idx);
    }
};


template <typename Types>
class SkipBackwardWalker: public FindBackwardWalkerBase<Types, SkipBackwardWalker<Types>> {

    typedef FindBackwardWalkerBase<Types, SkipBackwardWalker<Types>>            Base;
    typedef typename Types::Key                                                 Key;
    typedef typename Types::Accumulator                                         Accumulator;

    BigInt prefix_ = 0;

public:

    typedef typename Base::ResultType                                           ResultType;

    SkipBackwardWalker(Int stream, Int index, Key distance):
        Base(stream, index, distance)
    {
        Base::search_type() = SearchType::GT;
    }

    template <Int Idx, typename StreamObj>
    ResultType stream(const StreamObj* obj, Int start) {
        return Base::template stream<Idx>(obj, start);
    }

//    template <Int Idx, typename TreeTypes>
//    ResultType stream(const PackedFSEMap<TreeTypes>* map, Int start)
//    {
//      auto result = Base::template array<Idx>(map, start);
//
//      auto begin = result >= 0 ? result : 0;
//
//      auto sum = map->sum(0, begin, start);
//
//      prefix_ -= sum;
//
//      return result;
//    }

    template <Int Idx, typename TreeTypes>
    ResultType stream(const PackedFSEMarkableMap<TreeTypes>* map, Int start)
    {
        auto result = Base::template array<Idx>(map, start);

        auto begin = result >= 0 ? result : 0;

        auto sum = map->sum(0, begin, start);

        prefix_ -= sum;

        return result;
    }



    template <Int StreamIdx, typename StreamTypes, typename SearchResult>
    void postProcessStream(const PkdFTree<StreamTypes>* tree, Int start, const SearchResult& result)
    {
        auto begin = result.idx();// >= 0 ? result.idx() : 0;

        auto sum = tree->sum(1, begin + 1, start + 1);

        prefix_ -= sum;
    }


    void prepare(typename Base::Iterator& iter)
    {
        prefix_ = iter.cache().second_prefix();

        Base::prepare(iter);
    }

    BigInt finish(typename Base::Iterator& iter, Int idx)
    {
        iter.cache().second_prefix() = prefix_;
        return Base::finish(iter, idx);
    }
};










template <typename Types>
class PrevLeafWalker: public PrevLeafWalkerBase<Types, PrevLeafWalker<Types>> {

protected:

    typedef PrevLeafWalkerBase<Types, PrevLeafWalker<Types>>                    Base;
    typedef typename Base::Key                                                  Key;
    typedef typename Base::Position                                             Position;
    typedef typename Base::Iterator                                             Iterator;

public:

    typedef typename Base::ResultType                                           ResultType;

    PrevLeafWalker(Int stream, Int index): Base(stream, index)
    {}

    template <Int Idx, typename StreamObj>
    ResultType stream(const StreamObj* obj, Int start) {
        return Base::template stream<Idx>(obj, start);
    }

//    template <Int Idx, typename TreeTypes>
//    ResultType stream(const PackedFSEMap<TreeTypes>* map, Int start)
//    {
//      return Base::template array<Idx>(map, start);
//    }

    template <Int Idx, typename TreeTypes>
    ResultType stream(const PackedFSEMarkableMap<TreeTypes>* map, Int start)
    {
        return Base::template array<Idx>(map, start);
    }
};



}
}

#endif
