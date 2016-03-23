
// Copyright 2013 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#pragma once

#include <memoria/v1/prototypes/bt/bt_walkers.hpp>

#include <memoria/v1/core/packed/sseq/packed_fse_searchable_seq.hpp>

namespace memoria {
namespace v1 {
namespace louds     {


template <typename Types>
class FindEdgeWalkerBase {
protected:
    typedef Iter<typename Types::IterTypes> Iterator;
    typedef Ctr<typename Types::CtrTypes>   Container;

    typedef typename Types::BranchNodeEntry     BranchNodeEntry;

    WalkDirection direction_;

public:
    FindEdgeWalkerBase() {}

    WalkDirection& direction() {
        return direction_;
    }

    void empty(Iterator& iter)
    {
//      iter.cache().setup(BranchNodeEntry());
    }
};



template <typename Types>
class FindEndWalker: public FindEdgeWalkerBase<Types> {

    typedef FindEdgeWalkerBase<Types>       Base;
    typedef typename Base::Iterator         Iterator;
    typedef typename Base::Container        Container;

    typedef typename Types::BranchNodeEntry     BranchNodeEntry;

    BranchNodeEntry prefix_;

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
    typedef typename Types::BranchNodeEntry     BranchNodeEntry;

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
        iter.key_idx() = idx - 1;

        iter.cache().setup(BranchNodeEntry());
    }
};



template <typename Types>
class FindBeginWalker: public FindEdgeWalkerBase<Types> {

    typedef FindEdgeWalkerBase<Types>       Base;
    typedef typename Base::Iterator         Iterator;
    typedef typename Base::Container        Container;
    typedef typename Types::BranchNodeEntry     BranchNodeEntry;

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

    typedef typename Types::BranchNodeEntry     BranchNodeEntry;

    BranchNodeEntry prefix_;

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
}}