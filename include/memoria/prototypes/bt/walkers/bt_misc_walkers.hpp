
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

#include <memoria/prototypes/bt/walkers/bt_find_walkers.hpp>
#include <memoria/core/packed/tools/packed_tools.hpp>

#include <memoria/core/memory/ptr_cast.hpp>

namespace memoria {
namespace bt {

template <typename MyType, typename BranchPath, typename LeafPath>
class LeveledNodeWalkerBase {

    struct LeafStreamFn {

        MyType& walker_;

        LeafStreamFn(MyType& walker): walker_(walker) {}

        template <int32_t StreamIdx, typename Stream, typename... Args>
        auto stream(const Stream& stream, Args&&... args)
        {
            return walker_.template leafStream<StreamIdx>(stream, args...);
        }
    };


    struct NonLeafStreamFn {
        MyType& walker_;

        NonLeafStreamFn(MyType& walker): walker_(walker) {}

        template <int32_t StreamIdx, typename Stream, typename... Args>
        auto stream(const Stream& stream, Args&&... args)
        {
            return walker_.template nonLeafStream<StreamIdx>(stream, args...);
        }
    };

public:

    template <typename NodeTypes, typename... Args>
    auto treeNode(const bt::LeafNode<NodeTypes>& node, Args&&... args)
    {
        return node->template processStream<LeafPath>(LeafStreamFn(self()), args...);
    }

    template <typename NodeTypes, typename... Args>
    auto treeNode(const bt::BranchNode<NodeTypes>& node, Args&&... args)
    {
        return node->template processStream<BranchPath>(NonLeafStreamFn(self()), args...);
    }

    template <typename NodeTypes, typename... Args>
    auto treeNode(bt::LeafNode<NodeTypes>& node, Args&&... args)
    {
        return node->template processStream<LeafPath>(LeafStreamFn(self()), args...);
    }

    template <typename NodeTypes, typename... Args>
    auto treeNode(bt::BranchNode<NodeTypes>& node, Args&&... args)
    {
        return node->template processStream<BranchPath>(NonLeafStreamFn(self()), args...);
    }

    MyType& self() {return *ptr_cast<MyType>(this);}
    const MyType& self() const {return *ptr_cast<const MyType>(this);}
};




template <typename MyType, typename BranchPath, typename LeafPath>
struct NodeWalkerBase {
    template <typename CtrT, typename NodeT, typename... Args>
    auto treeNode(LeafNodeSO<CtrT, NodeT>& node, Args&&... args)
    {
        return node.template processStream<LeafPath>(self(), std::forward<Args>(args)...);
    }

    template <typename CtrT, typename NodeT, typename... Args>
    auto treeNode(BranchNodeSO<CtrT, NodeT>& node, Args&&... args)
    {
        return node.template processStream<BranchPath>(self(), std::forward<Args>(args)...);
    }

    template <typename CtrT, typename NodeT, typename... Args>
    auto treeNode(const LeafNodeSO<CtrT, NodeT>& node, Args&&... args)
    {
        return node.template processStream<LeafPath>(self(), std::forward<Args>(args)...);
    }

    template <typename CtrT, typename NodeT, typename... Args>
    auto treeNode(const BranchNodeSO<CtrT, NodeT>& node, Args&&... args)
    {
        return node.template processStream<BranchPath>(self(), std::forward<Args>(args)...);
    }

    MyType& self() {return *ptr_cast<MyType>(this);}
    const MyType& self() const {return *ptr_cast<const MyType>(this);}
};



template <int32_t Stream, typename SubstreamsIdxList>
struct SubstreamsSetNodeFn {
    template <typename CtrT, typename NodeT, typename... Args>
    auto treeNode(LeafNodeSO<CtrT, NodeT>& node, Args&&... args)
    {
        return node.template processSubstreamsByIdx<Stream, SubstreamsIdxList>(std::forward<Args>(args)...);
    }


    template <typename CtrT, typename NodeT, typename... Args>
    auto treeNode(const LeafNodeSO<CtrT, NodeT>& node, Args&&... args)
    {
        return node.template processSubstreamsByIdx<Stream, SubstreamsIdxList>(std::forward<Args>(args)...);
    }
};


template <int32_t Stream>
struct StreamNodeFn {
    template <typename CtrT, typename NodeT, typename... Args>
    auto treeNode(LeafNodeSO<CtrT, NodeT>& node, Args&&... args)
    {
        return node.template processStream<IntList<Stream>>(std::forward<Args>(args)...);
    }


    template <typename CtrT, typename NodeT, typename... Args>
    auto treeNode(const LeafNodeSO<CtrT, NodeT>& node, Args&&... args)
    {
        return node.template processStream<IntList<Stream>>(std::forward<Args>(args)...);
    }
};



struct GetLeafValuesFn {
    template <typename StreamType, typename... Args>
    auto stream(const StreamType& obj, Args&&... args)
    {
        return obj.get_values(std::forward<Args>(args)...);
    }
};



struct SetLeafValuesFn {
    template <typename StreamType, typename... Args>
    auto stream(StreamType& obj, Args&&... args)
    {
        return obj.set_values(std::forward<Args>(args)...);
    }
};

}}
