
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef _MEMORIA_CONTAINERS_SEQDENSE_EDGE_WALKERS_HPP
#define _MEMORIA_CONTAINERS_SEQDENSE_EDGE_WALKERS_HPP

#include <memoria/prototypes/bt/bt_walkers.hpp>

#include <memoria/core/packed/sseq/packed_fse_searchable_seq.hpp>

namespace memoria   {
namespace seq_dense {


template <typename Types>
class FindEdgeWalkerBase {
protected:
    typedef Iter<typename Types::IterTypes> Iterator;
    typedef Ctr<typename Types::CtrTypes>   Container;

    typedef typename Types::Accumulator     Accumulator;

    WalkDirection direction_;

public:
    FindEdgeWalkerBase() {}

    WalkDirection& direction() {
        return direction_;
    }

    void empty(Iterator& iter)
    {
//      iter.cache().setup(Accumulator());
    }
};



template <typename Types>
class FindEndWalker: public FindEdgeWalkerBase<Types> {

    typedef FindEdgeWalkerBase<Types>       Base;
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
        if (node->level() > 0)
        {
            VectorAdd(prefix_, node->maxKeys() - node->keysAt(node->children_count() - 1));
        }
        else {
            VectorAdd(prefix_, node->maxKeys());
        }

        return node->children_count() - 1;
    }

    void finish(Iterator& iter, Int idx)
    {
        iter.key_idx() = idx + 1;
        iter.cache().setup(prefix_);
    }
};


template <typename Types>
class FindREndWalker: public FindEdgeWalkerBase<Types> {

    typedef FindEdgeWalkerBase<Types>       Base;
    typedef typename Base::Iterator         Iterator;
    typedef typename Base::Container        Container;
    typedef typename Types::Accumulator     Accumulator;

public:
    typedef Int ReturnType;

    FindREndWalker(Int stream, const Container&) {}

    template <typename Node>
    ReturnType treeNode(const Node* node)
    {
        return 0;
    }

    void finish(Iterator& iter, Int idx)
    {
        iter.key() = idx - 1;

        iter.cache().setup(Accumulator());
    }
};



template <typename Types>
class FindBeginWalker: public FindEdgeWalkerBase<Types> {

    typedef FindEdgeWalkerBase<Types>       Base;
    typedef typename Base::Iterator         Iterator;
    typedef typename Base::Container        Container;
    typedef typename Types::Accumulator     Accumulator;

public:
    typedef Int ReturnType;


    FindBeginWalker(Int stream, const Container&) {}


    template <typename Node>
    ReturnType treeNode(const Node* node, Int)
    {
        return 0;
    }

    void finish(Iterator& iter, Int idx)
    {
        iter.idx() = 0;
    }
};

template <typename Types>
class FindRBeginWalker: public FindEdgeWalkerBase<Types> {

    typedef FindEdgeWalkerBase<Types>       Base;
    typedef typename Base::Iterator         Iterator;
    typedef typename Base::Container        Container;

    typedef typename Types::Accumulator     Accumulator;

    Accumulator prefix_;

public:
    FindRBeginWalker(Int stream, const Container&) {}

    typedef Int ReturnType;



    template <typename Node>
    ReturnType treeNode(const Node* node)
    {
        VectorAdd(prefix_, node->maxKeys() - node->keysAt(node->children_count() - 1));

        return node->children_count() - 1;
    }

    void finish(Iterator& iter, Int idx)
    {
        iter.key_idx() = idx;

        iter.cache().setup(prefix_);
    }
};





}
}

#endif
