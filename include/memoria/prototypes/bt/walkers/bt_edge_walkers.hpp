
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_EDGE_WALKERS_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_EDGE_WALKERS_HPP

namespace memoria {
namespace bt1     {

template <typename Types>
class FindEdgeWalkerBase {
protected:
    typedef Iter<typename Types::IterTypes> Iterator;
    typedef Ctr<typename Types::CtrTypes>   Container;

    WalkDirection direction_;

public:
    FindEdgeWalkerBase() {}

    WalkDirection& direction() {
        return direction_;
    }

    void empty(Iterator& iter)
    {

    }
};



template <typename Types>
class FindBeginWalker: public FindEdgeWalkerBase<Types> {

    typedef FindEdgeWalkerBase<Types>       Base;
    typedef typename Base::Iterator         Iterator;
    typedef typename Base::Container        Container;

public:

    FindBeginWalker(Int stream, const Container&) {}


    template <typename Node>
    Int treeNode(const Node* node, Int start)
    {
        return 0;
    }

    void finish(Iterator& iter, Int idx)
    {
        iter.idx() = 0;
    }
};





}
}

#endif
