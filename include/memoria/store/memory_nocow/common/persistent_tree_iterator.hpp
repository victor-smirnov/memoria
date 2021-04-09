
// Copyright 2015 Victor Smirnov
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

#include <memoria/core/types.hpp>

#include <memoria/core/tools/optional.hpp>
#include <memoria/core/tools/static_array.hpp>

#include "persistent_tree_node.hpp"

#include <type_traits>
#include <iostream>

/**
 * Serializable Copy-on-write B+Tree (Without leaf links).
 */

namespace memoria {
namespace store {
namespace memory_nocow {

template <typename BranchNode, typename LeafNode>
class PersistentTreeIteratorBase {
public:

    using NodeBaseT     = typename LeafNode::NodeBaseT;
    using LeafNodeT     = LeafNode;
    using BranchNodeT   = BranchNode;

    using Path = core::StaticArray<NodeBaseT*, 8, core::NullPtrFunctor>;

protected:
    Path path_;

    int32_t idx_;

    template <typename, typename, typename, typename>
    friend class PersistentTree;

public:
    PersistentTreeIteratorBase() {}

//    void unlock()
//    {
//    	for (int32_t c = 0; c < path_.size(); c++)
//    	{
//    		path_[c]->unlock();
//    	}
//    }

    int32_t local_pos() const {
        return idx_;
    }

    bool next_leaf()
    {
        Path next = path_;
        if (get_next_node(path_, next, 0))
        {
            path_ = next;
            idx_ = 0;
            return true;
        }
        else {
            return false;
        }
    }

    bool prev_leaf()
    {
        Path prev = path_;
        if (get_prev_node(path_, prev, 0))
        {
            path_ = prev;
            idx_ = this->leaf()->size() - 1;
            return true;
        }
        else {
            return false;
        }
    }

    bool operator++()
    {
        int32_t size = leaf()->size();
        if (++idx_ < size)
        {
            return true;
        }
        else {
            return next_leaf();
        }
    }

    bool operator++(int)
    {
        int32_t size = leaf()->size();
        if (++idx_ < size)
        {
            return true;
        }
        else {
            return next_leaf();
        }
    }

    bool operator--()
    {
        if (++idx_ >= 0)
        {
            return true;
        }
        else {
            return prev_leaf();
        }
    }

    bool operator--(int)
    {
        if (--idx_ >= 0)
        {
            return true;
        }
        else {
            return prev_leaf();
        }
    }

    bool is_end() const
    {
        return idx_ >= leaf()->size();
    }

    bool is_start() const
    {
        return idx_ < 0;
    }

    auto key() const {
        return leaf()->key(idx_);
    }



    static bool get_next_node(Path& path, Path& next, int32_t level)
    {
        if (level < path.size() - 1)
        {
            BranchNode* parent = get_branch_node(path, level + 1);

            int32_t size = parent->size();

            int32_t parent_idx = parent->find_child_node(path[level]);

            if (parent_idx < size - 1)
            {
                next[level] = parent->data(parent_idx + 1);

                return true;
            }
            else if (get_next_node(path, next, level + 1))
            {
                next[level] = to_branch_node(next[level + 1])->first_child();
                return true;
            }
            else {
                return false;
            }
        }
        else {
            return false;
        }
    }

    static bool get_prev_node(Path& path, Path& prev, int32_t level)
    {
        if (level < path.size() - 1)
        {
            BranchNode* parent = get_branch_node(path, level + 1);

            int32_t parent_idx = parent->find_child_node(path[level]);

            if (parent_idx > 0)
            {
                prev[level] = parent->data(parent_idx - 1);

                return true;
            }
            else if (get_prev_node(path, prev, level + 1))
            {
                prev[level] = to_branch_node(prev[level + 1])->last_child();
                return true;
            }
            else {
                return false;
            }
        }
        else {
            return false;
        }
    }

    static BranchNode* get_branch_node(Path& path, int32_t level)
    {
        return static_cast<BranchNode*>(path[level]);
    }

    void dump(std::ostream& out = std::cout) const
    {
        if (path_.size() > 0)
        {
            out << "CoWTree Iterator: idx = " << idx_ << std::endl;
            out << "Leaf: " << std::endl;
            leaf()->dump(out);
        }
        else {
            out << "Empty Iterator" << std::endl;
        }
    }

    void dumpPath(std::ostream& out = std::cout) const
    {
        if (path_.size() > 0)
        {
            out << "CoWTree Iterator: idx = " << idx_ << std::endl;

            for (int32_t c = path_.size() - 1; c >= 0; c--)
            {
                out << "Node: " << c << std::endl;

                NodeBaseT* node = path_[c];

                if (node->is_leaf()) {
                    to_leaf_node(node)->dump(out);
                }
                else {
                    to_branch_node(node)->dump(out);
                }

                out << std::endl;
            }
        }
        else {
            out << "Empty Iterator" << std::endl;
        }
    }
protected:

    NodeBaseT* root() {
        return path_[path_.size() - 1];
    }

    const NodeBaseT* root() const {
        return path_[path_.size() - 1];
    }

    LeafNode* leaf() {
        return static_cast<LeafNode*>(path_[0]);
    }

    const LeafNode* leaf() const {
        return static_cast<const LeafNode*>(path_[0]);
    }

    Path& path() {
        return path_;
    }

    const Path& path() const {
        return path_;
    }


    void set_idx(int32_t idx) {
        idx_ = idx;
    }

    void add_idx(int32_t idx) {
        idx_ += idx;
    }

    static BranchNodeT* to_branch_node(NodeBaseT* node) {
        return static_cast<BranchNodeT*>(node);
    }

    static const BranchNodeT* to_branch_node(const NodeBaseT* node) {
        return static_cast<const BranchNodeT*>(node);
    }

    static LeafNodeT* to_leaf_node(NodeBaseT* node) {
        return static_cast<LeafNodeT*>(node);
    }

    static const LeafNodeT* to_leaf_node(const NodeBaseT* node) {
        return static_cast<const LeafNodeT*>(node);
    }

};


template <typename BranchNode, typename LeafNode>
class PersistentTreeIterator: public PersistentTreeIteratorBase<BranchNode, LeafNode> {
public:

    const auto& value() const {
        return this->leaf()->data(this->local_pos());
    }

    auto& value() {
        return this->leaf()->data(this->local_pos());
    }
};

template <typename BranchNode, typename LeafNode>
class PersistentTreeConstIterator: public PersistentTreeIteratorBase<BranchNode, LeafNode> {
    using Base = PersistentTreeIteratorBase<BranchNode, LeafNode>;

public:

    const auto& value() const {
        return this->leaf()->data(this->local_pos());
    }
};


}
}}
