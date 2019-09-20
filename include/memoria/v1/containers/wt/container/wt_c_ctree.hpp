
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

#include <memoria/v1/core/container/container.hpp>

#include <memoria/v1/containers/wt/wt_names.hpp>

#include <functional>

namespace memoria {
namespace v1 {


MEMORIA_V1_CONTAINER_PART_BEGIN(wt::CtrCTreeName)
public:
    typedef typename Base::Tree                                                 Tree;
    typedef typename Base::Seq                                                  Seq;

    typedef typename Tree::Iterator                                             TreeIterator;

    static const int32_t BitsPerlabel = 8;

    struct FindChildFn {

        int32_t label_;
        int32_t label_idx_;

        int32_t start_;
        int32_t end_;

        int32_t target_idx_ = -1;
        bool try_next_;

        int32_t size_;

        FindChildFn(int32_t label): label_(label) {}

        template <int32_t Idx, typename StreamTypes>
        void stream(const PkdFSSeq<StreamTypes>* seq, int32_t idx)
        {
            start_      = seq->rank(idx, 1);
            auto result = seq->selectFw(idx, 0, 1);

            size_       = seq->size();

            if (result.is_found())
            {
                end_ = start_ + (result.iter_local_pos() - idx);
            }
            else {
                end_ = start_ + (size_ - idx);
            }

            try_next_ = !result.is_found();
        }

        template <int32_t Idx, typename StreamTypes>
        void stream(const PackedFSEArray<StreamTypes>* labels)
        {
            for (int32_t c = start_; c < end_; c++)
            {
                int32_t lbl = labels->value(0, c);

                if (lbl >= label_)
                {
                    target_idx_ = c - start_;
                    return;
                }
            }
        }

        template <typename Node>
        void treeNode(const Node* node, int32_t start)
        {
            node->template processStream<IntList<0, 1>>(*this, start);

            if (end_ > start_)
            {
                node->template processStream<IntList<1, 1, 0>>(*this);
            }
        }
    };



    auto findChild(TreeIterator& node, int32_t label)
    {
        auto& self = this->self();
        auto& tree = self.tree();

        auto iter = tree->firstChild(node.node());

        while (true)
        {
            FindChildFn fn(label);
            int32_t idx = iter->iter_local_pos();

            Tree::self().leaf_dispatcher().dispatch(iter->iter_leaf(), fn, idx);

            if (fn.target_idx_ >= 0)
            {
                if (idx + fn.target_idx_ < fn.size_)
                {
                    iter->iter_local_pos() += fn.target_idx_;
                }

                break;
            }
            else if (fn.try_next_)
            {
                int32_t leaf_rest = iter->iter_leaf_size(0) - iter->iter_local_pos();

                iter->skipFw(leaf_rest);

                if (iter->isEof())
                {
                    break;
                }
            }
            else {
                iter->iter_local_pos() += fn.end_ - fn.start_;
                break;
            }
        }

        return iter;
    }


//  TreeIterator insertNode(const LoudsNode& at, int32_t label)
//  {
//      auto& self = this->self();
//      auto& tree = self.tree();
//
//      LoudsNode node = tree.insertNode(at);
//      MEMORIA_V1_ASSERT_TRUE(node);
//      MEMORIA_V1_ASSERT_TRUE(labels()->insert(node.rank1() - 1, label));
//
//      return node;
//  }


/*
    void insertPath(uint64_t path, int32_t size, function<void (const LoudsNode&, int32_t label, int32_t level)> fn)
    {
        auto& self = this->self();
        auto& tree = self.tree();


        LoudsTree* louds = tree();

        LoudsNode node = louds->root();

        int32_t level = 0;

        while (!louds->isLeaf(node))
        {
            int32_t label = GetBits(&path, level * BitsPerLabel, BitsPerLabel);

            node = find_child(node, label);

            int32_t lbl = labels()->value(node.rank1() - 1);

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

                louds->insert(node.iter_local_pos(), 1, 2 - first);
                louds->reindex();

                node = louds->node(node.iter_local_pos()); // iter_refresh

                int32_t label = GetBits(&path, level * BitsPerLabel, BitsPerLabel);

                labels()->insert(node.rank1() - 1, label);

                louds->reindex();

                MEMORIA_V1_ASSERT(louds, ==, this->tree());

                fn(node, label, level);

                node = louds->first_child(node);

                level++;
                first = false;
            }

            tree()->insert(node.iter_local_pos(), 0, 1);
            tree()->reindex();
        }
    }

    bool query_path(uint64_t path, int32_t size, function<void (const LoudsNode&, int32_t label, int32_t level)> fn) const
    {
        LoudsTree* louds = tree();

        LoudsNode node = louds->root();

        int32_t level = 0;

        while (!louds->isLeaf(node))
        {
            int32_t label = GetBits(&path, level * BitsPerLabel, BitsPerLabel);

            LoudsNode child = find_child(node, label);

            MEMORIA_V1_ASSERT(child.is_empty(), !=, true);

            level++;
        }

        return level < size;
    }

    enum class Status {
        LEAF, NOT_FOUND, OK, FINISH
    };

    Status remove_path(const LoudsNode& node, uint64_t path, int32_t size, int32_t level)
    {
        LoudsTree* louds = tree();
        if (!louds->isLeaf(node))
        {
            int32_t label = GetBits(&path, level * BitsPerLabel, BitsPerLabel);

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

                    if (node.iter_local_pos() > 0)
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

    bool remove_path(uint64_t path, int32_t size)
    {
        LoudsNode node = tree()->root();
        return remove_path(node, path, size, 0) != Status::NOT_FOUND;
    }
*/
MEMORIA_V1_CONTAINER_PART_END

}}
