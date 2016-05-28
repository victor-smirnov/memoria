
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

    FindWalkerBase(Int leaf_index, const TargetType& target, SearchType search_type):
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

    FindForwardWalkerBase(Int leaf_index, const TargetType& target, SearchType search_type):
        Base(leaf_index, target, search_type)
    {}

    using Base::treeNode;

    template <typename NodeTypes>
    void treeNode(const bt::BranchNode<NodeTypes>* node, WalkCmd cmd, Int start, Int end)
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
    void treeNode(const bt::LeafNode<NodeTypes>* node, WalkCmd cmd, Int start, Int end)
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



    template <Int StreamIdx, typename Tree>
    StreamOpResult find_non_leaf(const Tree* tree, bool root, Int index, Int start)
    {
        auto size = tree->size();

        if (start < size)
        {
            auto k = Base::target_ - Base::sum_;

            auto result = tree->findForward(Base::search_type_, index, start, k);

            Base::sum_ += result.prefix();

            return StreamOpResult(result.idx(), start, result.idx() >= size, false);
        }
        else {
            return StreamOpResult(size, start, true, true);
        }
    }


    template <Int StreamIdx, typename Tree>
    void process_branch_cmd(const Tree* tree, WalkCmd cmd, Int index, Int start, Int end)
    {
        if (cmd == WalkCmd::FIX_TARGET)
        {
            Base::sum_ -= tree->value(index, end);
        }
    }


    template <Int StreamIdx, typename Tree>
    StreamOpResult find_leaf(const Tree* tree, Int start)
    {
        if (tree != nullptr)
        {
            if (start < tree->size())
            {
                auto k = Base::target_ - Base::sum_;

                Int index   = this->leaf_index();

                auto result = tree->findForward(Base::search_type_, index, start, k);

                Base::sum_ += result.prefix();

                return StreamOpResult(result.idx(), start, result.idx() >= tree->size());
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


    template <Int StreamIdx, typename StreamType>
    void branch_size_prefix(const StreamType* stream, Int start, Int end)
    {
        auto sum = stream->sum(0, start, end);

        Base::branch_size_prefix()[StreamIdx] += sum;
    }


    template <Int StreamIdx, typename StreamType>
    void branch_size_prefix(const StreamType* stream, Int start, Int end, FixTargetTag)
    {
//        auto sum = stream->sum(0, start, end);
//
//      Base::branch_size_prefix()[StreamIdx] += sum;
    }


    template <Int StreamIdx, typename StreamType>
    void leaf_size_prefix(const StreamType* stream)
    {
        auto size = stream->size();

        Base::branch_size_prefix()[StreamIdx] += size;
    }


    template <
        typename StreamObj,
        typename T,
        Int From,
        Int To,
        template <typename, Int, Int> class IterAccumItem
    >
    void branch_iterator_BranchNodeEntry(const StreamObj* obj, IterAccumItem<T, From, To>& item, Int start, Int end)
    {
        static_assert(To <= StructSizeProvider<StreamObj>::Value, "Invalid BTree structure");

        for (Int c = 0; c < To - From; c++)
        {
            obj->_add(c + From, start, end, item[c]);
        }
    }

    template <
        typename StreamObj,
        typename T,
        template <typename> class AccumItem
    >
    void branch_iterator_BranchNodeEntry(const StreamObj* obj, AccumItem<T>& item, Int start, Int end){}

    template <Int Offset, Int From, Int Size, typename StreamObj, typename AccumItem>
    void leaf_iterator_BranchNodeEntry(const StreamObj* obj, AccumItem& item, Int start, Int end)
    {
        if (obj != nullptr)
        {
            const Int Idx = Offset - AccumItem::From;

            if (end - start == 1 && start > 0)
            {
                for (Int c = 0; c < Size; c++)
                {
                    item[Idx + c] += obj->value(c + From, start);
                }
            }
            else {
                for (Int c = 0; c < Size; c++)
                {
                    item[Idx + c] = 0;
                    obj->_add(c + From, end, item[Idx + c]);
                }
            }
        }
    }

    template <Int Offset, Int From, Int Size, typename StreamObj, typename AccumItem>
    void leaf_iterator_BranchNodeEntry(const StreamObj* obj, AccumItem& item)
    {
        const Int Idx = Offset - AccumItem::From;

        if (obj != nullptr) {
            for (Int c = 0; c < Size; c++)
            {
                obj->_add(c + From, item[Idx + c]);
            }
        }
    }
};


template <
    typename Types
>
class FindForwardWalker: public FindForwardWalkerBase<Types,FindForwardWalker<Types>> {

    using Base  = FindForwardWalkerBase<Types,FindForwardWalker<Types>>;
protected:
    using TargetType    = typename Base::TargetType;

public:
    FindForwardWalker(Int leaf_index, const TargetType& target, SearchType search_type = SearchType::GE):
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
    FindGTForwardWalker(Int leaf_index, const TargetType& target):
        Base(leaf_index, target, SearchType::GT)
    {}

    FindGTForwardWalker(Int stream, Int leaf_index, const TargetType& target):
        Base(leaf_index, target, SearchType::GT)
    {}
};

template <
    typename Types
>
class FindGEForwardWalker: public FindForwardWalkerBase<Types, FindGTForwardWalker<Types>> {

    using Base          = FindForwardWalkerBase<Types, FindGTForwardWalker<Types>>;
    using TargetType    = typename Base::TargetType;

public:
    FindGEForwardWalker(Int leaf_index, const TargetType& target):
        Base(leaf_index, target, SearchType::GE)
    {}

    FindGEForwardWalker(Int stream, Int leaf_index, const TargetType& target):
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

    FindBackwardWalkerBase(Int leaf_index, const TargetType& target, SearchType search_type):
        Base(leaf_index, target, search_type)
    {}

    template <Int StreamIdx, typename Tree>
    StreamOpResult find_non_leaf(const Tree* tree, bool root, Int index, Int start)
    {
        if (start > tree->size()) start = tree->size() - 1;

        if (start >= 0)
        {
            auto k          = Base::target_ - Base::sum_;

            auto result     = tree->findBackward(Base::search_type_, index, start, k);
            Base::sum_      += result.prefix();

            Int idx = result.idx();

            return StreamOpResult(idx, start, idx < 0);
        }
        else {
            return StreamOpResult(start, start, true, true);
        }
    }


    template <Int StreamIdx, typename Tree>
    StreamOpResult find_leaf(const Tree* tree, Int start)
    {
        if (start > tree->size()) start = tree->size();

        if (start >= 0)
        {
            auto k          = Base::target_ - Base::sum_;

            Int index       = this->leaf_index();

            Int start1      = start == tree->size() ? start - 1 : start;

            auto result     = tree->findBackward(Base::search_type_, index, start1, k);
            Base::sum_      += result.prefix();

            Int idx = result.idx();

            return StreamOpResult(idx, start, idx < 0);
        }
        else {
            return StreamOpResult(start, start, true, true);
        }
    }

    template <typename... Args>
    auto treeNode(Args&&... args) ->
    decltype(std::declval<Base>().treeNode(std::declval<Args>()...))
    {
        return Base::treeNode(std::forward<Args>(args)...);
    }

    template <typename NodeTypes>
    void treeNode(const bt::BranchNode<NodeTypes>* node, WalkCmd cmd, Int start, Int end)
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
    void treeNode(const bt::LeafNode<NodeTypes>* node, WalkCmd cmd, Int start, Int end)
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

    template <Int StreamIdx, typename Tree>
    void process_branch_cmd(const Tree* tree, WalkCmd cmd, Int index, Int start, Int end)
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

    template <Int StreamIdx, typename StreamType>
    void branch_size_prefix(const StreamType* obj, Int start, Int end)
    {
        Int s = start > (obj->size() - 1) ? obj->size() - 1 : start;

        obj->_sub(0, end + 1, s + 1, Base::branch_size_prefix()[StreamIdx]);
    }

    template <Int StreamIdx, typename StreamType>
    void branch_size_prefix(const StreamType* obj, Int start, Int end, FixTargetTag)
    {
        obj->_add(0, end + 1, end + 2, Base::branch_size_prefix()[StreamIdx]);
    }



    template <Int StreamIdx, typename StreamType>
    void leaf_size_prefix(const StreamType* obj)
    {
        Base::branch_size_prefix()[StreamIdx] -= obj->size();
    }


    template <
        typename StreamObj,
        typename T,
        Int From,
        Int To,
        template <typename, Int, Int> class IterAccumItem
    >
    void branch_iterator_BranchNodeEntry(const StreamObj* obj, IterAccumItem<T, From, To>& item, Int start, Int end)
    {
        static_assert(To <= StructSizeProvider<StreamObj>::Value, "Invalid BTree structure");

        Int s = start > (obj->size() - 1) ? obj->size() - 1 : start;

        for (Int c = 0; c < To - From; c++)
        {
            obj->_sub(c + From, end + 1, s + 1, item[c]);
        }
    }

    template <
        typename StreamObj,
        typename T,
        template <typename> class AccumItem
    >
    void branch_iterator_BranchNodeEntry(const StreamObj* obj, AccumItem<T>& item, Int start, Int end){}

    template <Int Offset, Int From, Int Size, typename StreamObj, typename AccumItem>
    void leaf_iterator_BranchNodeEntry(const StreamObj* obj, AccumItem& item, Int start, Int end)
    {
        const Int Idx = Offset - std::remove_reference<decltype(item)>::type::From;

        if (start >= obj->size()) start = obj->size() - 1;

        for (Int c = 0; c < Size; c++)
        {
            item[Idx + c] = 0;
            obj->_add(c + From, end, item[Idx + c]);
        }
    }

    template <Int Offset, Int From, Int Size, typename StreamObj, typename AccumItem>
    void leaf_iterator_BranchNodeEntry(const StreamObj* obj, AccumItem& item, bool leaf)
    {
        const Int Idx = Offset - std::remove_reference<decltype(item)>::type::From;

        if (leaf)
        {
            for (Int c = 0; c < Size; c++)
            {
                item[Idx + c] = 0;
            }
        }
        else {
            for (Int c = 0; c < Size; c++)
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
    FindBackwardWalker(Int leaf_index, const TargetType& target, SearchType search_type = SearchType::GE):
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
    FindGTBackwardWalker(Int leaf_index, const TargetType& target):
        Base(leaf_index, target, SearchType::GT)
    {}

    FindGTBackwardWalker(Int stream, Int leaf_index, const TargetType& target):
        Base(leaf_index, target, SearchType::GT)
    {}
};

template <
    typename Types
>
class FindGEBackwardWalker: public FindBackwardWalkerBase<Types, FindGTBackwardWalker<Types>> {

    using Base  = FindBackwardWalkerBase<Types, FindGTBackwardWalker<Types>>;

    using TargetType = typename Base::TargetType;

public:
    FindGEBackwardWalker(Int leaf_index, const TargetType& target):
        Base(leaf_index, target, SearchType::GE)
    {}

    FindGEBackwardWalker(Int stream, Int leaf_index, const TargetType& target):
        Base(leaf_index, target, SearchType::GE)
    {}
};




}
}}
