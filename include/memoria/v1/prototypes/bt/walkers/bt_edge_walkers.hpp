
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

#include <memoria/v1/core/types.hpp>
#include <memoria/v1/core/container/container.hpp>

namespace memoria {
namespace v1 {
namespace bt1 {

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

    FindBeginWalker(int32_t stream, const Container&) {}


    template <typename Node>
    int32_t treeNode(const Node& node, int32_t start)
    {
        return 0;
    }

    void finish(Iterator& iter, int32_t idx)
    {
        iter.local_pos() = 0;
    }
};





}
}}
