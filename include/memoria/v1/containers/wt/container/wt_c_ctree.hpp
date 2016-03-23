
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/core/container/container.hpp>

#include <memoria/v1/containers/wt/wt_names.hpp>

#include <functional>

namespace memoria {


MEMORIA_CONTAINER_PART_BEGIN(memoria::wt::CtrCTreeName)

    typedef typename Base::Tree                                                 Tree;
    typedef typename Base::Seq                                                  Seq;

    typedef typename Tree::Iterator                                             TreeIterator;

//  typedef std::function<Int (Int)>                                            LabelProviderFn;
//  typedef std::function<void (SeqIterator& Int)>                              SeqHandlerFn;

    static const Int BitsPerlabel = 8;

    struct FindChildFn {

        Int label_;
        Int label_idx_;

        Int start_;
        Int end_;

        Int target_idx_ = -1;
        bool try_next_;

        Int size_;

        FindChildFn(Int label): label_(label) {}

        template <Int Idx, typename StreamTypes>
        void stream(const PkdFSSeq<StreamTypes>* seq, Int idx)
        {
            start_      = seq->rank(idx, 1);
            auto result = seq->selectFw(idx, 0, 1);

            size_       = seq->size();

            if (result.is_found())
            {
                end_ = start_ + (result.idx() - idx);
            }
            else {
                end_ = start_ + (size_ - idx);
            }

            try_next_ = !result.is_found();
        }

        template <Int Idx, typename StreamTypes>
        void stream(const PackedFSEArray<StreamTypes>* labels)
        {
            for (Int c = start_; c < end_; c++)
            {
                Int lbl = labels->value(0, c);

                if (lbl >= label_)
                {
                    target_idx_ = c - start_;
                    return;
                }
            }
        }

        template <typename Node>
        void treeNode(const Node* node, Int start)
        {
            node->template processStream<IntList<0>>(*this, start);

            if (end_ > start_)
            {
                node->template processStream<IntList<1, 0, 0>>(*this);
            }
        }
    };



    auto findChild(TreeIterator& node, Int label)
    {
        auto& self = this->self();
        auto& tree = self.tree();

        auto iter = tree.firstChild(node.node());

        while (true)
        {
            FindChildFn fn(label);
            Int idx = iter->idx();

            Tree::LeafDispatcher::dispatch(iter->leaf(), fn, idx);

            if (fn.target_idx_ >= 0)
            {
                if (idx + fn.target_idx_ < fn.size_)
                {
                    iter->idx() += fn.target_idx_;
                }

                break;
            }
            else if (fn.try_next_)
            {
                Int leaf_rest = iter->leaf_size(0) - iter->idx();

                iter->skipFw(leaf_rest);

                if (iter->isEof())
                {
                    break;
                }
            }
            else {
                iter->idx() += fn.end_ - fn.start_;
                break;
            }
        }

        return iter;
    }


//  TreeIterator insertNode(const LoudsNode& at, Int label)
//  {
//      auto& self = this->self();
//      auto& tree = self.tree();
//
//      LoudsNode node = tree.insertNode(at);
//      MEMORIA_ASSERT_TRUE(node);
//      MEMORIA_ASSERT_TRUE(labels()->insert(node.rank1() - 1, label));
//
//      return node;
//  }


/*
    void insertPath(UBigInt path, Int size, function<void (const LoudsNode&, Int label, Int level)> fn)
    {
        auto& self = this->self();
        auto& tree = self.tree();


        LoudsTree* louds = tree();

        LoudsNode node = louds->root();

        Int level = 0;

        while (!louds->isLeaf(node))
        {
            Int label = GetBits(&path, level * BitsPerLabel, BitsPerLabel);

            node = find_child(node, label);

            Int lbl = labels()->value(node.rank1() - 1);

            if (!louds->isLeaf(node) && lbl == label)
            {
                level++;
            }
            else {
                break;
            }
        }

        bool first = true;

        if (level < size)
        {
            while (level < size)
            {
                louds = this->tree();

                louds->insert(node.idx(), 1, 2 - first);
                louds->reindex();

                node = louds->node(node.idx()); // refresh

                Int label = GetBits(&path, level * BitsPerLabel, BitsPerLabel);

                labels()->insert(node.rank1() - 1, label);

                louds->reindex();

                MEMORIA_ASSERT(louds, ==, this->tree());

                fn(node, label, level);

                node = louds->first_child(node);

                level++;
                first = false;
            }

            tree()->insert(node.idx(), 0, 1);
            tree()->reindex();
        }
    }

    bool query_path(UBigInt path, Int size, function<void (const LoudsNode&, Int label, Int level)> fn) const
    {
        LoudsTree* louds = tree();

        LoudsNode node = louds->root();

        Int level = 0;

        while (!louds->isLeaf(node))
        {
            Int label = GetBits(&path, level * BitsPerLabel, BitsPerLabel);

            LoudsNode child = find_child(node, label);

            MEMORIA_ASSERT(child.is_empty(), !=, true);

            level++;
        }

        return level < size;
    }

    enum class Status {
        LEAF, NOT_FOUND, OK, FINISH
    };

    Status remove_path(const LoudsNode& node, UBigInt path, Int size, Int level)
    {
        LoudsTree* louds = tree();
        if (!louds->isLeaf(node))
        {
            Int label = GetBits(&path, level * BitsPerLabel, BitsPerLabel);

            LoudsNode child = find_child(node, label);

            if (child.is_empty())
            {
                return Status::NOT_FOUND;
            }
            else {
                Status status = remove_path(child, path, size, level + 1);

                if (status == Status::OK)
                {
                    bool alone = louds->isAlone(node);

                    if (node.idx() > 0)
                    {
                        removeLeaf(node);
                    }

                    if (alone)
                    {
                        return Status::OK;
                    }
                    else {
                        return Status::FINISH;
                    }
                }
                else {
                    return status;
                }
            }
        }
        else {
            return Status::OK;
        }
    }

    bool remove_path(UBigInt path, Int size)
    {
        LoudsNode node = tree()->root();
        return remove_path(node, path, size, 0) != Status::NOT_FOUND;
    }
*/
MEMORIA_CONTAINER_PART_END

}
