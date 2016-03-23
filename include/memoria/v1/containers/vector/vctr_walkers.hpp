
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

#include <memoria/v1/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/v1/core/tools/static_array.hpp>

#include <memoria/v1/core/container/container.hpp>

#include <memoria/v1/prototypes/bt/bt_walkers.hpp>

#include <memoria/v1/prototypes/bt/nodes/leaf_node.hpp>

#include <ostream>

namespace memoria {
namespace v1 {
namespace mvector       {


template <typename Types>
class FindWalkerBase {
protected:
    typedef typename Types::Key                                                 Key;
    typedef typename Types::BranchNodeEntry                                         BranchNodeEntry;

    typedef Iter<typename Types::IterTypes>                                     Iterator;

    BigInt prefix_ = 0;

    Key key_;

    WalkDirection direction_;

public:

    FindWalkerBase(Key key):
        key_(key)
    {}

    const WalkDirection& direction() const {
        return direction_;
    }

    WalkDirection& direction() {
        return direction_;
    }


    void finish(Iterator& iter, Int idx)
    {
        iter.key_idx()  = idx;

        Int size = iter.size();

        if (idx < size)
        {
            iter.cache().setup(prefix_);
        }
        else {
            iter.cache().setup(prefix_ - size);
        }
    }

    void empty(Iterator& iter)
    {
        iter.key_idx()  = 0;

        iter.cache().setup(0);
    }

    BigInt prefix() const {
        return prefix_;
    }
};





template <typename Types>
class FindGEWalker: public FindWalkerBase<Types> {

    typedef FindWalkerBase<Types>       Base;
    typedef typename Base::Key          Key;

public:
    FindGEWalker(Key key, Int key_num): Base(key, key_num)
    {}

    template <typename Node>
    void treeNode(const Node* node)
    {
        Base::idx_ = node->findLES(Base::key_num_, Base::key_ - std::get<0>(Base::prefix_)[Base::key_num_], Base::prefix_);

        if (node->level() != 0 && Base::idx_ == node->children_count())
        {
            VectorSub(Base::prefix_, node->keysAt(node->children_count() - 1));
            Base::idx_--;
        }
    }
};







template <typename Types>
class FindRangeWalkerBase {
protected:
    typedef Iter<typename Types::IterTypes> Iterator;
    typedef Ctr<typename Types::CtrTypes>   Container;

    typedef typename Types::BranchNodeEntry     BranchNodeEntry;

    WalkDirection direction_;

public:
    FindRangeWalkerBase() {}

    WalkDirection& direction() {
        return direction_;
    }

    void empty(Iterator& iter)
    {
        iter.cache().setup(BranchNodeEntry());
    }
};



template <typename Types>
class FindBeginWalker: public FindRangeWalkerBase<Types> {

    typedef FindRangeWalkerBase<Types>      Base;
    typedef typename Base::Iterator         Iterator;
    typedef typename Base::Container        Container;
    typedef typename Types::BranchNodeEntry     BranchNodeEntry;

public:
    typedef Int ReturnType;


    FindBeginWalker(Int stream, const Container&) {}


    template <typename Node>
    ReturnType treeNode(const Node* node)
    {
        return 0;
    }

    void finish(Iterator& iter, Int idx)
    {
        iter.idx() = 0;

        iter.cache().setup(BranchNodeEntry());
    }
};



}
}}