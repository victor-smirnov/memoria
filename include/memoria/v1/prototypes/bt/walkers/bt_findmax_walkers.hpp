
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

#include <memoria/v1/prototypes/bt/walkers/bt_walker_base.hpp>
#include <memoria/v1/prototypes/bt/walkers/bt_find_walkers.hpp>

#include <memoria/v1/core/tools/assert.hpp>

namespace memoria {
namespace v1 {
namespace bt {

/***********************************************************************/

template <typename Types, typename MyType>
class FindMaxWalkerBaseBase: public WalkerBase<Types, MyType> {

public:
    using Base          = WalkerBase<Types, MyType>;

    using LeafPath      = typename Types::LeafPath;

    using TargetType    = typename Types::template TargetType<LeafPath>;

    using ThisType = FindMaxWalkerBaseBase<Types, MyType>;
    using Iterator = typename Base::Iterator;

protected:

    const TargetType& target_;

    SearchType search_type_ = SearchType::GT;

public:

    FindMaxWalkerBaseBase(int32_t leaf_index, const TargetType& target, SearchType search_type):
        Base(leaf_index), target_(target), search_type_(search_type)
    {}

    const SearchType& search_type() const {
        return search_type_;
    }

    const TargetType& target() const {
        return target_;
    }
};


template <typename Types, typename MyType>
class FindMaxWalkerBase: public FindMaxWalkerBaseBase<Types, MyType> {
    using Base          = FindMaxWalkerBaseBase<Types, MyType>;

public:

    using TargetType    = typename Base::TargetType;
    using Position      = typename Base::Position;
    using LeafPath       = typename Base::LeafPath;

    FindMaxWalkerBase(int32_t leaf_index, const TargetType& target, SearchType search_type):
        Base(leaf_index, target, search_type)
    {}

    using Base::treeNode;

//    template <typename... Args>
//    auto treeNode(Args&&... args) -> decltype(Base::treeNode(std::forward<Args>(args)...))
//    {
//        return Base::treeNode(std::forward<Args>(args)...);
//    }

    template <typename CtrT, typename NodeT>
    void treeNode(const BranchNodeSO<CtrT, NodeT>& node, WalkCmd cmd, int32_t start, int32_t end)
    {
        auto& self = this->self();

        if (cmd == WalkCmd::FIX_TARGET)
        {
            self.processCmd(node, cmd, start, end);
            self.processBranchSizePrefix(node, start, end, FixTargetTag());
        }
        else if (cmd == WalkCmd::PREFIXES)
        {
            self.processBranchIteratorBranchNodeEntry(node, start, end);
            self.processBranchSizePrefix(node, start, end);
        }
    }


    template <typename CtrT, typename NodeT>
    void treeNode(const LeafNodeSO<CtrT, NodeT>& node, WalkCmd cmd, int32_t start, int32_t end)
    {
        auto& self = this->self();

        if (this->compute_leaf_)
        {
            if (cmd == WalkCmd::THE_ONLY_LEAF)
            {
//              self.processLeafIteratorBranchNodeEntry(node, this->leaf_BranchNodeEntry(), start, end);
            }
            else if (cmd == WalkCmd::FIRST_LEAF)
            {
                // FIXME: is this call necessary here?
//              self.processLeafIteratorBranchNodeEntry(node, this->leaf_BranchNodeEntry(), start, end);
                self.processBranchIteratorBranchNodeEntryWithLeaf(node, this->branch_BranchNodeEntry());

                self.processLeafSizePrefix(node);
            }
            else if (cmd == WalkCmd::LAST_LEAF) {
//              self.processLeafIteratorBranchNodeEntry(node, this->leaf_BranchNodeEntry(), start, end);
            }
            else {
                // throw exception ?
            }
        }
    }



    template <int32_t StreamIdx, typename Tree>
    StreamOpResult find_non_leaf(const Tree& tree, bool root, int32_t index, int32_t start)
    {
        auto size = tree->size();

        if (start < size)
        {
            MEMORIA_V1_ASSERT(start, ==, 0);

            auto result = tree.findForward(Base::search_type_, index, Base::target_);

            return StreamOpResult(result.local_pos(), start, result.local_pos() >= size, false);
        }
        else {
            return StreamOpResult(size, start, true, true);
        }
    }


    template <int32_t StreamIdx, typename Tree>
    void process_branch_cmd(Tree&& tree, WalkCmd cmd, int32_t index, int32_t start, int32_t end)
    {
        if (cmd == WalkCmd::FIX_TARGET)
        {
//          Base::sum_ -= tree->value(index, end);
        }
    }


    template <int32_t StreamIdx, typename Tree>
    StreamOpResult find_leaf(const Tree& tree, int32_t start)
    {
        if (tree)
        {
            if (start < tree.size())
            {
                MEMORIA_V1_ASSERT(start, ==, 0);

                int32_t index   = this->leaf_index();

                auto result = tree.findForward(Base::search_type_, index, Base::target_);

                return StreamOpResult(result.local_pos(), start, result.local_pos() >= tree.size());
            }
            else {
                return StreamOpResult(start, start, true, true);
            }
        }
        else {
            return StreamOpResult(0, 0, true, true);
        }
    }

    auto& branch_size_prefix() {
        return Base::branch_size_prefix();
    }

    const auto& branch_size_prefix() const {
        return Base::branch_size_prefix();
    }


    template <int32_t StreamIdx, typename StreamType>
    void branch_size_prefix(StreamType&& stream, int32_t start, int32_t end)
    {
        auto sum = stream.sum(0, start, end);

        Base::branch_size_prefix()[StreamIdx] += sum;
    }


    template <int32_t StreamIdx, typename StreamType>
    void branch_size_prefix(StreamType&& stream, int32_t start, int32_t end, FixTargetTag)
    {
//        auto sum = stream->sum(0, start, end);
//
//      Base::branch_size_prefix()[StreamIdx] += sum;
    }


    template <int32_t StreamIdx, typename StreamType>
    void leaf_size_prefix(StreamType&& stream)
    {
        auto size = stream->size();

        Base::branch_size_prefix()[StreamIdx] += size;
    }


    template <
        typename StreamObj,
        typename T,
        int32_t From,
        int32_t To,
        template <typename, int32_t, int32_t> class IterAccumItem
    >
    void branch_iterator_BranchNodeEntry(StreamObj&& obj, IterAccumItem<T, From, To>& item, int32_t start, int32_t end)
    {
        static_assert(To <= StructSizeProvider<StreamObj>::Value, "Invalid BTree structure");

        for (int32_t c = 0; c < To - From; c++)
        {
            obj._add(c + From, start, end, item[c]);
        }
    }

    template <
        typename StreamObj,
        typename T,
        template <typename> class AccumItem
    >
    void branch_iterator_BranchNodeEntry(StreamObj&& obj, AccumItem<T>& item, int32_t start, int32_t end){}

    template <int32_t Offset, int32_t From, int32_t Size, typename StreamObj, typename AccumItem>
    void leaf_iterator_BranchNodeEntry(const StreamObj& obj, AccumItem& item, int32_t start, int32_t end)
    {
        if (obj)
        {
            const int32_t Idx = Offset - AccumItem::From;

            if (end - start == 1 && start > 0)
            {
                for (int32_t c = 0; c < Size; c++)
                {
                    item[Idx + c] += obj.value(c + From, start);
                }
            }
            else {
                for (int32_t c = 0; c < Size; c++)
                {
                    item[Idx + c] = 0;
                    obj._add(c + From, end, item[Idx + c]);
                }
            }
        }
    }

    template <int32_t Offset, int32_t From, int32_t Size, typename StreamObj, typename AccumItem>
    void leaf_iterator_BranchNodeEntry(StreamObj&& obj, AccumItem& item)
    {
        const int32_t Idx = Offset - AccumItem::From;

        if (obj != nullptr) {
            for (int32_t c = 0; c < Size; c++)
            {
                obj._add(c + From, item[Idx + c]);
            }
        }
    }
};


template <
    typename Types
>
class FindMaxWalker: public FindMaxWalkerBase<Types,FindMaxWalker<Types>> {

    using Base  = FindMaxWalkerBase<Types,FindMaxWalker<Types>>;
protected:
    using TargetType    = typename Base::TargetType;

public:
    FindMaxWalker(int32_t leaf_index, const TargetType& target, SearchType search_type = SearchType::GE):
        Base(leaf_index, target, search_type)
    {}
};


template <
    typename Types
>
class FindMaxGTWalker: public FindMaxWalkerBase<Types, FindMaxGTWalker<Types>> {

    using Base          = FindMaxWalkerBase<Types, FindMaxGTWalker<Types>>;
    using TargetType    = typename Base::TargetType;

public:
    FindMaxGTWalker(int32_t leaf_index, const TargetType& target):
        Base(leaf_index, target, SearchType::GT)
    {}

    FindMaxGTWalker(int32_t stream, int32_t leaf_index, const TargetType& target):
        Base(leaf_index, target, SearchType::GT)
    {}
};

template <
    typename Types
>
class FindMaxGEWalker: public FindMaxWalkerBase<Types, FindMaxGEWalker<Types>> {

    using Base          = FindMaxWalkerBase<Types, FindMaxGEWalker<Types>>;
    using TargetType    = typename Base::TargetType;

public:
    FindMaxGEWalker(int32_t leaf_index, const TargetType& target):
        Base(leaf_index, target, SearchType::GE)
    {}

    FindMaxGEWalker(int32_t stream, int32_t leaf_index, const TargetType& target):
        Base(leaf_index, target, SearchType::GE)
    {}
};


}
}}
