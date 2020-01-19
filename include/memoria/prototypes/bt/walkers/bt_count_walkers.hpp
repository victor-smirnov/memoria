
// Copyright 2016 Victor Smirnov
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

#include <memoria/prototypes/bt/nodes/leaf_node.hpp>
#include <memoria/prototypes/bt/nodes/branch_node.hpp>

#include <memoria/prototypes/bt/walkers/bt_find_walkers.hpp>
#include <memoria/core/tools/assert.hpp>

namespace memoria {
namespace bt {


/**********************************************************************/

template <
    typename Types,
    typename MyType
>
class CountForwardWalkerBase: public FindForwardWalkerBase<Types, MyType> {
protected:
    using Base       = FindForwardWalkerBase<Types, MyType>;
    using CtrSizeT   = typename Types::CtrSizeT;

    using typename Base::LeafPath;

    using Base::sum_;
    using Base::direction_;



public:

    using Base::leaf_index;
    using Base::treeNode;

    CountForwardWalkerBase():
        Base(-1, 0, SearchType::GE)
    {}

    struct FindBranchFn {
        MyType& walker_;

        FindBranchFn(MyType& walker): walker_(walker) {}

        template <int32_t ListIdx, typename StreamType, typename... Args>
        StreamOpResult stream(const StreamType* stream, bool root, int32_t index, int32_t start, Args&&... args)
        {
            StreamOpResult result = walker_.template find_non_leaf<ListIdx>(stream, root, index, start, std::forward<Args>(args)...);

            // TODO: should we also forward args... to this call?
            walker_.template postProcessBranchStream<ListIdx>(stream, start, result.local_pos());

            return result;
        }
    };

    MyType& self() {return *T2T<MyType*>(this);}
    const MyType& self() const {return *T2T<const MyType*>(this);}

    template <typename NodeTypes>
    StreamOpResult treeNode(const bt::BranchNode<NodeTypes>* node, WalkDirection direction, int32_t start)
    {
        auto& self = this->self();

        direction_ = direction;

        int32_t index = node->template translateLeafIndexToBranchIndex<LeafPath>(self.leaf_index());

        using BranchPath     = typename bt::BranchNode<NodeTypes>::template BuildBranchPath<LeafPath>;
        using BranchSizePath = IntList<ListHead<LeafPath>::Value>;

        auto sizes_substream = node->template substream<BranchSizePath>();

        auto result = node->template processStream<BranchPath>(FindBranchFn(self), node->is_root(), index, start, sizes_substream);

        self.postProcessBranchNode(node, direction, start, result);

        return result;
    }



    template <int32_t StreamIdx, typename Tree, typename SizesSubstream>
    StreamOpResult find_non_leaf(const Tree& tree, bool root, int32_t index, int32_t start, const SizesSubstream& sizes_substream)
    {
        auto size = tree->size();

        if (start < size)
        {
            MEMORIA_V1_ASSERT(tree.size(), ==, sizes_substream.size());

            auto tree_iter  = tree.iterator(index, start);
            auto sizes_iter = sizes_substream.iterator(0, start);

            for (int32_t c = start; c < size; c++)
            {
                sizes_iter.next();
                tree_iter.next();

                if (tree_iter.value() != sizes_iter.value())
                {
                    return StreamOpResult(c, start, false, false);
                }
                else {
                    sum_ += sizes_iter.value();
                }
            }

            return StreamOpResult(size, start, true, false);
        }
        else {
            return StreamOpResult(size, start, true, true);
        }
    }


    template <int32_t StreamIdx, typename Seq>
    StreamOpResult find_leaf(const Seq& seq, int32_t start)
    {
        MEMORIA_V1_ASSERT_TRUE(seq);

        auto count = seq->countFW(start);

        if (direction_ == WalkDirection::UP)
        {
            sum_ += count.count();

            this->set_leaf_index(count.symbol());

            int32_t end = start + count.count();

            return StreamOpResult(end, start, end >= seq->size());
        }
        else {
            if (count.symbol() == leaf_index())
            {
                sum_ += count.count();

                int32_t end = start + count.count();

                return StreamOpResult(end, start, end >= seq->size());
            }
            else {
                return StreamOpResult(start, start, false);
            }
        }

    }
};




template <
    typename Types
>
class CountForwardWalker: public CountForwardWalkerBase<Types, CountForwardWalker<Types>> {

    using Base = CountForwardWalkerBase<Types, CountForwardWalker<Types>>;

public:
    CountForwardWalker(): Base() {}
};





template <
    typename Types,
    typename MyType
>
class CountBackwardWalkerBase: public FindBackwardWalkerBase<Types, MyType> {
protected:
    using Base       = FindBackwardWalkerBase<Types, MyType>;
    using CtrSizeT   = typename Types::CtrSizeT;

    using typename Base::LeafPath;

    using Base::sum_;
    using Base::direction_;



public:

    using Base::leaf_index;
    using Base::treeNode;

    CountBackwardWalkerBase():
        Base(-1, 0, SearchType::GE)
    {}

    struct FindBranchFn {
        MyType& walker_;

        FindBranchFn(MyType& walker): walker_(walker) {}

        template <int32_t ListIdx, typename StreamType, typename... Args>
        StreamOpResult stream(const StreamType& stream, bool root, int32_t index, int32_t start, Args&&... args)
        {
            StreamOpResult result = walker_.template find_non_leaf<ListIdx>(stream, root, index, start, std::forward<Args>(args)...);

            walker_.template postProcessBranchStream<ListIdx>(stream, start, result.local_pos());

            return result;
        }
    };

    MyType& self() {return *T2T<MyType*>(this);}
    const MyType& self() const {return *T2T<const MyType*>(this);}

    template <typename CtrT, typename NodeT>
    StreamOpResult treeNode(const BranchNodeSO<CtrT, NodeT>& node, WalkDirection direction, int32_t start)
    {
        auto& self = this->self();

        direction_ = direction;

        int32_t index = node.template translateLeafIndexToBranchIndex<LeafPath>(self.leaf_index());

        using BranchPath     = typename BranchNodeSO<CtrT, NodeT>::template BuildBranchPath<LeafPath>;
        using BranchSizePath = IntList<ListHead<LeafPath>::Value>;

        auto sizes_substream = node.template substream<BranchSizePath>();

        auto result = node.template processStream<BranchPath>(FindBranchFn(self), node->is_root(), index, start, sizes_substream);

        self.postProcessBranchNode(node, direction, start, result);

        return result;
    }



    template <int32_t StreamIdx, typename Tree, typename SizesSubstream>
    StreamOpResult find_non_leaf(const Tree& tree, bool root, int32_t index, int32_t start, const SizesSubstream& sizes_substream)
    {
        if (start > tree.size()) start = tree.size() - 1;

        if (start >= 0)
        {
            MEMORIA_V1_ASSERT(tree.size(), ==, sizes_substream.size());

            for (int32_t c = start; c >= 0; c--)
            {
                auto v1 = tree.get_values(c, index);
                auto v2 = sizes_substream.get_values(c, 0);

                if (v1 != v2)
                {
                    return StreamOpResult(c, start, false, false);
                }
                else {
                    sum_ += v1;
                }
            }

            return StreamOpResult(-1, start, true, false);
        }
        else {
            return StreamOpResult(start, start, true, true);
        }
    }


    template <int32_t StreamIdx, typename Seq>
    StreamOpResult find_leaf(const Seq& seq, int32_t start)
    {
        MEMORIA_V1_ASSERT_TRUE(seq);

        if (direction_ == WalkDirection::UP)
        {
            auto count = seq.countBW(start);

            sum_ += count.count();

            this->set_leaf_index(count.symbol());

            int32_t end = start - count.count();

            return StreamOpResult(end, start, end < 0);
        }
        else {
            start = seq.size() - 1;
            auto count = seq.countBW(start);

            if (count.symbol() == leaf_index())
            {
                sum_ += count.count();

                int32_t end = start - count.count();

                return StreamOpResult(end, start, end < 0);
            }
            else {
                return StreamOpResult(start, start, false);
            }
        }
    }
};




template <
    typename Types
>
class CountBackwardWalker: public CountBackwardWalkerBase<Types, CountBackwardWalker<Types>> {

    using Base = CountBackwardWalkerBase<Types, CountBackwardWalker<Types>>;

public:
    CountBackwardWalker(): Base() {}
};





}}
