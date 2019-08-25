
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

#include <memoria/v1/prototypes/bt/nodes/leaf_node.hpp>
#include <memoria/v1/prototypes/bt/nodes/branch_node.hpp>

#include <memoria/v1/prototypes/bt/walkers/bt_walker_base.hpp>


namespace memoria {
namespace v1 {
namespace bt {

struct FixTargetTag {};

/***********************************************************************/

template <typename Types, typename MyType>
class FindWalkerBase: public WalkerBase<Types, MyType> {

public:
    using Base          = WalkerBase<Types, MyType>;

    using LeafPath      = typename Types::LeafPath;

    using TargetType    = typename Types::template TargetType<LeafPath>;

    using ThisType = FindWalkerBase<Types, MyType>;
    using Iterator = typename Base::Iterator;

protected:
    TargetType sum_;

    TargetType target_;

    SearchType search_type_ = SearchType::GT;

public:

    FindWalkerBase(int32_t leaf_index, const TargetType& target, SearchType search_type):
        Base(leaf_index), sum_(), target_(target), search_type_(search_type)
    {}

    const SearchType& search_type() const {
        return search_type_;
    }

    TargetType sum() const {
        return sum_;
    }

    TargetType result() const {
        return sum_;
    }

    TargetType target() const {
        return target_;
    }
};


template <typename Types, typename MyType>
class FindForwardWalkerBase: public FindWalkerBase<Types, MyType> {
    using Base          = FindWalkerBase<Types, MyType>;

public:

    using TargetType    = typename Base::TargetType;
    using Position      = typename Base::Position;
    using LeafPath      = typename Base::LeafPath;

    FindForwardWalkerBase(int32_t leaf_index, const TargetType& target, SearchType search_type):
        Base(leaf_index, target, search_type)
    {}

    using Base::treeNode;

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
    StreamOpResult find_non_leaf(const Tree* tree, bool root, int32_t index, int32_t start)
    {
        auto size = tree->size();

        if (start < size)
        {
            auto k = Base::target_ - Base::sum_;

            auto result = tree->findForward(Base::search_type_, index, start, k);

            Base::sum_ += result.prefix();

            return StreamOpResult(result.local_pos(), start, result.local_pos() >= size, false);
        }
        else {
            return StreamOpResult(size, start, true, true);
        }
    }


    template <int32_t StreamIdx, typename Tree>
    void process_branch_cmd(const Tree* tree, WalkCmd cmd, int32_t index, int32_t start, int32_t end)
    {
        if (cmd == WalkCmd::FIX_TARGET)
        {
            Base::sum_ -= tree->value(index, end);
        }
    }


    template <int32_t StreamIdx, typename Tree>
    StreamOpResult find_leaf(const Tree* tree, int32_t start)
    {
        if (tree != nullptr)
        {
            if (start < tree->size())
            {
                auto k = Base::target_ - Base::sum_;

                int32_t index   = this->leaf_index();

                auto result = tree->findForward(Base::search_type_, index, start, k);

                Base::sum_ += result.prefix();

                return StreamOpResult(result.local_pos(), start, result.local_pos() >= tree->size());
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
    void branch_size_prefix(const StreamType* stream, int32_t start, int32_t end)
    {
        auto sum = stream->sum(0, start, end);

        Base::branch_size_prefix()[StreamIdx] += sum;
    }


    template <int32_t StreamIdx, typename StreamType>
    void branch_size_prefix(const StreamType* stream, int32_t start, int32_t end, FixTargetTag)
    {
//        auto sum = stream->sum(0, start, end);
//
//      Base::branch_size_prefix()[StreamIdx] += sum;
    }


    template <int32_t StreamIdx, typename StreamType>
    void leaf_size_prefix(const StreamType* stream)
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
    void branch_iterator_BranchNodeEntry(const StreamObj* obj, IterAccumItem<T, From, To>& item, int32_t start, int32_t end)
    {
        static_assert(To <= StructSizeProvider<StreamObj>::Value, "Invalid BTree structure");

        for (int32_t c = 0; c < To - From; c++)
        {
            obj->_add(c + From, start, end, item[c]);
        }
    }

    template <
        typename StreamObj,
        typename T,
        template <typename> class AccumItem
    >
    void branch_iterator_BranchNodeEntry(const StreamObj* obj, AccumItem<T>& item, int32_t start, int32_t end){}

    template <int32_t Offset, int32_t From, int32_t Size, typename StreamObj, typename AccumItem>
    void leaf_iterator_BranchNodeEntry(const StreamObj* obj, AccumItem& item, int32_t start, int32_t end)
    {
        if (obj != nullptr)
        {
            const int32_t Idx = Offset - AccumItem::From;

            if (end - start == 1 && start > 0)
            {
                for (int32_t c = 0; c < Size; c++)
                {
                    item[Idx + c] += obj->value(c + From, start);
                }
            }
            else {
                for (int32_t c = 0; c < Size; c++)
                {
                    item[Idx + c] = 0;
                    obj->_add(c + From, end, item[Idx + c]);
                }
            }
        }
    }

    template <int32_t Offset, int32_t From, int32_t Size, typename StreamObj, typename AccumItem>
    void leaf_iterator_BranchNodeEntry(const StreamObj* obj, AccumItem& item)
    {
        const int32_t Idx = Offset - AccumItem::From;

        if (obj != nullptr) {
            for (int32_t c = 0; c < Size; c++)
            {
                obj->_add(c + From, item[Idx + c]);
            }
        }
    }
};


template <
    typename Types
>
class FindForwardWalker: public FindForwardWalkerBase<Types, FindForwardWalker<Types>> {

    using Base  = FindForwardWalkerBase<Types,FindForwardWalker<Types>>;
protected:
    using TargetType    = typename Base::TargetType;

public:
    FindForwardWalker(int32_t leaf_index, const TargetType& target, SearchType search_type = SearchType::GE):
        Base(leaf_index, target, search_type)
    {}
};


template <
    typename Types
>
class FindGTForwardWalker: public FindForwardWalker<Types> {

    using Base          = FindForwardWalker<Types>;
    using TargetType    = typename Base::TargetType;

public:
    FindGTForwardWalker(int32_t leaf_index, const TargetType& target):
        Base(leaf_index, target, SearchType::GT)
    {}

    FindGTForwardWalker(int32_t stream, int32_t leaf_index, const TargetType& target):
        Base(leaf_index, target, SearchType::GT)
    {}
};

template <
    typename Types
>
class FindGEForwardWalker: public FindForwardWalkerBase<Types, FindGEForwardWalker<Types>> {

    using Base          = FindForwardWalkerBase<Types, FindGEForwardWalker<Types>>;
    using TargetType    = typename Base::TargetType;

public:
    FindGEForwardWalker(int32_t leaf_index, const TargetType& target):
        Base(leaf_index, target, SearchType::GE)
    {}

    FindGEForwardWalker(int32_t stream, int32_t leaf_index, const TargetType& target):
        Base(leaf_index, target, SearchType::GE)
    {}
};













template <typename Types, typename MyType>
class FindBackwardWalkerBase: public FindWalkerBase<Types, MyType> {

protected:
    using Base = FindWalkerBase<Types, MyType>;

    using TargetType    = typename Base::TargetType;
    using LeafPath      = typename Base::LeafPath;

public:

    FindBackwardWalkerBase(int32_t leaf_index, const TargetType& target, SearchType search_type):
        Base(leaf_index, target, search_type)
    {}

    template <int32_t StreamIdx, typename Tree>
    StreamOpResult find_non_leaf(const Tree* tree, bool root, int32_t index, int32_t start)
    {
        if (start > tree->size()) start = tree->size() - 1;

        if (start >= 0)
        {
            auto k          = Base::target_ - Base::sum_;

            auto result     = tree->findBackward(Base::search_type_, index, start, k);
            Base::sum_      += result.prefix();

            int32_t idx = result.local_pos();

            return StreamOpResult(idx, start, idx < 0);
        }
        else {
            return StreamOpResult(start, start, true, true);
        }
    }


    template <int32_t StreamIdx, typename Tree>
    StreamOpResult find_leaf(const Tree* tree, int32_t start)
    {
        if (start > tree->size()) start = tree->size();

        if (start >= 0)
        {
            auto k          = Base::target_ - Base::sum_;

            int32_t index       = this->leaf_index();

            int32_t start1      = start == tree->size() ? start - 1 : start;

            auto result     = tree->findBackward(Base::search_type_, index, start1, k);
            Base::sum_      += result.prefix();

            int32_t idx = result.local_pos();

            return StreamOpResult(idx, start, idx < 0);
        }
        else {
            return StreamOpResult(start, start, true, true);
        }
    }

//    template <typename... Args>
//    auto treeNode(Args&&... args) ->
//    decltype(std::declval<Base>().treeNode(std::declval<Args>()...))
//    {
//        return Base::treeNode(std::forward<Args>(args)...);
//    }

    using Base::treeNode;

    template <typename NodeTypes>
    void treeNode(const bt::BranchNode<NodeTypes>* node, WalkCmd cmd, int32_t start, int32_t end)
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

    template <typename NodeTypes>
    void treeNode(const bt::LeafNode<NodeTypes>* node, WalkCmd cmd, int32_t start, int32_t end)
    {
        auto& self = this->self();

        if (cmd == WalkCmd::THE_ONLY_LEAF)
        {
//          self.processLeafIteratorBranchNodeEntry(node, this->leaf_BranchNodeEntry(), start, end);
        }
        else if (cmd == WalkCmd::FIRST_LEAF)
        {
//          self.processLeafIteratorBranchNodeEntry(node, this->leaf_BranchNodeEntry(), true);
        }
        else if (cmd == WalkCmd::LAST_LEAF)  {
//          self.processLeafIteratorBranchNodeEntry(node, this->leaf_BranchNodeEntry(), start, end);
            self.processBranchIteratorBranchNodeEntryWithLeaf(node, this->branch_BranchNodeEntry(), false);

            self.processLeafSizePrefix(node);
        }
        else {
            // ?
        }
    }

    template <int32_t StreamIdx, typename Tree>
    void process_branch_cmd(const Tree* tree, WalkCmd cmd, int32_t index, int32_t start, int32_t end)
    {
        if (cmd == WalkCmd::FIX_TARGET)
        {
            Base::sum_ -= tree->value(index, end + 1);
        }
    }

    auto& branch_size_prefix() {
        return Base::branch_size_prefix();
    }

    const auto& branch_size_prefix() const {
        return Base::branch_size_prefix();
    }

    template <int32_t StreamIdx, typename StreamType>
    void branch_size_prefix(const StreamType* obj, int32_t start, int32_t end)
    {
        int32_t s = start > (obj->size() - 1) ? obj->size() - 1 : start;

        obj->_sub(0, end + 1, s + 1, Base::branch_size_prefix()[StreamIdx]);
    }

    template <int32_t StreamIdx, typename StreamType>
    void branch_size_prefix(const StreamType* obj, int32_t start, int32_t end, FixTargetTag)
    {
        obj->_add(0, end + 1, end + 2, Base::branch_size_prefix()[StreamIdx]);
    }



    template <int32_t StreamIdx, typename StreamType>
    void leaf_size_prefix(const StreamType* obj)
    {
        Base::branch_size_prefix()[StreamIdx] -= obj->size();
    }


    template <
        typename StreamObj,
        typename T,
        int32_t From,
        int32_t To,
        template <typename, int32_t, int32_t> class IterAccumItem
    >
    void branch_iterator_BranchNodeEntry(const StreamObj* obj, IterAccumItem<T, From, To>& item, int32_t start, int32_t end)
    {
        static_assert(To <= StructSizeProvider<StreamObj>::Value, "Invalid BTree structure");

        int32_t s = start > (obj->size() - 1) ? obj->size() - 1 : start;

        for (int32_t c = 0; c < To - From; c++)
        {
            obj->_sub(c + From, end + 1, s + 1, item[c]);
        }
    }

    template <
        typename StreamObj,
        typename T,
        template <typename> class AccumItem
    >
    void branch_iterator_BranchNodeEntry(const StreamObj* obj, AccumItem<T>& item, int32_t start, int32_t end){}

    template <int32_t Offset, int32_t From, int32_t Size, typename StreamObj, typename AccumItem>
    void leaf_iterator_BranchNodeEntry(const StreamObj* obj, AccumItem& item, int32_t start, int32_t end)
    {
        const int32_t Idx = Offset - std::remove_reference<decltype(item)>::type::From;

        if (start >= obj->size()) start = obj->size() - 1;

        for (int32_t c = 0; c < Size; c++)
        {
            item[Idx + c] = 0;
            obj->_add(c + From, end, item[Idx + c]);
        }
    }

    template <int32_t Offset, int32_t From, int32_t Size, typename StreamObj, typename AccumItem>
    void leaf_iterator_BranchNodeEntry(const StreamObj* obj, AccumItem& item, bool leaf)
    {
        const int32_t Idx = Offset - std::remove_reference<decltype(item)>::type::From;

        if (leaf)
        {
            for (int32_t c = 0; c < Size; c++)
            {
                item[Idx + c] = 0;
            }
        }
        else {
            for (int32_t c = 0; c < Size; c++)
            {
                obj->_sub(c + From, item[Idx + c]);
            }
        }
    }
};


template <
    typename Types
>
class FindBackwardWalker: public FindBackwardWalkerBase<Types, FindBackwardWalker<Types>> {

    using Base  = FindBackwardWalkerBase<Types, FindBackwardWalker<Types>>;

    using TargetType    = typename Base::TargetType;

public:
    FindBackwardWalker(int32_t leaf_index, const TargetType& target, SearchType search_type = SearchType::GE):
        Base(leaf_index, target, search_type)
    {}
};


template <
    typename Types
>
class FindGTBackwardWalker: public FindBackwardWalkerBase<Types, FindGTBackwardWalker<Types>> {

    using Base  = FindBackwardWalkerBase<Types, FindGTBackwardWalker<Types>>;

    using TargetType    = typename Base::TargetType;

public:
    FindGTBackwardWalker(int32_t leaf_index, const TargetType& target):
        Base(leaf_index, target, SearchType::GT)
    {}

    FindGTBackwardWalker(int32_t stream, int32_t leaf_index, const TargetType& target):
        Base(leaf_index, target, SearchType::GT)
    {}
};




template <
    typename Types
>
class FindGEBackwardWalker: public FindBackwardWalkerBase<Types, FindGEBackwardWalker<Types>> {

    using Base  = FindBackwardWalkerBase<Types, FindGEBackwardWalker<Types>>;

    using TargetType = typename Base::TargetType;

public:
    FindGEBackwardWalker(int32_t leaf_index, const TargetType& target):
        Base(leaf_index, target, SearchType::GE)
    {}

    FindGEBackwardWalker(int32_t stream, int32_t leaf_index, const TargetType& target):
        Base(leaf_index, target, SearchType::GE)
    {}
};




}
}}
