
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

#include <memoria/v1/core/types/types.hpp>

#include <dumbo/v1/allocators/inmem/persistent_tree_node.hpp>


#include <memoria/v1/core/tools/optional.hpp>
#include <memoria/v1/core/tools/static_array.hpp>

#include <type_traits>

/**
 * Serializable Copy-on-write B+Tree (Without leaf links).
 */

namespace dumbo {
namespace v1 {
namespace inmem {

using namespace memoria::v1;

template <typename BranchNode, typename LeafNode>
class PersistentTreeIteratorBase {
public:

    using NodeBaseT     = typename LeafNode::NodeBaseT;
    using LeafNodeT     = LeafNode;
    using BranchNodeT   = BranchNode;

    using Path = core::StaticArray<NodeBaseT*, 8, core::NullPtrFunctor>;

protected:
    Path path_;

    Int idx_;

    template <typename, typename, typename, typename>
    friend class PersistentTree;

public:
    PersistentTreeIteratorBase() {}

    void unlock()
    {
    	for (Int c = 0; c < path_.size(); c++)
    	{
    		path_[c]->unlock();
    	}
    }

    Int idx() const {
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
        Int size = leaf()->size();
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
        Int size = leaf()->size();
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



    static bool get_next_node(Path& path, Path& next, Int level)
    {
        if (level < path.size() - 1)
        {
            BranchNode* parent = get_branch_node(path, level + 1);

            Int size = parent->size();

            Int parent_idx = parent->find_child_node(path[level]);

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

    static bool get_prev_node(Path& path, Path& prev, Int level)
    {
        if (level < path.size() - 1)
        {
            BranchNode* parent = get_branch_node(path, level + 1);

            Int parent_idx = parent->find_child_node(path[level]);

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

    static BranchNode* get_branch_node(Path& path, Int level)
    {
        return static_cast<BranchNode*>(path[level]);
    }

    void dump(std::ostream& out = std::cout) const
    {
        if (path_.size() > 0)
        {
            out<<"CoWTree Iterator: idx = "<<idx_<<endl;
            out<<"Leaf: "<<endl;
            leaf()->dump(out);
        }
        else {
            out<<"Empty Iterator"<<endl;
        }
    }

    void dumpPath(std::ostream& out = std::cout) const
    {
        if (path_.size() > 0)
        {
            out<<"CoWTree Iterator: idx = "<<idx_<<endl;

            for (Int c = path_.size() - 1; c >= 0; c--)
            {
                out<<"Node: "<<c<<endl;

                NodeBaseT* node = path_[c];

                if (node->is_leaf()) {
                    to_leaf_node(node)->dump(out);
                }
                else {
                    to_branch_node(node)->dump(out);
                }

                out<<endl;
            }
        }
        else {
            out<<"Empty Iterator"<<endl;
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


    void set_idx(Int idx) {
        idx_ = idx;
    }

    void add_idx(Int idx) {
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
        return this->leaf()->data(this->idx());
    }

    auto& value() {
        return this->leaf()->data(this->idx());
    }
};

template <typename BranchNode, typename LeafNode>
class PersistentTreeConstIterator: public PersistentTreeIteratorBase<BranchNode, LeafNode> {
    using Base = PersistentTreeIteratorBase<BranchNode, LeafNode>;

public:

    const auto& value() const {
        return this->leaf()->data(this->idx());
    }
};


}
}}
