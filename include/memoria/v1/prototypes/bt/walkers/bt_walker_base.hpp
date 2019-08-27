
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

#include <memoria/v1/prototypes/bt/nodes/leaf_node.hpp>
#include <memoria/v1/prototypes/bt/nodes/branch_node.hpp>

#include <memoria/v1/core/container/container.hpp>

#include <memoria/v1/core/types/list/list_tree.hpp>
#include <memoria/v1/prototypes/bt/walkers/bt_walker_base_detail.hpp>

#include <memoria/v1/core/types/algo/for_each.hpp>

#include <tuple>

namespace memoria {
namespace v1 {

namespace bt {

template <typename Types, typename LeafPath_>
struct WalkerTypes: Types {
    using LeafPath = LeafPath_;
};


class StreamOpResult {
    int32_t idx_;
    int32_t start_;
    bool out_of_range_;
    bool empty_;

public:
    StreamOpResult(int32_t idx, int32_t start, bool out_of_range, bool empty = false): idx_(idx), start_(start), out_of_range_(out_of_range), empty_(empty) {}

    int32_t local_pos() const {
        return idx_;
    }

    int32_t start() const {
        return start_;
    }

    bool out_of_range(){
        return out_of_range_;
    }

    bool empty(){
        return empty_;
    }
};



template <
    typename Types,
    typename MyType
>
class WalkerBase {
public:
    typedef Iter<typename Types::IterTypes>                                     Iterator;
    typedef typename Types::IteratorBranchNodeEntry                             IteratorBranchNodeEntry;
    typedef typename Types::LeafStreamsStructList                               LeafStructList;
    typedef typename Types::LeafRangeList                                       LeafRangeList;
    typedef typename Types::LeafRangeOffsetList                                 LeafRangeOffsetList;



    static const int32_t Streams                                                    = Types::Streams;

    using Position = typename Types::Position;
    using LeafPath = typename Types::LeafPath;

    static const int32_t Stream = ListHead<LeafPath>::Value;

protected:

    int32_t leaf_index_ = 0;

    int32_t idx_backup_ = 0;
    Position branch_size_prefix_backup_;

    Position branch_size_prefix_;
    IteratorBranchNodeEntry branch_prefix_;
    IteratorBranchNodeEntry leaf_prefix_;

    bool compute_branch_    = true;
    bool compute_leaf_      = true;

    WalkDirection direction_;

public:

    template <typename LeafPath>
    using AccumItemH = typename Types::template AccumItemH<LeafPath>;

protected:

    struct FindBranchFn {
        MyType& walker_;

        FindBranchFn(MyType& walker): walker_(walker) {}

        template <int32_t ListIdx, typename StreamType, typename... Args>
        StreamOpResult stream(StreamType&& stream, bool root, int32_t index, int32_t start, Args&&... args)
        {
            StreamOpResult result = walker_.template find_non_leaf<ListIdx>(stream, root, index, start, std::forward<Args>(args)...);

            // TODO: should we also forward args... to this call?
            walker_.template postProcessBranchStream<ListIdx>(stream, start, result.local_pos());

            return result;
        }
    };


    struct FindLeafFn {
        MyType& walker_;

        FindLeafFn(MyType& walker): walker_(walker) {}

        template <int32_t ListIdx, typename StreamType, typename... Args>
        StreamOpResult stream(StreamType&& stream, int32_t start, Args&&... args)
        {
            StreamOpResult result = walker_.template find_leaf<ListIdx>(stream, start, std::forward<Args>(args)...);

            // TODO: should we also forward args... to this call?
            walker_.template postProcessLeafStream<ListIdx>(
                std::forward<StreamType>(stream), start, result.local_pos()
            );

            return result;
        }
    };

    struct ProcessBranchCmdFn {
        MyType& walker_;

        ProcessBranchCmdFn(MyType& walker): walker_(walker) {}

        template <int32_t ListIdx, typename StreamType, typename... Args>
        void stream(StreamType&& stream, WalkCmd cmd, Args&&... args)
        {
            walker_.template process_branch_cmd<ListIdx>(
                std::forward<StreamType>(stream), cmd, std::forward<Args>(args)...
            );
        }
    };

    struct ProcessLeafCmdFn {
        MyType& walker_;

        ProcessLeafCmdFn(MyType& walker): walker_(walker) {}

        template <int32_t ListIdx, typename StreamType, typename... Args>
        void stream(StreamType&& stream, WalkCmd cmd, Args&&... args)
        {
            walker_.template process_leaf_cmd<ListIdx>(
                        std::forward<StreamType>(stream), cmd, std::forward<Args>(args)...
            );
        }
    };


public:

    WalkerBase(int32_t leaf_index):
        leaf_index_(leaf_index)
    {}

    void result() const {}


    void empty(Iterator& iter)
    {
        iter.local_pos() = 0;
    }

    static constexpr int32_t current_stream() {
        return ListHead<LeafPath>::Value;
    }

    int32_t leaf_index() const
    {
        return leaf_index_;
    }

    void set_leaf_index(int32_t value)
    {
        leaf_index_ = value;
    }

    const bool& compute_branch() const {
        return compute_branch_;
    }

    bool& compute_branch() {
        return compute_branch_;
    }

    const bool& compute_leaf() const {
        return compute_leaf_;
    }

    bool& compute_leaf() {
        return compute_leaf_;
    }

    static constexpr int32_t branchIndex(int32_t leaf_index)
    {
        return bt::LeafToBranchIndexTranslator<LeafStructList, LeafPath, 0>::BranchIndex + leaf_index;
    }


    template <typename LeafPath>
    auto branch_index(int32_t index)
    {
        return AccumItemH<LeafPath>::value(index, branch_prefix_);
    }

    template <typename LeafPath>
    auto branch_index(int32_t index) const
    {
        return AccumItemH<LeafPath>::cvalue(index, branch_prefix_);
    }

    template <typename LeafPath>
    auto leaf_index(int32_t index)
    {
        return AccumItemH<LeafPath>::value(index, leaf_prefix_);
    }

    template <typename LeafPath>
    auto leaf_index(int32_t index) const
    {
        return AccumItemH<LeafPath>::cvalue(index, leaf_prefix_);
    }


    template <typename LeafPath>
    auto total_index(int32_t index) const
    {
        return AccumItemH<LeafPath>::cvalue(index, branch_prefix_) +
                AccumItemH<LeafPath>::cvalue(index, leaf_prefix_);
    }

    void prepare(Iterator& iter)
    {
        branch_prefix_      = iter.cache().prefixes();
        leaf_prefix_        = iter.cache().leaf_prefixes();
        branch_size_prefix_ = iter.cache().size_prefix();

        branch_size_prefix_backup_  = iter.cache().size_prefix();
        idx_backup_         = iter.local_pos();
    }

    void finish(Iterator& iter, int32_t idx, WalkCmd cmd) const
    {
        iter.local_pos() = idx;

        iter.finish_walking(idx, self(), cmd);

        iter.cache().prefixes()      = branch_prefix_;
        iter.cache().leaf_prefixes() = leaf_prefix_;
        iter.cache().size_prefix()   = branch_size_prefix_;
    }

    const IteratorBranchNodeEntry& branch_BranchNodeEntry() const {
        return branch_prefix_;
    }

    IteratorBranchNodeEntry& branch_BranchNodeEntry() {
        return branch_prefix_;
    }

    const IteratorBranchNodeEntry& leaf_BranchNodeEntry() const {
        return leaf_prefix_;
    }

    IteratorBranchNodeEntry& leaf_BranchNodeEntry() {
        return leaf_prefix_;
    }

    const Position& branch_size_prefix() const {
        return branch_size_prefix_;
    }

    Position& branch_size_prefix() {
        return branch_size_prefix_;
    }

    const Position& branch_size_prefix_backup() const {
        return branch_size_prefix_backup_;
    }

    int32_t idx_backup() const {
        return idx_backup_;
    }

    MyType& self() {return *T2T<MyType*>(this);}
    const MyType& self() const {return *T2T<const MyType*>(this);}

    template <typename CtrT, typename NodeT>
    StreamOpResult treeNode(const BranchNodeSO<CtrT, NodeT>& node, WalkDirection direction, int32_t start)
    {
        auto& self = this->self();

        this->direction_ = direction;

        int32_t index = node.template translateLeafIndexToBranchIndex<LeafPath>(self.leaf_index());

        using BranchPath = typename BranchNodeSO<CtrT, NodeT>::template BuildBranchPath<LeafPath>;
        auto result = node.template processStream<BranchPath>(
                FindBranchFn(self), node.node()->is_root(), index, start
        );

        self.postProcessBranchNode(node, direction, start, result);

        return result;
    }

    template <typename CtrT, typename NodeT>
    StreamOpResult treeNode(const LeafNodeSO<CtrT, NodeT>& node, WalkDirection direction, int32_t start)
    {
        this->direction_ = direction;

        auto& self = this->self();
        auto result = node.template processStream<LeafPath>(FindLeafFn(self), start);

        self.postProcessLeafNode(node, direction, start, result);

        return result;
    }


    template <typename Node, typename... Args>
    void processCmd(const Node* node, WalkCmd cmd, Args&&... args){}

    template <typename CtrT, typename NodeT, typename... Args>
    void processCmd(const BranchNodeSO<CtrT, NodeT>& node, WalkCmd cmd, Args&&... args)
    {
        auto& self = this->self();

        int32_t index = node.template translateLeafIndexToBranchIndex<LeafPath>(self.leaf_index());

        using BranchPath = typename BranchNodeSO<CtrT, NodeT>::template BuildBranchPath<LeafPath>;

        return node.template processStream<BranchPath>(
                    ProcessBranchCmdFn(self), cmd, index, std::forward<Args>(args)...
        );
    }


    template <typename Node, typename... Args>
    void processLeafIteratorBranchNodeEntry(Node& node, IteratorBranchNodeEntry&accum, Args&&... args)
    {
        _::LeafAccumWalker<
            LeafStructList,
            LeafRangeList,
            LeafRangeOffsetList,
            Node::template StreamStartIdx<
                ListHead<LeafPath>::Value
            >::Value
        > w;

        Node::template StreamDispatcher<
            ListHead<LeafPath>::Value
        >
        ::dispatchAll(node.allocator(), w, self(), accum, std::forward<Args>(args)...);
    }


    struct ProcessBranchIteratorBranchNodeEntryWithLeaf
    {
        template <int32_t StreamIdx, typename Node, typename Walker, typename Accum, typename... Args>
        static bool process(Node& node, Walker&& walker, Accum&& accum, Args&&... args)
        {
            _::LeafAccumWalker<
                LeafStructList,
                LeafRangeList,
                LeafRangeOffsetList,
                Node::template StreamStartIdx<
                    StreamIdx
                >::Value
            > w;

            using SD = typename Node::template StreamDispatcher<
                StreamIdx
            >;

            SD(node.state()).dispatchAll(node.allocator(), w, walker, accum, std::forward<Args>(args)...);

            return true;
        }
    };


    template <typename Node, typename... Args>
    void processBranchIteratorBranchNodeEntryWithLeaf(Node& node, IteratorBranchNodeEntry&accum, Args&&... args)
    {
        ForEach<0, Streams>::process(ProcessBranchIteratorBranchNodeEntryWithLeaf(), node, self(), accum, std::forward<Args>(args)...);
    }


    template <typename Node, typename... Args>
    void processBranchIteratorBranchNodeEntry(Node& node, Args&&... args)
    {
        using ItrAccList = list_tree::MakeValueList<int32_t, 0, Node::Streams>;

        _::IteratorStreamRangesListWalker<
            ItrAccList
        >::
        process(self(), node, branch_BranchNodeEntry(), std::forward<Args>(args)...);
    }

    struct BranchSizePrefix
    {
        template <int32_t GroupIdx, int32_t AllocatorIdx, int32_t ListIdx, typename StreamObj, typename... Args>
        void stream(StreamObj&& obj, MyType& walker, Args&&... args)
        {
            walker.template branch_size_prefix<ListIdx>(obj, std::forward<Args>(args)...);
        }
    };


    struct LeafSizePrefix
    {
        template <int32_t GroupIdx, int32_t AllocatorIdx, int32_t ListIdx, typename StreamObj, typename... Args>
        void stream(StreamObj&& obj, MyType& walker, Args&&... args)
        {
            walker.template leaf_size_prefix<ListIdx>(obj, std::forward<Args>(args)...);
        }
    };



    template <typename Node, typename... Args>
    void processBranchSizePrefix(Node& node, Args&&... args)
    {
        node.processStreamsStart(BranchSizePrefix(), self(), std::forward<Args>(args)...);
    }

    template <typename Node, typename... Args>
    void processLeafSizePrefix(Node& node, Args&&... args)
    {
        node.processStreamsStart(LeafSizePrefix(), self(), std::forward<Args>(args)...);
    }

    template <int32_t StreamIdx, typename StreamType>
    void postProcessBranchStream(StreamType&&, int32_t, int32_t) {}

    template <int32_t StreamIdx, typename StreamType>
    void postProcessLeafStream(StreamType&&, int32_t, int32_t) {}

    template <typename Node, typename Result>
    void postProcessBranchNode(Node& node, WalkDirection direction, int32_t start, Result&&) {}

    template <typename Node, typename Result>
    void postProcessLeafNode(Node& node, WalkDirection direction, int32_t start, Result&&) {}
};




}
}}
