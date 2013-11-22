
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINERS_DBLMAP2_EDGE_CONTAINER_WALKERS_HPP
#define _MEMORIA_CONTAINERS_DBLMAP2_EDGE_CONTAINER_WALKERS_HPP

#include <memoria/prototypes/bt/bt_tools.hpp>
#include <memoria/core/tools/static_array.hpp>

#include <memoria/core/container/container.hpp>

#include <memoria/prototypes/bt/bt_walkers.hpp>

#include <ostream>

namespace memoria       {
namespace dblmap       {



template <typename Types>
class FindRangeWalkerBase {
protected:
    typedef Iter<typename Types::IterTypes> Iterator;
    typedef Ctr<typename Types::CtrTypes>   Container;

    typedef typename Types::Accumulator     Accumulator;

    WalkDirection direction_;

public:
    FindRangeWalkerBase() {}

    WalkDirection& direction() {
        return direction_;
    }

    void empty(Iterator& iter)
    {

    }
};



template <typename Types>
class FindEndWalker: public FindRangeWalkerBase<Types> {

    typedef FindRangeWalkerBase<Types>      Base;
    typedef typename Base::Iterator         Iterator;
    typedef typename Base::Container        Container;

    typedef typename Types::Accumulator     Accumulator;

    Accumulator prefix_;

public:
    typedef Int ReturnType;

    FindEndWalker(Int stream, const Container&) {}

    template <typename Node>
    ReturnType treeNode(const Node* node, Int start)
    {
        node->template processStream<0>(*this, node->level(), start);

        return node->size(0) - 1;
    }

    template <Int Idx, typename Tree>
    void stream(const Tree* tree, Int level, Int start)
    {
        if (level > 0)
        {
        	tree->sums(0, tree->size() - 1, std::get<Idx>(prefix_));
        }
        else {
        	tree->sums(std::get<Idx>(prefix_));
        }
    }


    void finish(Iterator& iter, Int idx)
    {
        iter.idx() = idx + 1;
        iter.cache().setup(prefix_);
    }
};


template <typename Types>
class FindREndWalker: public FindRangeWalkerBase<Types> {

    typedef FindRangeWalkerBase<Types>      Base;
    typedef typename Base::Iterator         Iterator;
    typedef typename Base::Container        Container;
    typedef typename Types::Accumulator     Accumulator;

public:
    typedef Int ReturnType;

    FindREndWalker(Int stream, const Container&) {}

    template <typename Node>
    ReturnType treeNode(const Node* node, Int start)
    {
        return 0;
    }

    void finish(Iterator& iter, Int idx)
    {
        iter.idx() = idx - 1;
    }
};



template <typename Types>
class FindBeginWalker: public FindRangeWalkerBase<Types> {

    typedef FindRangeWalkerBase<Types>      Base;
    typedef typename Base::Iterator         Iterator;
    typedef typename Base::Container        Container;
    typedef typename Types::Accumulator     Accumulator;

public:
    typedef Int ReturnType;

    FindBeginWalker(Int stream, const Container&) {}


    template <typename Node>
    ReturnType treeNode(const Node* node, Int start)
    {
        return 0;
    }

    void finish(Iterator& iter, Int idx)
    {
        iter.idx() = 0;
    }
};

template <typename Types>
class FindRBeginWalker: public FindRangeWalkerBase<Types> {

    typedef FindRangeWalkerBase<Types>      Base;
    typedef typename Base::Iterator         Iterator;
    typedef typename Base::Container        Container;

    typedef typename Types::Accumulator     Accumulator;

    Accumulator prefix_;

public:
    typedef Int ReturnType;

    FindRBeginWalker(Int stream, const Container&) {}

    template <typename Node>
    ReturnType treeNode(const Node* node, Int start)
    {
        node->process(0, *this);

        return node->size(0) - 1;
    }

    template <Int Idx, typename Tree>
    void stream(const Tree* tree)
    {
        tree->sums(0, tree->size() - 1, std::get<Idx>(prefix_));
    }

    void finish(Iterator& iter, Int idx)
    {
        iter.idx() = idx;
        iter.cache().setup(prefix_);
    }
};



}
}

#endif
