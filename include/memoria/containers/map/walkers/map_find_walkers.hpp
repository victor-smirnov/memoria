
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINERS_MAP_FIND_WALKERS_HPP
#define _MEMORIA_CONTAINERS_MAP_FIND_WALKERS_HPP

#include <memoria/core/tools/static_array.hpp>
#include <memoria/prototypes/bt/bt_walkers.hpp>

#include <ostream>

namespace memoria       {
namespace map           {


template <typename Types>
class FindWalkerBase {
protected:
    typedef typename Types::Key                                                 Key;
    typedef typename Types::Accumulator                                         Accumulator;

    typedef Iter<typename Types::IterTypes>                                     Iterator;

    Key prefix_ = 0;

    Key key_;
    Int key_num_;
    Int stream_;

    WalkDirection direction_;



public:
    FindWalkerBase(Int stream, Int key_num, Key key):
        key_(key), key_num_(key_num), stream_(stream)
    {}

    const WalkDirection& direction() const {
        return direction_;
    }

    WalkDirection& direction() {
        return direction_;
    }


    void finish(Iterator& iter, Int idx)
    {
        iter.idx()  = idx;

        iter.cache().setup(prefix_);
    }

    void empty(Iterator& iter)
    {
        iter.idx()  = 0;

        iter.cache().setup(0);
    }
};


template <typename Types>
class FindLTWalker: public FindWalkerBase<Types> {

    typedef FindWalkerBase<Types>       Base;
    typedef typename Base::Key          Key;

public:
    typedef Int ReturnType;

    FindLTWalker(Int stream, Int key_num, Int key): Base(stream, key_num, key)
    {}

    template <typename Node>
    ReturnType treeNode(const Node* node, Int start)
    {
        const typename Node::Map& map = node->map();

        return map.findLTS(Base::key_num_, Base::key_ - Base::prefix_[Base::key_num_], Base::prefix_);
    }
};

template <typename Types>
class FindLEWalker: public FindWalkerBase<Types> {

    typedef FindWalkerBase<Types>       Base;
    typedef typename Base::Key          Key;

public:
    FindLEWalker(Int stream, Int key_num, Key key): Base(stream, key_num, key)
    {}

    typedef Int ResultType;
    typedef Int ReturnType;

    template <typename Node>
    ReturnType treeNode(const Node* node, Int start)
    {
        return node->find(Base::stream_, *this, node->level(), start);
    }

    template <Int Idx, typename Tree>
    Int stream(const Tree* tree, Int level, Int start)
    {
        auto& key       = Base::key_;
        auto& prefix    = Base::prefix_;

        auto target     = key - prefix;

        auto result     = tree->findLEForward(0, 0, target);

        prefix += result.prefix();

        return result.idx();
    }
};



}
}

#endif
